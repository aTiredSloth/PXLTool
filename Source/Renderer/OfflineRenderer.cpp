#include "OfflineRenderer.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Evaluated/SceneEvaluator.hpp"
#include "Renderer/Backends/GL/OpenGLBackend.hpp"
#include "Renderer/Backends/RenderBackend.hpp"
#include <future>
#include <iostream>
#include <memory>
#include <GLFW/glfw3.h>

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

		{
			EvaluatedScene Evaluation = EvaluateScene(Scene);
			
			Promise.set_value(pBackend->Render(Evaluation, Settings));
		}
		
		return std::move(Promise);
	}
}
