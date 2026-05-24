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
	glm::mat4 InverseBindMatrix;
	std::vector<BoneWeight> Weights;
};

struct SkeletonAsset
{
	std::vector<Bone> Bones;
};

#endif
