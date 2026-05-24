#ifndef SCENEDESCRIPTION_HPP
#define SCENEDESCRIPTION_HPP
#include "Core/Runtime/ModelInstance.hpp"
#include <glm/vec3.hpp>
#include <vector>


struct Camera
{
	static constexpr glm::vec3 Up = glm::vec3(0,0,1);
	static constexpr glm::vec3 Right = glm::vec3(1,0,0);
	static constexpr glm::vec3 Forward = glm::vec3(0,1,0);
	
	glm::vec3 Location{0,0,0};
	glm::qua<float> Rotation{1,0,0,0};

	bool bIsOrtho = false;
	int OrthoLeft = -64, OrthoRight = 64, OrthoTop = 64, OrthoBottom = -64;
	float Aspect = 16.f/9.f;
	float Near = 0.01f;
	float Far = 1000.0f;
	float FOV = 90;
};

struct SceneDescription
{
	float TimeStamp;
	Camera CameraData;
	std::vector<ModelInstance> Models;
};

#endif
