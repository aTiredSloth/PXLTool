#include "UI.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Renderer/OfflineRenderer.hpp"
#include "Core/Runtime/SceneDescription.hpp"
#include "crow/app.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include <Crow/include/crow.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <saucer/smartview.hpp>
#include <sstream>
#include <string>
#include <tuple>

constexpr auto const Port = 4081;
std::future<void> Server;
crow::SimpleApp App;
namespace UI
{
	inline static const char Base64Chars[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	std::tuple<uint32_t, uint32_t, uint32_t, SceneDescription> GetSceneDescription(SQLite::Database& DB, SQLite::Statement& Query)
	{
		uint32_t Width = 512;
		uint32_t Height = 512;
		uint32_t Frames = 1;
		
		SceneDescription Desc;
		while (Query.executeStep())
   	{
        	double Time = Query.getColumn(1);
        	const char* CameraData = Query.getColumn(2);
        	const char* Models = Query.getColumn(3);
         Width = Query.getColumn(4);
         Height = Query.getColumn(5);
         const char* pPostShaderString = Query.getColumn(6);
         Frames = Query.getColumn(7);
         
         Desc.TimeStamp = Time;
         crow::json::rvalue CameraJson = crow::json::load(CameraData);
         Desc.CameraData.Location.x = CameraJson["Location"]["x"].d();
         Desc.CameraData.Location.y = CameraJson["Location"]["y"].d();
         Desc.CameraData.Location.z = CameraJson["Location"]["z"].d();
         
         glm::vec3 Euler;
         Euler.x = glm::radians(CameraJson["Rotation"]["x"].d());
         Euler.y = glm::radians(CameraJson["Rotation"]["y"].d());
         Euler.z = glm::radians(CameraJson["Rotation"]["z"].d());
         
         Desc.CameraData.Rotation = glm::quat(Euler);

         Desc.CameraData.bIsOrtho = CameraJson["IsOrtho"].b();
         Desc.CameraData.OrthoLeft = CameraJson["OrthoLeft"].d();
         Desc.CameraData.OrthoRight = CameraJson["OrthoRight"].d();
         Desc.CameraData.OrthoTop = CameraJson["OrthoTop"].d();
         Desc.CameraData.OrthoBottom = CameraJson["OrthoBottom"].d();
         Desc.CameraData.Aspect = CameraJson["Aspect"].d();
         Desc.CameraData.Near = CameraJson["Near"].d();
         Desc.CameraData.Far = CameraJson["Far"].d();
         Desc.CameraData.FOV = CameraJson["FOV"].d();
	
         crow::json::rvalue ModelsJson = crow::json::load(Models);

         for (auto& Model : ModelsJson)
			{
				AssetID MeshId = Model["MeshId"].u();
				AssetID AnimationId = Model["AnimationId"].u();
				AssetID ShaderId = Model["ShaderId"].u();
				AssetID TextureId = Model["TextureId"].u();

				Transform Transformation;
				Transformation.Location.x = Model["Location"]["x"].d();
				Transformation.Location.y = Model["Location"]["y"].d();
				Transformation.Location.z = Model["Location"]["z"].d();

     			Euler.x = glm::radians(Model["Rotation"]["x"].d());
            Euler.y = glm::radians(Model["Rotation"]["y"].d());
            Euler.z = glm::radians(Model["Rotation"]["z"].d());

            Transformation.Rotation = Euler;

            Transformation.Scale.x = Model["Scale"]["x"].d();
				Transformation.Scale.y = Model["Scale"]["y"].d();
				Transformation.Scale.z = Model["Scale"]["z"].d();

				Desc.Models.push_back({MeshId, AnimationId, ShaderId, TextureId, Transformation});
			}
			
         crow::json::rvalue PostShader = crow::json::load(pPostShaderString);
         for (auto& Id : PostShader)
			{
				Desc.PostShaders.push_back(Id.u());
			}
      }

		return {Width, Height, Frames, Desc};
	}
	
	std::string Base64Encode(const std::vector<char>& Data)
	{
		std::string Out;
		
		int Val = 0;
		int ValB = -6;
		
		for (unsigned char C : Data)
		{
			Val = (Val << 8) + C;
			ValB += 8;
			
			while (ValB >= 0)
			{
				Out.push_back(Base64Chars[(Val >> ValB) & 0x3F]);
				ValB -= 6;
			}
		}
		
		if (ValB > -6)
		{
			Out.push_back(Base64Chars[((Val << 8) >> (ValB + 8)) & 0x3F]);
		}
		
		while (Out.size() % 4)
		{
			Out.push_back('=');
		}
		
		return Out;
	}

	void RegisterAPI(SQLite::Database& pDB);
	coco::stray start(saucer::application* app)
	{
		auto window = saucer::window::create(app).value();
		auto webview = saucer::smartview::create({ .window = window });
		
		window->set_title("PXLTool");
		
		webview->set_url(std::string("http://localhost:") + std::to_string(App.port()));
		
		window->show();
		
		co_await app->finish();
	}

	int Start(SQLite::Database& DB)
	{
		std::vector<ModelInstance> Models;
		DB.exec("CREATE TABLE IF NOT EXISTS SCENES ( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT, Width INTEGER, Height INTEGER, PostShaders TEXT, FramesPerSecond INTEGER)");
		
		RegisterAPI(DB);
		
		Server = App.port(Port).multithreaded().run_async();
		auto Result = saucer::application::create({ .id = "PXLTool" })->run(start);
		App.stop();
		
		return Result;
	}

	void RegisterAPI(SQLite::Database& DB)
	{
		CROW_ROUTE(App, "/")
		([]()
		{
			crow::response res;
			res.set_static_file_info("UI/build/200.html");
			
			return res; 
		});

		CROW_ROUTE(App, "/_app/<path>")
		([](const crow::request&, crow::response& res, std::string path)
		{
			namespace fs = std::filesystem;

			path = "_app/" + path;
			fs::path base = "UI/build";
			fs::path requested = base / path;
			
			if(fs::exists(requested) && fs::is_regular_file(requested))
			{
      		res.set_static_file_info(requested.string());
			}
			else
			{
      		res.set_static_file_info((base / "200.html").string());
			}
				
			res.end(); 
		});
		
		static std::string const CameraDefault = R"({
    "IsOrtho": false,
    "OrthoLeft": -64,
    "OrthoRight": 64,
    "OrthoTop": 64,
    "OrthoBottom": -64,
    "Aspect": 1.7778,
    "Near": 0.01,
    "Far": 1000.0,
    "FOV": 90,
    "Location": { "x": 0, "y": 0, "z": 0 },
    "Rotation": { "x": 0, "y": 0, "z": 0 }
})";
		
		//get all scene
		CROW_ROUTE(App, "/api/scenes")
		([&DB]()
		{
			crow::json::wvalue Json;

			try
			{
				SQLite::Statement Query(DB, "SELECT * FROM SCENES");

				uint32_t i = 0;
				while (Query.executeStep())
    			{
        			const char* Name = Query.getColumn(0);
        			double Time = Query.getColumn(1);
        			const char* CameraData = Query.getColumn(2);
        			const char* Models = Query.getColumn(3);
           		int Width = Query.getColumn(4);
           		int Height = Query.getColumn(5);
            	const char* pPostShaderJsonString = Query.getColumn(6);
             	int FramesPerSecond = Query.getColumn(7);
           		Json[i]["Name"] = Name;
             	Json[i]["Time"] = Time;
             	Json[i]["Camera"] = crow::json::load(CameraData ? CameraData : CameraDefault.c_str());
             	Json[i]["Models"] = crow::json::load(Models ? Models : "[]");
             	Json[i]["Width"] = Width;
             	Json[i]["Height"] = Height;
             	Json[i]["PostShaders"] = crow::json::load(pPostShaderJsonString ? pPostShaderJsonString : "[]");
             	Json[i]["FramesPerSecond"] = FramesPerSecond;
              	i += 1;
    			}
			}
			catch(std::exception& e)
			{
				std::cout << e.what() << std::endl;
			}

			return Json; 
		});
		
		//create scene
		CROW_ROUTE(App, "/api/newscene/<string>")
		([&DB](const std::string& Name)
		{
			SQLite::Statement Query (DB, "INSERT INTO SCENES VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
			
			Query.bind(1, Name);
			Query.bind(2, 0);
			Query.bind(3, CameraDefault);
			Query.bind(4, "[]");
			Query.bind(5, 0);
			Query.bind(6, 0);
			Query.bind(7, "[]");
			Query.bind(8, 1);
			Query.exec();

			return Name;
		});

		//delete scene
		CROW_ROUTE(App, "/api/removescene/<string>")
		([&DB](const std::string& Name)
		{
			std::string const Command = "DELETE FROM SCENES WHERE Name = \"" + Name + "\"";
			DB.exec(Command.c_str());

			return 200;
		});
		
		//render scene
		CROW_ROUTE(App, "/api/scene/<string>/render")
		([&DB](const std::string& Name)
		{
			std::string const Command = "SELECT * FROM SCENES WHERE Name = \"" + Name + "\"";
			
			SQLite::Statement Query(DB, Command);

			auto const [Width, Height, Frames, Desc] = GetSceneDescription(DB, Query);
			
			auto [Color, Normal, Depth] = OfflineRenderer::DispatchRender(Desc, {Width, Height});

			if (!Color || !Normal || !Depth)
			{
				return crow::response(500);
			}
			
			std::vector<char> const ColorBytes(Color->GetSize(), 0);
			Color->Copy((char*)ColorBytes.data());
			std::vector<char> const NormalBytes(Normal->GetSize(), 0);
			Normal->Copy((char*)NormalBytes.data());
			std::vector<char> const DepthBytes(Depth->GetSize(), 0);
			Depth->Copy((char*)DepthBytes.data());
			
			crow::json::wvalue Json;

			Json["color"] = Base64Encode(ColorBytes);
			Json["normal"] = Base64Encode(NormalBytes);
			Json["depth"] = Base64Encode(DepthBytes);

			return crow::response(Json);
		});

		//export scene
		CROW_ROUTE(App, "/api/scene/<string>/export/<string>")
		([&DB](const std::string& Name, const std::string& Path)
		{
			std::string const Command = "SELECT * FROM SCENES WHERE Name = \"" + Name + "\"";
			
			SQLite::Statement Query(DB, Command);

			auto [Width, Height, FramesPerSecond, Desc] = GetSceneDescription(DB, Query);

			float MaxTime = 0.0f;
			float TickTime = 1.0f;
			for (auto& Model : Desc.Models)
			{
				if (!Model.AnimationId)
				{
					continue;
				}

				auto Animation = AssetManager::GetAnimation(Model.AnimationId);

				if (!Animation)
				{
					continue;
				}

				if (MaxTime < Animation->Length/Animation->TicksPerSecond)
				{
					MaxTime = Animation->Length/Animation->TicksPerSecond;
					TickTime = Animation->TicksPerSecond;
				}
			}
			uint32_t FrameCount = MaxTime * FramesPerSecond;
			
			for (uint32_t i = 0; i < FrameCount; ++i)
			{
				auto [Color, Normal, Depth] = OfflineRenderer::DispatchRender(Desc, {Width, Height});

				if (!Color || !Normal || !Depth)
				{
					return crow::response(500);
				}

				Desc.TimeStamp = ((float)i/(float)FrameCount) * MaxTime * TickTime;
				
				std::vector<char> const ColorBytes(Color->GetSize(), 0);
				Color->Copy((char*)ColorBytes.data());
				std::vector<char> const NormalBytes(Normal->GetSize(), 0);
				Normal->Copy((char*)NormalBytes.data());
				std::vector<char> const DepthBytes(Depth->GetSize(), 0);
				Depth->Copy((char*)DepthBytes.data());

				std::string const BaseName = "./" + Path + "/" + Name + "_" + std::to_string(i);
				std::ofstream OutColorFile(BaseName + "_Color.png", std::ios::binary);
				std::ofstream OutNormalFile(BaseName + "_Normal.png", std::ios::binary);
				std::ofstream OutDepthFile(BaseName + "_Depth.png", std::ios::binary);

				if (!OutColorFile || !OutNormalFile || !OutDepthFile)
				{
					return crow::response(500);
				}
				
				OutColorFile.write(ColorBytes.data(), ColorBytes.size());
				OutNormalFile.write(NormalBytes.data(), NormalBytes.size());
				OutDepthFile.write(DepthBytes.data(), DepthBytes.size());

				OutColorFile.close();
				OutNormalFile.close();
				OutDepthFile.close();
			}

			return crow::response(200);
		});
		
		//edit scene
		CROW_ROUTE(App, "/api/scene/<string>/update").methods(crow::HTTPMethod::Post)
		([&DB](const crow::request& req, const std::string& Name)
		{
			crow::json::rvalue Json = crow::json::load(req.body);
			
			crow::json::wvalue CameraJson = Json["Camera"];
			crow::json::wvalue ModelsJson = Json["Models"];
			crow::json::wvalue PostShaders = Json["PostShaders"];
			
			std::string const Command =
    		"UPDATE SCENES SET Time = ? , Camera = ?, Models = ?, Width = ?, Height = ?, PostShaders = ?, FramesPerSecond = ? WHERE Name = ?";
			SQLite::Statement Query(DB, Command);
			Query.bind(1, Json["Time"].d());
			Query.bind(2, CameraJson.dump());
			Query.bind(3, ModelsJson.dump());
			Query.bind(4, Json["Width"].i());
			Query.bind(5, Json["Height"].i());
			Query.bind(6, PostShaders.dump());
			Query.bind(7, Json["FramesPerSecond"].i());
			Query.bind(8, Name);
			Query.exec();

			return 200;
		});
		//get all meshes
		CROW_ROUTE(App, "/api/meshes")
		([]()
		{
			crow::json::wvalue Json;

			uint64_t i = 0;
			for (auto& Model : AssetManager::GetAllMeshIds())
			{
				Json[i] = Model;
				i += 1;
			}

			return Json;
		});
		//get all animations
		CROW_ROUTE(App, "/api/animations")
		([]()
		{
			crow::json::wvalue Json;

			uint64_t i = 0;
			for (auto& Animation : AssetManager::GetAllAnimationIds())
			{
				Json[i] = Animation;
				i += 1;
			}

			return Json;
		});
		//get all shaders
		CROW_ROUTE(App, "/api/shaders")
		([]()
		{
			crow::json::wvalue Json;

			uint64_t i = 0;
			for (auto& Animation : AssetManager::GetAllShaderIds())
			{
				Json[i] = Animation;
				i += 1;
			}

			return Json;
		});
		//get all textures
		CROW_ROUTE(App, "/api/textures")
		([]()
		{
			crow::json::wvalue Json;

			uint64_t i = 0;
			for (auto& Texture : AssetManager::GetAllTextureIds())
			{
				Json[i] = Texture;
				i += 1;
			}

			return Json;
		});
		//Load
		CROW_ROUTE(App, "/api/load").methods(crow::HTTPMethod::Post)
		([](const crow::request& req)
		{
   		crow::multipart::message Message(req);

        	auto Part = Message.get_part_by_name("file");
         auto Header = Part.get_header_object("Content-Disposition");

        	std::string FileName = Header.params["filename"];

         auto DotPos = FileName.find_last_of('.');
 
         if (DotPos == std::string::npos)
         {
             return crow::response(400, "Missing extension");
         }
         
			auto Extension = FileName.substr(DotPos);

         std::stringstream Stream;
         Stream.write(Part.body.c_str(), Part.body.size());
         
			if (Extension == ".glsl")
			{
				AssetManager::LoadShader(FileName, Stream);
			}
			else if (Extension == ".obj" || Extension == ".gltf" || Extension == ".dae")
			{
				AssetManager::LoadScene(Extension.substr(1), Stream);
			}
			else if (Extension == ".png")
			{
				AssetManager::LoadTexture(Stream);
			}
			
        	return crow::response(200, "File uploaded");
		});
	}
}
