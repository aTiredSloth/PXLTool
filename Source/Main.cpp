#include "Renderer/OfflineRenderer.hpp"
#include "UI/UI.hpp"

int main(int argc, char* argv[])
{
	if (!OfflineRenderer::Initialize(RenderBackend::OpenGL))
	{
		return -1;
	}

	auto Result = UI::Start();
	
	OfflineRenderer::Shutdown();
	
	return Result;
}
