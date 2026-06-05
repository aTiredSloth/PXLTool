#include "SceneEvaluator.hpp"
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Core/Assets/SkeletonAsset.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Runtime/ModelInstance.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <optional>
#include <vector>

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
static void CalcFinalMatrix(const AnimationAsset& Animation, float Time, const glm::mat4& ParentTransform, const Bone& BoneChannel, std::vector<glm::mat4>& FinalMatrices, const std::vector<Bone>& Bones, size_t Index);

EvaluatedSkeleton EvaluateAnimation(const SkeletonAsset& Skeleton, const AnimationAsset& Animation, float Time)
{
	auto& Bones = Skeleton.Bones;

	if (Bones.empty())
	{
		return {};
	}
	
	EvaluatedSkeleton Evaluation;

	Evaluation.Matrices.resize(Bones.size());
	CalcFinalMatrix(Animation, Time * Animation.TicksPerSecond, glm::mat4(1.0), Bones.front(), Evaluation.Matrices, Bones, 0);

	return Evaluation;
}

std::optional<EvaluatedModel> EvaluateModel(const ModelInstance& Model, float Time)
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
			Evaluation.Skeleton = {};
			return Evaluation;
		}
		
		Evaluation.Skeleton = EvaluateAnimation(Mesh->Skeleton, *Animation, Time);
	}
	
	return Evaluation;
}

EvaluatedScene EvaluateScene(const SceneDescription& Scene)
{
	EvaluatedScene Evaluation;
	Evaluation.PostShaders = Scene.PostShaders;

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
		auto Eval = EvaluateModel(Model, Scene.TimeStamp);

		if (Eval)
		{
			Evaluation.Models.push_back(*Eval);
		}
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

void CalcFinalMatrix(const AnimationAsset& Animation, float Time, const glm::mat4& ParentTransform, const Bone& BoneChannel, std::vector<glm::mat4>& FinalMatrices, const std::vector<Bone>& Bones, size_t Index)
{
	glm::mat4 const GlobalTransform = ParentTransform * GetChannelMatrix(Animation, BoneChannel, Time);
	FinalMatrices[Index] = GlobalTransform * BoneChannel.OffsetMatrix;

	for (auto& Child : BoneChannel.Children)
	{
		CalcFinalMatrix(Animation, Time, GlobalTransform, Bones[Child], FinalMatrices, Bones, Child);
	}
}

glm::mat4 GetChannelMatrix(const AnimationAsset& Animation, const Bone& BoneChannel, float Time)
{
	auto& Channels = Animation.Channels;
	if (!Channels.contains(BoneChannel.Name))
	{
		return glm::mat4(1.0);
	}

	auto& ChannelRef = Channels.at(BoneChannel.Name);
	auto& [Rotation, Scale, Location] = ChannelRef;
	
	glm::quat Rot = GetKeyValue<glm::quat>(Time, Rotation);
	glm::vec3 Scaling = GetKeyValue<glm::vec3>(Time, Scale);
	glm::vec3 Pos = GetKeyValue<glm::vec3>(Time, Location);

	return glm::translate(glm::mat4(1.0f), Pos) * glm::mat4_cast(Rot) * glm::scale(glm::mat4(1.0f), Scaling);
}
