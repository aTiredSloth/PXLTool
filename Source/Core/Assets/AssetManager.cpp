#include "AssetManager.hpp"
#include "Core/Assets/AnimationAsset.hpp"
#include "Core/Assets/AssetId.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include "Core/Assets/SkeletonAsset.hpp"
#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Statement.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <cstdint>
#include <cstring>
#include <glm/detail/type_quat.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <sstream>
#include <string>
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

	
	//std::unordered_map<AssetID, MeshAsset> Meshes;
	//std::unordered_map<AssetID, AnimationAsset> Animations;
	//std::unordered_map<AssetID, ShaderAsset> Shaders;
	//std::unordered_map<AssetID, TextureAsset> Textures;
	std::unique_ptr<SQLite::Database> pDatabase;
	
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

	inline uint32_t GenerateId()
	{
		static std::mt19937 rng(std::random_device{}());
   	return  rng();
	}


	bool Startup()
	{
		pDatabase = std::make_unique<SQLite::Database>("savedata.db3", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

		pDatabase->exec("CREATE TABLE IF NOT EXISTS MESHES ( Id INTEGER PRIMARY KEY, Name TEXT, Vertices BLOB, Indices BLOB, BoneCount INTEGER)");
		pDatabase->exec("CREATE TABLE IF NOT EXISTS BONES ( MeshId INTEGER, BoneIndex INTEGER, Name TEXT, LocalMatrix BLOB, OffsetMatrix BLOB, Parent INTEGER, Weights BLOB, Children BLOB)");
		
		pDatabase->exec("CREATE TABLE IF NOT EXISTS ANIMATIONS ( Id INTEGER PRIMARY KEY, Name TEXT, Length REAL, TicksPerSecond REAL, ChannelCount INTEGER)");
		pDatabase->exec("CREATE TABLE IF NOT EXISTS ANIMATION_CHANNELS (AnimationId INTEGER, ChannelIndex INTEGER, BoneName TEXT, Rotations BLOB, Positions BLOB, Scales BLOB)");
		
		pDatabase->exec("CREATE TABLE IF NOT EXISTS TEXTURES ( Id INTEGER PRIMARY KEY, Name TEXT, Width INTEGER, Height INTEGER, ChannelCount INTEGER, Blob BLOB)");
		pDatabase->exec("CREATE TABLE IF NOT EXISTS SHADERS ( Id INTEGER, Name TEXT, SourceCode TEXT)");
		
		return pDatabase != nullptr;
	}

	void Shutdown()
	{
		pDatabase->backup("savedata.db3", SQLite::Database::BackupType::Save);
		pDatabase.reset();
	}
	
	SQLite::Database& GetDatabase()
	{
		return *pDatabase;
	}

	template<typename Func>
	void IterateTable(const char* pTableName, Func Function)
	{
		std::string const Command = std::string("SELECT * FROM ") + pTableName;
		
		SQLite::Statement Query(GetDatabase(), Command);

		while (Query.executeStep())
		{
			Function(Query);
		}
	}

	std::vector<AssetID> GetIdsFromTable(const char* pTableName)
	{
		std::vector<AssetID> Array;

		IterateTable(pTableName, [&Array](SQLite::Statement& Query)
			{
				uint32_t const Id = Query.getColumn(0);
        		const char* pName = Query.getColumn(1);
           
           	Array.push_back(AssetID{pName, Id});
			}
		);
		
		return Array;
	}
	
	std::vector<AssetID> GetAllMeshIds()
	{
		return GetIdsFromTable("MESHES");
	}
	
	std::optional<MeshAsset> GetMesh(AssetID Id)
	{
		SQLite::Statement Query(GetDatabase(), "SELECT * FROM MESHES WHERE Id = ?");

		Query.bind(1, static_cast<uint32_t>(Id.Id));

		std::optional<MeshAsset> Data;
		while (Query.executeStep())
		{
			const char* pName = Query.getColumn(1);
			const Vertex* const pVertices = reinterpret_cast<const Vertex*>(Query.getColumn(2).getBlob());
			uint32_t const VertexCount = Query.getColumn(2).getBytes()/sizeof(Vertex);
			const uint32_t* pIndices = reinterpret_cast<const uint32_t*>(Query.getColumn(3).getBlob());
			uint32_t const IndexCount = Query.getColumn(3).getBytes()/sizeof(uint32_t);
			uint32_t const BoneCount = Query.getColumn(4);
			std::vector<Bone> Bones;
			Bones.resize(BoneCount);
			
			for (uint32_t i = 0; i < BoneCount; ++i)
			{
				SQLite::Statement BoneQuery(GetDatabase(), "SELECT * FROM BONES WHERE MeshId = ? AND BoneIndex = ?");

				BoneQuery.bind(1, static_cast<uint32_t>(Id.Id));
				BoneQuery.bind(2, static_cast<uint32_t>(i));

				
				while (BoneQuery.executeStep())
				{
					std::string const Name = BoneQuery.getColumn(2);
					glm::mat4 const LocalMatrix = *reinterpret_cast<const glm::mat4*>(BoneQuery.getColumn(3).getBlob());
					glm::mat4 const OffsetMatrix = *reinterpret_cast<const glm::mat4*>(BoneQuery.getColumn(4).getBlob());
					int const Parent = BoneQuery.getColumn(5);
					auto WeightColumn = BoneQuery.getColumn(6);
					const BoneWeight* const pWeights = reinterpret_cast<const BoneWeight*>(WeightColumn.getBlob());
					std::vector<BoneWeight> const Weights = pWeights ? 
						std::vector<BoneWeight>(pWeights, pWeights + (WeightColumn.getBytes()/sizeof(BoneWeight))) 
						: std::vector<BoneWeight>();
					auto ChildrenColumn = BoneQuery.getColumn(7);
					const int* const pChildren = reinterpret_cast<const int*>(ChildrenColumn.getBlob());
					std::vector<int> const Children = pChildren ? 
						std::vector<int>(pChildren, pChildren + (ChildrenColumn.getBytes()/sizeof(int))) 
						: std::vector<int>();

					Bones[i] = Bone{Name, LocalMatrix, OffsetMatrix, Parent, std::move(Weights), std::move(Children)};
				}
			}
			
			Data = MeshAsset
			{
				pName,
				std::vector<Vertex>(pVertices, pVertices + VertexCount),
				std::vector<uint32_t>(pIndices, pIndices + IndexCount),
				SkeletonAsset{std::move(Bones)}
			};
		}

		return Data;
	}

	std::vector<AssetID> GetAllAnimationIds()
	{
		return GetIdsFromTable("ANIMATIONS");
	}
	
	std::optional<AnimationAsset> GetAnimation(AssetID Id)
	{
		SQLite::Statement Query(GetDatabase(), "SELECT * FROM ANIMATIONS WHERE Id = ?");

		Query.bind(1, static_cast<uint32_t>(Id.Id));

		std::optional<AnimationAsset> Data;
		while (Query.executeStep())
		{
			std::unordered_map<std::string, AnimationAsset::BoneChannels> Channels;
			
			const char* const pName = Query.getColumn(1);
			double const Length = Query.getColumn(2);
			double const TicksPerSecond = Query.getColumn(3);
			uint32_t const ChannelCount = Query.getColumn(4);

			for (auto i = 0; i < ChannelCount; ++i)
			{
				SQLite::Statement ChannelQuery(GetDatabase(), "SELECT * FROM ANIMATION_CHANNELS WHERE AnimationId = ? AND ChannelIndex = ?");

				ChannelQuery.bind(1, static_cast<uint32_t>(Id.Id));
				ChannelQuery.bind(2, static_cast<uint32_t>(i));

				
				while (ChannelQuery.executeStep())
				{
					const char* const pBoneName = ChannelQuery.getColumn(2);

					auto const RotationColumn = ChannelQuery.getColumn(3);
					const std::pair<float, glm::qua<float>>* const pRotations = reinterpret_cast<const std::pair<float, glm::qua<float>>*>(RotationColumn.getBlob());
					uint32_t const RotationCount = RotationColumn.getBytes()/sizeof(*pRotations);
					auto const PositionColumn = ChannelQuery.getColumn(4);
					const std::pair<float, glm::vec3>* const pPositions = reinterpret_cast<const  std::pair<float, glm::vec3>*>(PositionColumn.getBlob());
					uint32_t const PositionCount = PositionColumn.getBytes()/sizeof(*pPositions);
					auto const ScaleColumn = ChannelQuery.getColumn(5);
					const std::pair<float, glm::vec3>* const pScales = reinterpret_cast<const  std::pair<float, glm::vec3>*>(ScaleColumn.getBlob());
					uint32_t const ScaleCount = ScaleColumn.getBytes()/sizeof(*pScales);

					Channels.insert(
						{
							pBoneName, 
							std::tuple<AnimationAsset::KeyFrame<glm::qua<float>> , AnimationAsset::KeyFrame<glm::vec3>, AnimationAsset::KeyFrame<glm::vec3>>
							{
								std::vector<std::pair<float, glm::quat>>(pRotations, pRotations + RotationCount),
								std::vector<std::pair<float, glm::vec3>>(pPositions, pPositions + PositionCount),
								std::vector<std::pair<float, glm::vec3>>(pScales, pScales + ScaleCount)
							}
						}
					);
					
				}
			}
			
			Data = AnimationAsset
			{
				static_cast<float>(Length),
				static_cast<float>(TicksPerSecond),
				pName,
				std::move(Channels)
			};
		}

		return Data;
	}

	std::vector<AssetID> GetAllShaderIds()
	{
		return GetIdsFromTable("SHADERS");
	}
	
	std::optional<ShaderAsset> GetShader(AssetID Id)
	{
		SQLite::Statement Query(GetDatabase(), "SELECT * FROM SHADERS WHERE Id = ?");

		Query.bind(1, static_cast<uint32_t>(Id.Id));

		std::optional<ShaderAsset> Data;
		while (Query.executeStep())
		{
			const char* const pSourceCode = Query.getColumn(2);
			
			Data = ShaderAsset
			{
				pSourceCode
			};
		}

		return Data;
	}

	void LoadShader(const std::string& Name, std::istream& Stream)
	{
		std::stringstream StringStream;
		StringStream << Stream.rdbuf();

		SQLite::Statement Insert(GetDatabase(), "INSERT INTO SHADERS (Id, Name, SourceCode) VALUES (?,?,?)");

		Insert.bind(1, GenerateId());
		Insert.bind(2, Name);
		Insert.bind(3, StringStream.str());

		Insert.exec();
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

			if (pMesh->mTextureCoords[0])
			{
				NewVertex.TexCoords.x = pMesh->mTextureCoords[0][i].x;
				NewVertex.TexCoords.y = pMesh->mTextureCoords[0][i].y;
			}
			
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
		
		auto Bones = ImportSkeleton(pMesh).Bones;
		
		SetVertexWeights(Bones, Vertices);

		SQLite::Statement InsertMesh(GetDatabase(), "INSERT INTO MESHES (Id, Name, Vertices, Indices, BoneCount) VALUES (?,?,?,?,?)");

		auto MeshId = GenerateId();
		InsertMesh.bind(1, MeshId);
		InsertMesh.bind(2, pMesh->mName.C_Str());
		InsertMesh.bind(3, Vertices.data(), sizeof(Vertex)*Vertices.size());
		InsertMesh.bind(4, Indices.data(), sizeof(uint32_t)*Indices.size());
		InsertMesh.bind(5, static_cast<uint32_t>(Bones.size()));

		InsertMesh.exec();

		for (auto i = 0; i < Bones.size(); ++i)
		{
			auto& Bone = Bones[i];
			
			SQLite::Statement InsertBone(GetDatabase(), "INSERT INTO BONES "
				"(MeshId, BoneIndex, Name, LocalMatrix, OffsetMatrix, Parent, Weights, Children) VALUES (?,?,?,?,?,?,?,?)");

			
			InsertBone.bind(1, MeshId);
			InsertBone.bind(2, i);
			InsertBone.bind(3, Bone.Name);
			InsertBone.bind(4, &Bone.LocalMatrix, sizeof(glm::mat4));
			InsertBone.bind(5, &Bone.OffsetMatrix, sizeof(glm::mat4));
			InsertBone.bind(6, Bone.Parent);
			InsertBone.bind(7, Bone.Weights.data(), sizeof(BoneWeight)*Bone.Weights.size());
			InsertBone.bind(8, Bone.Children.data(), sizeof(int)*Bone.Children.size());

			InsertBone.exec();
		}
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
		
		uint32_t const ChannelCount = static_cast<uint32_t>(Channels.size());
		SQLite::Statement InsertAnimation(GetDatabase(), "INSERT INTO ANIMATIONS (Id, Name, Length, TicksPerSecond, ChannelCount) VALUES (?,?,?,?,?)");

		auto const AnimId = GenerateId();
		InsertAnimation.bind(1, AnimId);
		InsertAnimation.bind(2, pAnimation->mName.C_Str());
		InsertAnimation.bind(3, pAnimation->mDuration);
		InsertAnimation.bind(4, pAnimation->mTicksPerSecond);
		InsertAnimation.bind(5, ChannelCount);

		InsertAnimation.exec();
		
		for (uint32_t i = 0; i < ChannelCount; ++i)
		{
			auto& [Name, Data] = *std::next(Channels.begin(), i);
			auto& [Rotations, Scales, Positions] = Data;
			
			SQLite::Statement InsertChannel(GetDatabase(), "INSERT INTO ANIMATION_CHANNELS" 
				" (AnimationId, ChannelIndex, BoneName, Rotations, Positions, Scales) VALUES (?,?,?,?,?,?)");

			InsertChannel.bind(1, AnimId);
			InsertChannel.bind(2, i);

			InsertChannel.bind(3, Name);
			InsertChannel.bind(4, Rotations.data(), Rotations.size()*sizeof(glm::quat));
			InsertChannel.bind(5, Positions.data(), Positions.size()*sizeof(glm::vec3));
			InsertChannel.bind(6, Scales.data(), Scales.size()*sizeof(glm::vec3));

			InsertChannel.exec();
		}
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

	void LoadTexture(const std::string& Name, std::istream& Stream)
	{
		std::vector<uint8_t> const Binary = std::vector<uint8_t>(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());

		int Width, Height, ChannelCount;
		
		uint8_t* const pImageBytes = stbi_load_from_memory(Binary.data(), Binary.size(), &Width, &Height, &ChannelCount, 0);
		int const Size = Width*Height*ChannelCount;
		
		SQLite::Statement Insert(GetDatabase(), "INSERT INTO TEXTURES (Id, Name, Width, Height, ChannelCount, Blob) VALUES (?,?,?,?,?,?)");

		Insert.bind(1, GenerateId());
		Insert.bind(2, Name);
		Insert.bind(3, Width);
		Insert.bind(4, Height);
		Insert.bind(5, ChannelCount);
		Insert.bind(6, pImageBytes, Size);

		Insert.exec();

		stbi_image_free(pImageBytes);
		
	}

	std::vector<AssetID> GetAllTextureIds()
	{
		return GetIdsFromTable("TEXTURES");
	}
	
	std::optional<TextureAsset> GetTexture(AssetID Id)
	{
		SQLite::Statement Query(GetDatabase(), "SELECT * FROM TEXTURES WHERE Id = ?");

		Query.bind(1, static_cast<uint32_t>(Id.Id));

		std::optional<TextureAsset> Data;
		while (Query.executeStep())
		{
			int const Width = Query.getColumn(2);
			int const Height = Query.getColumn(3);
			unsigned const ChannelCount = Query.getColumn(4);
			unsigned const BufferSize = Width*Height*ChannelCount;
			std::shared_ptr<char[]> pBuffer = std::shared_ptr<char[]>(new char[BufferSize]);
			SQLite::Column const BufferColumn = Query.getColumn(5);
			std::memcpy(pBuffer.get(), BufferColumn.getBlob(), BufferColumn.getBytes());
			
			Data = TextureAsset
			{
				Width,
				Height,
				ChannelCount,
				pBuffer
			};
		}

		return Data;
	}

	void SetVertexWeights(const std::vector<Bone>& Bones, std::vector<Vertex>& Vertices)
	{
		for (size_t i = 0; i < Bones.size(); ++i)
		{
			auto& Bone = Bones[i];
			
			for (auto& Weight : Bone.Weights)
			{
				auto& Vert = Vertices[Weight.Id];

				auto Count = 0u;
				for (auto j = 0; j < Vertex::MaxBoneWeights; ++j)
				{
					auto& Id = Vert.BoneIds[j];

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
						if (Vert.BoneWeights[j] < Vert.BoneWeights[WeakestSlot])
						{
							WeakestSlot = j;
						}
					}

					if (Vert.BoneWeights[WeakestSlot] < Weight.Weight)
					{
						Vert.BoneIds[WeakestSlot] = i;
						Vert.BoneWeights[WeakestSlot] = Weight.Weight;
					}
				}
				else 
				{
					auto& Id = Vert.BoneIds[Count];
					auto& VertexWeight = Vert.BoneWeights[Count];
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
