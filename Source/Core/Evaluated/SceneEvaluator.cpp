#include "SceneEvaluator.hpp"
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Core/Assets/SkeletonAsset.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Runtime/ModelInstance.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>

std::pair<glm::mat4, glm::mat4> CalculateCameraProjection(const Camera& CameraData)
{
	glm::mat4 const ViewMatrix = glm::lookAt(CameraData.Location, CameraData.Location + (CameraData.Rotation * -Camera::Forward), Camera::Up);
	glm::mat4 const ProjectionMatrix = CameraData.bIsOrtho ? glm::ortho<float>
	(
		CameraData.OrthoLeft, 
		CameraData.OrthoRight, 
		CameraData.OrthoBottom,
		CameraData.OrthoTop, 
		CameraData.Near, 
		CameraData.Far
	) : glm::perspective(glm::radians(CameraData.FOV), CameraData.Aspect, CameraData.Near, CameraData.Far);

	return {ViewMatrix, ProjectionMatrix};
}

static glm::mat4 GetChannelMatrix(const AnimationAsset& Animation, const Bone& BoneChannel, float Time);

EvaluatedSkeleton EvaluateAnimation(const SkeletonAsset& Skeleton, const AnimationAsset& Animation, float Time)
{
	auto& Bones = Skeleton.Bones;
	
	EvaluatedSkeleton Evaluation;

	Evaluation.Matrices.resize(Bones.size());

	for (size_t i = 0; i < Bones.size(); ++i)
	{
		Evaluation.Matrices[i] = GetChannelMatrix(Animation, Bones[i], Time);
	}

	return Evaluation;
}

EvaluatedModel EvaluateModel(const ModelInstance& Model, float Time)
{
	EvaluatedModel Evaluation;
	Evaluation.MeshId = Model.MeshId;
	Evaluation.ShaderId = Model.ShaderId;
	Evaluation.TextureId = Model.TextureId;
	const Transform& ModelTransform = Model.Transformation;
	
	glm::mat4 const Translation = glm::translate(glm::mat4(1.0f), ModelTransform.Location);
	glm::mat4 const Scale = glm::scale(glm::mat4(1.0f), ModelTransform.Scale);
	glm::mat4 const Rotate = glm::mat4_cast(ModelTransform.Rotation);
	Evaluation.Transformation = Translation * Rotate * Scale;

	if (Model.AnimationId)
	{
		auto Mesh = AssetManager::GetMesh(Model.MeshId);

		if (!Mesh)
		{
			std::cout << "Failed to Evaluate Model " << Model.MeshId << " Mesh is not valid" << std::endl;
			return {};
		}

		auto Animation = AssetManager::GetAnimation(Model.AnimationId);

		if (!Animation)
		{
			std::cout << "Failed to Evaluate Mesh " << Mesh->Name << " Animation, it is not valid" << std::endl;
			return {};
		}
		
		Evaluation.Skeleton = EvaluateAnimation(Mesh->Skeleton, *Animation, Time);
	}
	
	return Evaluation;
}

EvaluatedScene EvaluateScene(const SceneDescription& Scene)
{
	EvaluatedScene Evaluation;

	//Calculate Matrices
	{
		auto const [ViewMatrix,ProjectionMatrix] = CalculateCameraProjection(Scene.CameraData);
		Evaluation.ViewMatrix = ViewMatrix;
		Evaluation.ProjectionMatrix = ProjectionMatrix;
	}

	Evaluation.Models.reserve(Scene.Models.size());
	//Evaluate Models
	for (auto& Model : Scene.Models)
	{
		Evaluation.Models.push_back(EvaluateModel(Model, Scene.TimeStamp));
	}
	
	return Evaluation;
}

template<typename T>
T GetKeyValue(float Time, AnimationAsset::KeyFrame<T> Channel)
{
	for (uint32_t i = 0; i < Channel.size(); ++i)
	{
		auto& Key = Channel[i];
		//Edges
		if (i == 0 && Time <= Key.first)
		{
			return Key.second;
			break;
		}
		else if (i == Channel.size() - 1 && Time >= Key.first)
		{
			return Key.second;
			break;
		}


		if (Time > Key.first)
		{
			continue;
		}
		
		auto& LastKey = Channel[i - 1];

		float const InterpValue = (Time - LastKey.first) / (Key.first - LastKey.first);

		return glm::mix(LastKey.second, Key.second, InterpValue);
	}

	return T();
}

glm::mat4 GetChannelMatrix(const AnimationAsset& Animation, const Bone& BoneChannel, float Time)
{
	auto& Channels = Animation.Channels;
	if (!Channels.contains(BoneChannel.Name))
	{
		return BoneChannel.InverseBindMatrix;
	}

	auto& ChannelRef = Channels.at(BoneChannel.Name);
	auto& [Rotation, Scale, Location] = ChannelRef;
	
	glm::quat Rot = GetKeyValue<glm::quat>(Time, Rotation);
	glm::vec3 Scaling = GetKeyValue<glm::vec3>(Time, Scale);
	glm::vec3 Pos = GetKeyValue<glm::vec3>(Time, Location);

	return glm::translate(glm::mat4(), Pos) * glm::mat4_cast(Rot) * glm::scale(glm::mat4(), Scaling);
}
