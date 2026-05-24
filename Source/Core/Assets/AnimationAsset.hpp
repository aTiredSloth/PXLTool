#ifndef ANIMATIONASSET_HPP
#define ANIMATIONASSET_HPP
#include <string>
#include <unordered_map>
#include <vector>
#include "glm/detail/type_quat.hpp"

struct AnimationAsset
{
	template<typename T>
	using KeyFrame = std::vector<std::pair<float, T>>;
	using BoneChannels = std::tuple<KeyFrame<glm::qua<float>> , KeyFrame<glm::vec3>, KeyFrame<glm::vec3>>;

	float Length;
	float FramesPerSecond;
	std::string Name;
	std::unordered_map<std::string, BoneChannels> Channels;
};

#endif
