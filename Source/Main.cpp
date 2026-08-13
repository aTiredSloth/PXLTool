#include "Core/Assets/AssetManager.hpp"
#include "Renderer/OfflineRenderer.hpp"
#include "UI/UI.hpp"
#include <SQLiteCpp/Database.h>

int main(int argc, char* argv[])
{
	if (!OfflineRenderer::Initialize(RenderBackend::OpenGL))
	{
		return -1;
	}

	if (!AssetManager::Startup())
	{
		return -1;
	}
	
	auto Result = UI::Start();
	
	OfflineRenderer::Shutdown();

	AssetManager::Shutdown();
	
	return Result;
}
