#ifndef SKELETONASSET_HPP
#define SKELETONASSET_HPP
#include <glm/mat4x4.hpp>
#include <string>
#include <vector>

struct BoneWeight
{
	bool operator<(const BoneWeight& Other) const
	{
		return Weight < Other.Weight;
	}
	bool operator>(const BoneWeight& Other) const
	{
		return Weight > Other.Weight;
	}
	int Id;
	float Weight;
};

struct Bone
{
	std::string Name;
	glm::mat4 LocalMatrix; //Offset from parent
	glm::mat4 OffsetMatrix; //Mesh to Bone/Bind Space
	int Parent;
	std::vector<BoneWeight> Weights;
	std::vector<int> Children;
};

struct SkeletonAsset
{
	std::vector<Bone> Bones;
};

#endif
