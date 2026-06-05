#include "OfflineRenderer.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Evaluated/SceneEvaluator.hpp"
#include "Renderer/Backends/GL/OpenGLBackend.hpp"
#include "Renderer/Backends/RenderBackend.hpp"
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

	ImageResource DispatchRender(const SceneDescription& Scene, const RenderSettings& Settings)
	{
		if (Settings.Width == 0 || Settings.Height == 0)
		{
			return ImageResource(nullptr, nullptr, nullptr);
		}
		
		EvaluatedScene Evaluation = EvaluateScene(Scene);
		
		return pBackend->Render(Evaluation, Settings);
	}
}
