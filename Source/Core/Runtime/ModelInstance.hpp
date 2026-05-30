#ifndef MODELINSTANCE_HPP
#define MODELINSTANCE_HPP
#include "Core/Assets/AssetId.hpp"
#include <glm/detail/type_quat.hpp>

struct Transform
{
	glm::vec3 Location;
	glm::qua<float> Rotation;
	glm::vec3 Scale;
};

struct ModelInstance
{
	AssetID MeshId;
	AssetID AnimationId;
	AssetID ShaderId;

	AssetID TextureId;
	
	Transform Transformation;
};

#endif
