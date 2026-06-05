#include "AssetManager.hpp"
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetId.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include "Core/Assets/SkeletonAsset.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstring>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <unordered_map>
#include<assimp/quaternion.h>
#include<assimp/vector3.h>
#include<assimp/matrix4x4.h>
#include "Core/Assets/TextureAsset.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "glm/gtc/quaternion.hpp"
#include "stb_image.h"

namespace AssetManager
{
	void ImportMesh(const aiMesh* pMesh);
	SkeletonAsset ImportSkeleton(const aiMesh* pMesh);
	void ImportAnimation(const aiAnimation* pAnimation);
	void SetVertexWeights(const std::vector<Bone>& Bones, std::vector<Vertex>& Vertices);

	
	std::unordered_map<AssetID, MeshAsset> Meshes;
	std::unordered_map<AssetID, AnimationAsset> Animations;
	std::unordered_map<AssetID, ShaderAsset> Shaders;
	std::unordered_map<AssetID, TextureAsset> Textures;

	namespace GLMAssimp
	{
		
		inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& From)
		{
			glm::mat4 To;
			To[0][0] = From.a1; To[1][0] = From.a2; To[2][0] = From.a3; To[3][0] = From.a4;
			To[0][1] = From.b1; To[1][1] = From.b2; To[2][1] = From.b3; To[3][1] = From.b4;
			To[0][2] = From.c1; To[1][2] = From.c2; To[2][2] = From.c3; To[3][2] = From.c4;
			To[0][3] = From.d1; To[1][3] = From.d2; To[2][3] = From.d3; To[3][3] = From.d4;
			return To;
		}
		
		static inline glm::vec3 GetGLMVec(const aiVector3D& vec)
		{
			return glm::vec3(vec.x, vec.y, vec.z);
		}
		
		static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
		{
			return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
		}
	};

	inline AssetID GenerateId()
	{
		static std::mt19937_64 rng(std::random_device{}());
   	return rng();
	}


	std::vector<AssetID> GetAllMeshIds()
	{
		std::vector<AssetID> Array(Meshes.size(), 0);

		for (size_t i = 0; i < Meshes.size(); ++i)
		{
			Array[i] = std::next(Meshes.begin(), i)->first;
		}

		return Array;
	}
	std::optional<MeshAsset> GetMesh(AssetID Id)
	{
		if (Meshes.contains(Id))
		{
			return std::optional<MeshAsset>(Meshes[Id]);
		}

		return std::optional<MeshAsset>();
	}

	std::vector<AssetID> GetAllAnimationIds()
	{
		std::vector<AssetID> Array(Animations.size(), 0);

		for (size_t i = 0; i < Array.size(); ++i)
		{
			Array[i] = std::next(Animations.begin(), i)->first;
		}

		return Array;
	}
	std::optional<AnimationAsset> GetAnimation(AssetID Id)
	{
		if (Animations.contains(Id))
		{
			return std::optional<AnimationAsset>(Animations[Id]);
		}

		return std::optional<AnimationAsset>();
	}

	std::vector<AssetID> GetAllShaderIds()
	{
		std::vector<AssetID> Array(Shaders.size(), 0);

		for (size_t i = 0; i < Array.size(); ++i)
		{
			Array[i] = std::next(Shaders.begin(), i)->first;
		}

		return Array;
	}
	std::optional<ShaderAsset> GetShader(AssetID Id)
	{
		if (Shaders.contains(Id))
		{
			return std::optional<ShaderAsset>(Shaders[Id]);
		}

		return std::optional<ShaderAsset>();
	}

	void LoadShader(const std::string& Name, std::istream& Stream)
	{
		std::stringstream StringStream;
		StringStream << Stream.rdbuf();

		ShaderAsset NewShader;
		NewShader.Name = Name;
		NewShader.SourceCode = StringStream.str();
		Shaders.insert({GenerateId(), std::move(NewShader)});
	}

	void LoadScene(const std::string& Extension, std::istream& Stream)
	{
		std::vector<uint8_t> const Binary = std::vector<uint8_t>(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
		
		Assimp::Importer Importer;

		unsigned Flags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_PopulateArmatureData;
		const aiScene* const pImportedScene = Importer.ReadFileFromMemory(Binary.data(), Binary.size(), Flags, Extension.c_str());

		if (!pImportedScene)
		{
			return;
		}
		
		for (size_t i = 0; i < pImportedScene->mNumMeshes; ++i)
		{
			ImportMesh(pImportedScene->mMeshes[i]);
		}
		
		for (size_t i = 0; i < pImportedScene->mNumAnimations; ++i)
		{
			ImportAnimation(pImportedScene->mAnimations[i]);
		}
	}

	void ImportMesh(const aiMesh* pMesh)
	{
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
		
		//Import Vertices
		Vertices.reserve(pMesh->mNumVertices);
		for (size_t i = 0; i < pMesh->mNumVertices; ++i)
		{
			Vertex NewVertex{};
			
			NewVertex.Position.x = pMesh->mVertices[i].x;
			NewVertex.Position.y = pMesh->mVertices[i].y;
			NewVertex.Position.z = pMesh->mVertices[i].z;
			
			NewVertex.TexCoords.x = pMesh->mTextureCoords[0][i].x;
			NewVertex.TexCoords.y = pMesh->mTextureCoords[0][i].y;
			
			NewVertex.Normal.x = pMesh->mNormals[i].x;
			NewVertex.Normal.y = pMesh->mNormals[i].y;
			NewVertex.Normal.z = pMesh->mNormals[i].z;
			
			Vertices.push_back(NewVertex);
		}
		
		Indices.reserve(pMesh->mNumFaces);
		for (size_t i = 0; i < pMesh->mNumFaces; ++i)
		{
			Indices.push_back(pMesh->mFaces[i].mIndices[0]);
			Indices.push_back(pMesh->mFaces[i].mIndices[1]);
			Indices.push_back(pMesh->mFaces[i].mIndices[2]);
		}
		
		MeshAsset NewMesh;
		NewMesh.Name = pMesh->mName.C_Str();
		NewMesh.Vertices = std::move(Vertices);
		NewMesh.Indices = std::move(Indices);
		NewMesh.Skeleton = ImportSkeleton(pMesh);

		SetVertexWeights(NewMesh.Skeleton.Bones, NewMesh.Vertices);

		Meshes.insert({GenerateId(), std::move(NewMesh)});
	}

	void ImportAnimation(const aiAnimation* pAnimation)
	{
		std::unordered_map<std::string, AnimationAsset::BoneChannels> Channels;
		for (uint32_t i = 0; i < pAnimation->mNumChannels; ++i)
		{
			aiNodeAnim* const pChannel = pAnimation->mChannels[i];
			AnimationAsset::BoneChannels NewChannels;
			auto& [Rotation, Scale, Location] = NewChannels;
			
			Rotation.reserve(pChannel->mNumRotationKeys);
			for (uint32_t j = 0; j < pChannel->mNumRotationKeys; ++j)
			{
				auto& Key = pChannel->mRotationKeys[j];
				Rotation.push_back({Key.mTime, GLMAssimp::GetGLMQuat(Key.mValue)});
			}
			
			Scale.reserve(pChannel->mNumScalingKeys);
			for (uint32_t j = 0; j < pChannel->mNumScalingKeys; ++j)
			{
				auto& Key = pChannel->mScalingKeys[j];
				Scale.push_back({Key.mTime, GLMAssimp::GetGLMVec(Key.mValue)});
			}
			
			Location.reserve(pChannel->mNumPositionKeys);
			for (uint32_t j = 0; j < pChannel->mNumPositionKeys; ++j)
			{
				auto& Key = pChannel->mPositionKeys[j];
				Location.push_back({Key.mTime, GLMAssimp::GetGLMVec(Key.mValue)});
			}
			
			Channels.insert({pChannel->mNodeName.C_Str(), NewChannels});
		}
		
		AnimationAsset NewAnimation;
		NewAnimation.TicksPerSecond = pAnimation->mTicksPerSecond;
		NewAnimation.Length = pAnimation->mDuration;
		NewAnimation.Name = pAnimation->mName.C_Str();
		NewAnimation.Channels = std::move(Channels);
		Animations.insert({GenerateId(), std::move(NewAnimation)});
	}

	SkeletonAsset ImportSkeleton(const aiMesh* pMesh)
	{
		if (pMesh->mNumBones == 0)
		{
			return {};
		}

		std::unordered_map<std::string, int> BoneToIndex;
		std::unordered_map<std::string, std::string> BoneToParentName;
		SkeletonAsset NewAsset;
		auto& Bones = NewAsset.Bones;
		Bones.resize(pMesh->mNumBones);
		for (uint32_t i = 0; i < Bones.size(); ++i)
		{
			const aiBone* const pSkeletonBone = pMesh->mBones[i];
			Bone& NewBone = Bones[i];
			NewBone.Name = pSkeletonBone->mName.C_Str();
			NewBone.OffsetMatrix = GLMAssimp::ConvertMatrixToGLMFormat(pSkeletonBone->mOffsetMatrix);
			NewBone.LocalMatrix = GLMAssimp::ConvertMatrixToGLMFormat(pSkeletonBone->mNode->mTransformation);
			NewBone.Parent = -1;
			
			for (uint32_t j = 0; j < pSkeletonBone->mNumWeights; ++j)
			{
				const aiVertexWeight& Weight = pSkeletonBone->mWeights[j];
				
				BoneWeight NewWeight;
				NewWeight.Weight = Weight.mWeight;
				NewWeight.Id = Weight.mVertexId;
				
				NewBone.Weights.push_back(NewWeight);
			}

			BoneToIndex.insert({NewBone.Name, i});
			if (pSkeletonBone->mNode->mParent)
			{
				BoneToParentName.insert({NewBone.Name, pSkeletonBone->mNode->mParent->mName.C_Str()});
			}
		}

		//Map Parents
		for (auto i = 0; i < Bones.size(); ++i)
		{
			auto& Bone = Bones[i];
			auto& ParentName = BoneToParentName[Bone.Name];
			
			if (BoneToIndex.contains(ParentName))
			{
				Bone.Parent = BoneToIndex[ParentName];
				Bones[Bone.Parent].Children.push_back(i);
			}
		}
		
		return NewAsset;
	}

	void LoadTexture(std::istream& Stream)
	{
		std::vector<uint8_t> const Binary = std::vector<uint8_t>(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());

		int Width, Height, ChannelCount;
		
		uint8_t* const pImageBytes = stbi_load_from_memory(Binary.data(), Binary.size(), &Width, &Height, &ChannelCount, 4);
		int const Size = Width*Height*ChannelCount;
		
		std::shared_ptr<char[]> pImageBuffer = std::shared_ptr<char[]>(new char[Size]);

		std::memcpy(pImageBuffer.get(), pImageBytes, Size);
		TextureAsset NewTexture;
		NewTexture.Width = Width;
		NewTexture.Height = Height;
		NewTexture.ChannelCount = ChannelCount;
		NewTexture.pBuffer = pImageBuffer;
		stbi_image_free(pImageBytes);

		Textures.insert({GenerateId(), NewTexture});
	}

	std::vector<AssetID> GetAllTextureIds()
	{
		std::vector<AssetID> Array(Textures.size(), 0);

		for (size_t i = 0; i < Array.size(); ++i)
		{
			Array[i] = std::next(Textures.begin(), i)->first;
		}

		return Array;
	}
	std::optional<TextureAsset> GetTexture(AssetID Id)
	{
		if (Textures.contains(Id))
		{
			return std::optional<TextureAsset>(Textures[Id]);
		}

		return std::optional<TextureAsset>();
	}

	void SetVertexWeights(const std::vector<Bone>& Bones, std::vector<Vertex>& Vertices)
	{
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			auto& Bone = Bones[i];
			
			for (auto& Weight : Bone.Weights)
			{
				auto& Vertex = Vertices[Weight.Id];

				auto Count = 0u;
				for (auto j = 0; j < Vertex::MaxBoneWeights; ++j)
				{
					auto& Id = Vertex.BoneIds[j];

					if (Id != -1)
					{
						Count += 1;
					}
				}

				if (Count == Vertex::MaxBoneWeights)
				{
					int WeakestSlot = 0;
					for (auto j = 1; j < Vertex::MaxBoneWeights; ++j)
					{
						if (Vertex.BoneWeights[j] < Vertex.BoneWeights[WeakestSlot])
						{
							WeakestSlot = j;
						}
					}

					if (Vertex.BoneWeights[WeakestSlot] < Weight.Weight)
					{
						Vertex.BoneIds[WeakestSlot] = i;
						Vertex.BoneWeights[WeakestSlot] = Weight.Weight;
					}
				}
				else 
				{
					auto& Id = Vertex.BoneIds[Count];
					auto& VertexWeight = Vertex.BoneWeights[Count];
					Id = i;
					VertexWeight = Weight.Weight;
				}
			}
		}

		//Normalize Weights
		for (auto& Vert : Vertices)
		{
			auto& W = *(glm::vec4*)Vert.BoneWeights;
			float Sum = W.x + W.y + W.z + W.w;
			if (Sum > 0.0f) W /= Sum;
		}
	}
}
