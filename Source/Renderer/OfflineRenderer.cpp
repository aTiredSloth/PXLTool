#include "OfflineRenderer.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Evaluated/SceneEvaluator.hpp"
#include "Renderer/Backends/GL/OpenGLBackend.hpp"
#include "Renderer/Backends/RenderBackend.hpp"
#include <future>
#include <iostream>
#include <memory>
#include <GLFW/glfw3.h>
#include <mutex>

namespace OfflineRenderer
{
	std::unique_ptr<IRenderBackend> pBackend;
	
	bool Initialize(RenderBackend Backend)
	{
		if (!glfwInit())
		{
			const char* pError = nullptr;
			glfwGetError(&pError);
			std::cout << "Failed to Initalize GLFW: " << pError << std::endl;
			return false;
		}

		glfwSetErrorCallback([](int error, const char* description)
			{
				fprintf(stderr, "GLFW error %d: %s\n", error, description);
			}
		);
		switch (Backend)
		{
      case RenderBackend::OpenGL:
      	pBackend = std::make_unique<OpenGLBackend>();
      	break;
      default:
       	return false;
        break;
      }

      if (!pBackend)
      {
      	return false;
      }
      
      return pBackend->Startup();
   }
   
	void Shutdown()
	{
		if (pBackend)
		{
			pBackend->Shutdown();
		}
		glfwTerminate();
	}

	std::promise<ImageResource> DispatchRender(const SceneDescription& Scene, const RenderSettings& Settings)
	{
		std::promise<ImageResource> Promise;
		if (Settings.Width == 0 || Settings.Height == 0)
		{
			Promise.set_value(ImageResource(nullptr, nullptr, nullptr));
			return Promise;
		}
		
		std::thread NewThread([&Promise, &Scene, &Settings]()
		{
			EvaluatedScene Evaluation = EvaluateScene(Scene);
			
			Promise.set_value(pBackend->Render(Evaluation, Settings));
		});
		
		NewThread.join();
		return Promise;
	}
}
