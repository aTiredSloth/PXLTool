#ifndef MESHASSET_HPP
#define MESHASSET_HPP
#include "Core/Assets/SkeletonAsset.hpp"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>


//Vertex Buffer Binding (Array of Items, where to bind, Distance between items)
struct VertexInputDesc
{
	/// <summary>
	/// Buffer to look in for data
	/// </summary>
	unsigned Binding;
	/// <summary>
	/// Distance Between each Item
	/// </summary>
	uint32_t Stride;
};


enum class EVertexAttributeFormatType
{
	Float,
	Vector2D,
	Vector3D,
	Vector4D,
	IVector2D,
	IVector3D,
	IVector4D
};

//Vertex Attribute Description (Single variable Data)
struct VertexAttributeDesc
{
	/// <summary>
	/// Buffer it's located in
	/// </summary>
	uint32_t Binding;
	/// <summary>
	/// Layout Location
	/// </summary>
	uint32_t Location;
	/// <summary>
	/// Format of Data
	/// </summary>
	EVertexAttributeFormatType Format;
	/// <summary>
	/// Offset in Each Item
	/// </summary>
	uint32_t Offset;
};

struct Vertex
{
	static constexpr auto MaxBoneWeights = 4;
	static const VertexInputDesc* GetBindings();
	static unsigned GetBindingCount();
	static const VertexAttributeDesc* GetAttributes();
	static unsigned GetAttributeCount();
	
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec4 Color;

	int BoneIds[MaxBoneWeights] = {-1, -1, -1, -1};
	float BoneWeights[MaxBoneWeights] = {0.0f, 0.0f, 0.0f, 0.0f};
};

struct MeshAsset
{
	std::string Name;
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;
	SkeletonAsset Skeleton;
};

#endif
