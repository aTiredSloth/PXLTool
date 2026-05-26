#include "UI.hpp"
#include "crow/app.h"
#include <Crow/include/crow.h>
#include <saucer/smartview.hpp>
#include <string>

constexpr auto const Port = 4081;
std::future<void> Server;
crow::SimpleApp App;
namespace UI
{

coco::stray start(saucer::application* app)
{
	auto window = saucer::window::create(app).value();
	auto webview = saucer::smartview::create({ .window = window });

	window->set_title("PXLTool");

	webview->set_url(std::string("http://localhost:") + std::to_string(App.port()));

	window->show();

	co_await app->finish();
}

int Start()
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
	Server = App.port(Port).run_async();
	auto Result = saucer::application::create({ .id = "PXLTool" })->run(start);
	App.stop();

	return Result;
}
}
