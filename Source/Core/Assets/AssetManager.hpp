#ifndef ASSETMANAGER_HPP
#define ASSETMANAGER_HPP
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetId.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include <optional>
#include <vector>

namespace AssetManager
{
	std::vector<AssetID> GetAllMeshIds();
	std::optional<MeshAsset> GetMesh(AssetID);

	std::vector<AssetID> GetAllAnimationIds();
	std::optional<AnimationAsset> GetAnimation(AssetID);

	std::vector<AssetID> GetAllShaderIds();
	std::optional<ShaderAsset> GetShader(AssetID);

	void LoadShader(const char* pFileName);
	void LoadScene(const char* pFileName);
}

#endif
