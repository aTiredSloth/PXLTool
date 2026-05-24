#ifndef EVALUATEDSCENE_HPP
#define EVALUATEDSCENE_HPP
#include "Core/Assets/AssetId.hpp"
#include <glm/mat4x4.hpp>
#include <vector>

struct EvaluatedSkeleton
{
	std::vector<glm::mat4> Matrices;
};

struct EvaluatedModel
{
	AssetID MeshId;
	AssetID ShaderId;
	EvaluatedSkeleton Skeleton;
	glm::mat4 Transformation;
};

struct EvaluatedScene
{
	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;
	std::vector<EvaluatedModel> Models;
};

#endif
