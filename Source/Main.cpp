#include "Renderer/OfflineRenderer.hpp"
#include "UI/UI.hpp"
#include <SQLiteCpp/Database.h>

int main(int argc, char* argv[])
{
	if (!OfflineRenderer::Initialize(RenderBackend::OpenGL))
	{
		return -1;
	}

	SQLite::Database DB("savedata.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
	
	auto Result = UI::Start(DB);
	
	OfflineRenderer::Shutdown();

	DB.backup("savedata.db3", SQLite::Database::BackupType::Save);
	
	return Result;
}
