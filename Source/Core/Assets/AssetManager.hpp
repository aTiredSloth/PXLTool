#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetId.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include "Core/Assets/TextureAsset.hpp"
#include <SQLiteCpp/Database.h>
#include <optional>
#include <vector>

namespace AssetManager
{
	bool Startup();
	void Shutdown();
	SQLite::Database& GetDatabase();
	
	std::vector<AssetID> GetAllMeshIds();
	std::optional<MeshAsset> GetMesh(AssetID);

	std::vector<AssetID> GetAllAnimationIds();
	std::optional<AnimationAsset> GetAnimation(AssetID);

	std::vector<AssetID> GetAllShaderIds();
	std::optional<ShaderAsset> GetShader(AssetID);

	std::vector<AssetID> GetAllTextureIds();
	std::optional<TextureAsset> GetTexture(AssetID);
	
	void LoadShader(const std::string& Name, std::istream& Stream);
	void LoadScene(const std::string& Extension, std::istream& Stream);
	void LoadTexture(const std::string& Name, std::istream& Stream);
}

#endif
