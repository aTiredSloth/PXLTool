#include "UI.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Renderer/OfflineRenderer.hpp"
#include "Core/Runtime/SceneDescription.hpp"
#include "crow/app.h"
#include "crow/http_request.h"
#include "crow/http_response.h"
#include "crow/json.h"
#include <Crow/include/crow.h>
#include <SQLiteCpp/Statement.h>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <saucer/smartview.hpp>
#include <string>

constexpr auto const Port = 4081;
std::future<void> Server;
crow::SimpleApp App;
namespace UI
{

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
		float TimeStamp;
		Camera CameraData;
		std::vector<ModelInstance> Models;
		DB.exec("CREATE TABLE IF NOT EXISTS SCENES ( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)");
		
		RegisterAPI(DB);
		
		Server = App.port(Port).run_async();
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
        			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
        			const char* Name = Query.getColumn(0);
        			double Time = Query.getColumn(1);
        			const char* CameraData = Query.getColumn(2);
        			const char* Models = Query.getColumn(3);

           		Json[i]["Name"] = Name;
             	Json[i]["Time"] = Time;
             	Json[i]["Camera"] = crow::json::wvalue(CameraData);
             	Json[i]["Models"] = crow::json::wvalue(Models);
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
			std::string const Command = "INSERT INTO SCENES VALUES (\"" + Name + "\", 0, \"{}\", \"{}\")";
			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
			DB.exec(Command.c_str());

			return Name;
		});

		//delete scene
		CROW_ROUTE(App, "/api/removescene/<string>")
		([&DB](const std::string& Name)
		{
			std::string const Command = "DELETE FROM SCENES WHERE Name = \"" + Name + "\"";
			DB.exec(Command.c_str());

			return 1;
		});
		
		//render scene
		CROW_ROUTE(App, "/api/scene/<string>/render")
		([&DB](const std::string& Name)
		{
			std::string const Command = "SELECT * FROM SCENES WHERE Name = \"" + Name + "\"";
			
			SQLite::Statement Query(DB, Command);

			SceneDescription Desc;
			while (Query.executeStep())
    		{
        		//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
        		double Time = Query.getColumn(1);
        		const char* CameraData = Query.getColumn(2);
        		const char* Models = Query.getColumn(3);

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

           	crow::json::rvalue ModelsJson = crow::json::load(Models);

            for (auto& Model : ModelsJson)
				{
					AssetID MeshId = Model["MeshId"].u();
					AssetID AnimationId = Model["AnimationId"].u();
					AssetID ShaderId = Model["ShaderId"].u();

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

					Desc.Models.push_back({MeshId, AnimationId, ShaderId, Transformation});
				}
    		}
			
			auto Promise = OfflineRenderer::DispatchRender(Desc, {128, 128});

			auto Future = Promise.get_future();
			
			Future.wait();

			return 1;
		});
		
		//edit scene
		//
		CROW_ROUTE(App, "/api/scene/<string>/update")
		([&DB](const crow::request& req, const std::string& Name)
		{
			crow::json::rvalue Json = crow::json::load(req.body);
			
			crow::json::wvalue CameraJson = Json["Camera"];
			crow::json::wvalue ModelsJson = Json["Models"];
			
			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
			std::string const Command = "UPDATE SCENES SET Time = " + std::to_string(Json["Time"].d()) + 
			", Camera = '" + CameraJson.dump() + "', Models = '" + ModelsJson.dump() + "' WHERE Name = '" + Name + "'";
			
			SQLite::Statement Query(DB, Command);

			Query.exec();

			return 1;
		});
			//set Models
		CROW_ROUTE(App, "/api/scene/<string>/setmodels")
		([&DB](const crow::request& req, const std::string& Name)
		{
			crow::json::rvalue Json = crow::json::load(req.body);
			
			crow::json::wvalue ModelsJson = Json["Models"];
			
			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
			std::string const Command = "UPDATE SCENES SET Models = '" + ModelsJson.dump() + "' WHERE Name = '" + Name + "'";
			
			SQLite::Statement Query(DB, Command);

			Query.exec();

			return 1;
		});
		//set Camera
		CROW_ROUTE(App, "/api/scene/<string>/setcamera")
		([&DB]( const crow::request& req, const std::string& Name)
		{
			crow::json::rvalue Json = crow::json::load(req.body);
			
			crow::json::wvalue CameraJson = Json["Camera"];
			
			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
			std::string const Command = "UPDATE SCENES SET Camera = '" + CameraJson.dump() + "' WHERE Name = '" + Name + "'";
			
			SQLite::Statement Query(DB, Command);

			Query.exec();

			return 1;
		});
			//set Time
		CROW_ROUTE(App, "/api/scene/<string>/settime")
		([&DB](const crow::request& req, const std::string& Name)
		{
			//( Name TEXT PRIMARY KEY, Time REAL, Camera TEXT, Models TEXT)
			std::string const Command = "UPDATE SCENES SET Time = " + req.body + " WHERE Name = '" + Name + "'";
			
			SQLite::Statement Query(DB, Command);

			Query.exec();

			return 1;
		});
		//get all meshes
		CROW_ROUTE(App, "/api/meshes")
		([&DB]()
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
		([&DB]()
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
		([&DB]()
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
		//Load
		CROW_ROUTE(App, "/api/load/<string>")
		([&DB](const std::string& FileName)
		{
			if (!std::filesystem::exists(FileName))
			{
				return 1;
			}

			std::filesystem::path Path = FileName;
			
			if (Path.extension() == ".glsl")
			{
				AssetManager::LoadShader(Path.string().c_str());
			}
			else if (Path.extension() == ".fbx" || Path.extension() == ".obj" || Path.extension() == ".gtlf")
			{
				AssetManager::LoadScene(Path.string().c_str());
			}

			return 1;
		});
	}
}
