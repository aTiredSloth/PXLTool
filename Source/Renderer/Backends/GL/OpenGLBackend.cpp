#include "OpenGLBackend.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Core/Runtime/ModelInstance.hpp"
#include "Renderer/Backends/GL/GLBuffer.hpp"
#include "Renderer/Backends/RenderBackend.hpp"
#include "Renderer/Backends/RenderSettings.hpp"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <utility>
#include <memory>
#include <unordered_map>
#include <glm/fwd.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _DEBUG
	#if defined(_MSC_VER)
    	#define DEBUG_BREAK() __debugbreak()
	#else
    	#define DEBUG_BREAK() __builtin_trap()
	#endif
#define GL_CHECK(x) \
    x; \
    { \
        GLenum err = glGetError(); \
        if (err != GL_NO_ERROR) \
        { \
            fprintf(stderr, "GL error %s in %s at %s:%d\n", GLErrorString(err), #x, __FILE__, __LINE__); \
            DEBUG_BREAK(); \
        } \
    }
#else
#define GL_CHECK(x) x
#endif

static inline GLenum GetFormatType(EVertexAttributeFormatType Format);
static inline GLsizei GetFormatSize(EVertexAttributeFormatType Format);
static inline bool ValidateShader(GLuint shader, const char* label);
static inline bool ValidateProgram(GLuint program);
static inline const char* GLErrorString(GLenum err);

class OpenGLBackend::Pimpl
{
	struct VertexBuffer
	{
		GLuint VAO;
		GLuint VBO;
		GLsizei Size;
	};
	//Vertex and Index
	using MeshBuffers = std::pair<VertexBuffer, GLBuffer>;

public:
	Pimpl() = default;	
	~Pimpl() = default;

	
	bool Startup()
	{
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		pWindow = glfwCreateWindow(32, 32, "OpenGLBackend", 0, 0);
		glfwMakeContextCurrent(pWindow);

		if (!pWindow)
		{
			std::cout << "Failed to Create GLFW Window" << std::endl;
			return false;
		}

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to Load Glad" << std::endl;
			return false;
		}
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glfwMakeContextCurrent(nullptr);
		
		return true;
	}
	RenderResult Render(const EvaluatedScene& Scene, const RenderSettings& Settings)
	{
		RenderResult Result{nullptr, nullptr, nullptr};
		
		RenderMutex.lock();
		GLFWwindow* pPrevContext = glfwGetCurrentContext();
		glfwMakeContextCurrent(pWindow);
		
		std::vector<std::tuple<MeshBuffers, GLuint, GLuint, GLBuffer>> Meshes;

		std::vector<GLuint> PostShaders;
		PostShaders.reserve(Scene.PostShaders.size());
		for (auto& PostShader : Scene.PostShaders)
		{
			PostShaders.push_back(UploadPostShader(PostShader));
		}
		
		Meshes.reserve(Scene.Models.size());
		for (auto& Model : Scene.Models)
		{
			Meshes.push_back
			(
			   {
					UploadMesh(Model.MeshId),
			     	UploadShader(Model.ShaderId),
					UploadTexture(Model.TextureId),
				   AcquireAnimationBuffer(Model.Skeleton.Matrices)
				}
			);

			auto& [Mesh, Shader, Texture, AnimBuffer] = Meshes.back();
			if (!Shader)
			{
				ReleaseDynamicBuffer(AnimBuffer);
				Meshes.pop_back();
				continue;
			}
		}

		auto const [FrameBuffer, ColorTexture, NormalTexture, DepthTexture] = AcquireFrameBuffer(Settings);
		
		GLBuffer const SceneBuffer = AcquireSceneBuffer(Scene.ViewMatrix, Scene.ProjectionMatrix);
		GLBuffer const TransformBuffer = AcquireTransformBuffer(Scene.Models);

		if (!FrameBuffer || !ColorTexture || !NormalTexture || !DepthTexture)
		{
			goto CleanUp;
		}

		GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FrameBuffer));
		GL_CHECK(glViewport(0, 0, Settings.Width, Settings.Height));
		glClear(GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		
		{
			uint32_t BufferOffset = 0;
			for (auto& [MeshBuffer, Shader, Texture, AnimationBuffer] : Meshes)
			{
				auto& [VertexBuffer, IndexBuffer] = MeshBuffer;
				GL_CHECK(glBindVertexArray(VertexBuffer.VAO));
	
				GL_CHECK(glUseProgram(Shader));
				
				GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer.VBO));
				GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer.Id));
	
				GL_CHECK(glBindBufferBase(
				   GL_UNIFORM_BUFFER,
				   IRenderBackend::SceneDataBinding,
				   SceneBuffer.Id));
	
				GL_CHECK(glBindBufferRange(
				   GL_UNIFORM_BUFFER,
				   IRenderBackend::ModelDataBinding,
				   TransformBuffer.Id,
				   BufferOffset,
				   sizeof(EvaluatedModel::Transformation)));
				BufferOffset += sizeof(EvaluatedModel::Transformation);
				
				GL_CHECK(glBindBufferBase(
				   GL_SHADER_STORAGE_BUFFER,
				   IRenderBackend::AnimationDataBinding,
				   AnimationBuffer.Id));
	
				if (ShaderTextureUsage[Shader])
				{				
					GL_CHECK(glBindTextureUnit(0, Texture));
					GL_CHECK(glUniform1i(IRenderBackend::TextureBinding, 0)); 
				}
				
				GL_CHECK(glDrawElements(GL_TRIANGLES, IndexBuffer.Size / (sizeof(uint32_t)), GL_UNSIGNED_INT, 0));
	
				//Cleanup
				GL_CHECK(glBindVertexArray(0));
				GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, IRenderBackend::SceneDataBinding, 0));
				GL_CHECK(glBindBufferBase(GL_UNIFORM_BUFFER, IRenderBackend::ModelDataBinding, 0));
				GL_CHECK(glBindBufferBase(GL_SHADER_STORAGE_BUFFER, IRenderBackend::AnimationDataBinding, 0));
				GL_CHECK(glBindBuffer(GL_UNIFORM_BUFFER, 0));
				GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
				GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
				GL_CHECK(glUseProgram(0));
			}
		}
		

		//Post
		PostProcessing(ColorTexture, Settings, PostShaders);

		Result = GetResults(FrameBuffer);
		
		//Cleanup
		CleanUp:
		for (auto& [a, b, c, AnimationBuffer] : Meshes)
		{
			ReleaseDynamicBuffer(AnimationBuffer);
		}
		
		GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
		ReleaseDynamicBuffer(TransformBuffer);
		ReleaseDynamicBuffer(SceneBuffer);

		//Process Results
		ReleaseFrameBuffer(FrameBuffer);

		glfwMakeContextCurrent(pPrevContext);
		RenderMutex.unlock();
		
		return Result;
	}

	MeshBuffers UploadMesh(AssetID Id)
	{
		auto It = Meshes.find(Id);
		if (It != Meshes.end())
		{
			return It->second;
		}

		MeshAsset Mesh = *AssetManager::GetMesh(Id);
		auto& Vertices = Mesh.Vertices;
		auto& Indices = Mesh.Indices;

		MeshBuffers NewBuffer;
		auto& [VertBuffer, IndexBuffer] = NewBuffer;

		VertBuffer.Size = Vertices.size() * sizeof(Vertex);
		IndexBuffer.Size = Indices.size() * sizeof(uint32_t);
		
		GL_CHECK(glCreateVertexArrays (1, &VertBuffer.VAO));
		GL_CHECK(glBindVertexArray(VertBuffer.VAO));

		GL_CHECK(glCreateBuffers(1, &VertBuffer.VBO));
		GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, VertBuffer.VBO));
		
		// Copy vertex attributes to GPU
		glBufferData(GL_ARRAY_BUFFER,
		             VertBuffer.Size,
		             Vertices.data(),
		             GL_STATIC_DRAW);

		GL_CHECK(glCreateBuffers(1, &IndexBuffer.Id));
		GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer.Id));
		
		// Copy vertex indices to GPU
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		             IndexBuffer.Size,
		             Indices.data(),
		             GL_STATIC_DRAW);

		
		/*  Send vertex attributes to shaders */
		for (auto i = 0; i < Vertex::GetAttributeCount(); ++i)
		{
			auto& Attribute = Vertex::GetAttributes()[i];
			GL_CHECK(glEnableVertexAttribArray(Attribute.Location));
			if (Attribute.Format == EVertexAttributeFormatType::IVector2D ||
    			Attribute.Format == EVertexAttributeFormatType::IVector3D ||
    			Attribute.Format == EVertexAttributeFormatType::IVector4D)
			{
				GL_CHECK(glVertexAttribIPointer(
        		Attribute.Location,
        		GetFormatSize(Attribute.Format),
        		GetFormatType(Attribute.Format),
        		sizeof(Vertex),
        		reinterpret_cast<const void*>(Attribute.Offset)
				));
			}
			else
			{
				GL_CHECK(glVertexAttribPointer(
        		Attribute.Location,
        		GetFormatSize(Attribute.Format),
        		GetFormatType(Attribute.Format),
        		GL_FALSE,
        		sizeof(Vertex),
        		reinterpret_cast<const void*>(Attribute.Offset)
				));
			}
		}

		Meshes.insert({ Id, NewBuffer });


		GL_CHECK(glBindVertexArray(0));

		return NewBuffer;
	}
	GLuint UploadShader(AssetID Id)
	{
		if (Shaders.contains(Id))
		{
			return Shaders[Id];
		}

		auto Shader = AssetManager::GetShader(Id);

		if (!Shader)
		{
			return 0;
		}
		
		GLuint VertexShader = GL_CHECK(glCreateShader(GL_VERTEX_SHADER));

		const char* VertexShaderSources[] = {"#version 460\n", "#define VERTEX_SHADER\n", Shader->SourceCode.c_str()};
		GL_CHECK(glShaderSource(VertexShader, sizeof(VertexShaderSources)/sizeof(VertexShaderSources[0]), VertexShaderSources, nullptr));
		
		GL_CHECK(glCompileShader(VertexShader));
		
		std::string const VertexName = std::string("Vertex Shader: " + Id.Name);

		if (!ValidateShader(VertexShader, VertexName.c_str()))
		{
			return 0;
		}

		GLuint FragmentShader = GL_CHECK(glCreateShader(GL_FRAGMENT_SHADER));

		const char* FragmentShaderSources[] = {"#version 460\n", "#define FRAGMENT_SHADER\n", Shader->SourceCode.c_str()};
		GL_CHECK(glShaderSource(FragmentShader, sizeof(FragmentShaderSources)/sizeof(FragmentShaderSources[0]), FragmentShaderSources, nullptr));
		
		GL_CHECK(glCompileShader(FragmentShader));
		
		std::string const FragmentName = std::string("Fragment Shader: " + Id.Name);
		if (!ValidateShader(FragmentShader, FragmentName.c_str()))
		{
			return 0;
		}
		
		GLuint Program = GL_CHECK(glCreateProgram());
		GL_CHECK(glAttachShader(Program, VertexShader));
		GL_CHECK(glAttachShader(Program, FragmentShader));
		GL_CHECK(glLinkProgram(Program));
		if (!ValidateProgram(Program))
		{
			GL_CHECK(glDeleteShader(VertexShader));
			return 0;
		}
		
		GL_CHECK(glDeleteShader(VertexShader));
		GL_CHECK(glDeleteShader(FragmentShader));

		bool bHasTexture = false;
		GLint UniformCount;
		GL_CHECK(glGetProgramInterfaceiv(Program, GL_UNIFORM, GL_ACTIVE_RESOURCES, &UniformCount));
		
		for (GLint i = 0; i < UniformCount; ++i)
		{
    		GLint Location;
    		GLenum Property = GL_LOCATION;
					
    		GL_CHECK(glGetProgramResourceiv(Program, GL_UNIFORM, i, 1, &Property, 1, nullptr, &Location));
					
    		if (Location == IRenderBackend::TextureBinding)
    		{
      		bHasTexture = true;
      		break;
    		}
		}

		Shaders.insert({Id, Program});
		ShaderTextureUsage.insert({Program, bHasTexture});
		return Program;
	}
	
	GLuint UploadPostShader(AssetID Id)
	{
		if (Shaders.contains(Id))
		{
			return Shaders[Id];
		}

		auto Shader = AssetManager::GetShader(Id);

		if (!Shader)
		{
			return 0;
		}
		
		GLuint const ComputeShader = GL_CHECK(glCreateShader(GL_COMPUTE_SHADER));

		const char* ComputeShaderSources[] = {"#version 460\n", Shader->SourceCode.c_str()};
		glShaderSource(ComputeShader, sizeof(ComputeShaderSources)/sizeof(ComputeShaderSources[0]), ComputeShaderSources, nullptr);
		
		GL_CHECK(glCompileShader(ComputeShader));
		
		std::string const ComputeName = std::string("Compute Shader: " + Id.Name);

		if (!ValidateShader(ComputeShader, ComputeName.c_str()))
		{
			return 0;
		}
		
		GLuint const Program = GL_CHECK(glCreateProgram());
		GL_CHECK(glAttachShader(Program, ComputeShader));
		GL_CHECK(glLinkProgram(Program));
		if (!ValidateProgram(Program))
		{
			return 0;
		}
		
		GL_CHECK(glDeleteShader(ComputeShader));
		
		Shaders.insert({Id, Program});
		return Program;
	}
	
	GLuint UploadTexture(AssetID Id)
	{
		if (Textures.contains(Id))
		{
			return Textures[Id];
		}
		
		auto Texture = AssetManager::GetTexture(Id);

		if (!Texture && FallbackTexture)
		{
			return FallbackTexture;
		}
		
		GLuint NewTex;
		GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &NewTex));

		GL_CHECK(glTextureParameteri(NewTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL_CHECK(glTextureParameteri(NewTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL_CHECK(glTextureParameteri(NewTex, GL_TEXTURE_WRAP_S, GL_REPEAT));
		GL_CHECK(glTextureParameteri(NewTex, GL_TEXTURE_WRAP_T, GL_REPEAT));

		GLenum Format = GL_R8;
		if (Texture)
		{
			switch (Texture->ChannelCount)
			{
			case 1:
				Format = GL_R8;
				break;
			case 2:
				Format = GL_RG8;
				break;
			case 3:
				Format = GL_RGB8;
				break;
			case 4:
				Format = GL_RGBA8;
				break;
			}
		}
		GL_CHECK(glTextureStorage2D(NewTex, 1, Format, Texture ? Texture->Width : 1, Texture ? Texture->Height : 1));

		if (!Texture)
		{
			FallbackTexture = NewTex;
			return FallbackTexture;
		}

		switch (Format)
		{
		case GL_R8:
			Format = GL_RED;
			break;
		case GL_RG8:
			Format = GL_RG;
			break;
		case GL_RGB8:
			Format = GL_RGB;
			break;
		case GL_RGBA8:
			Format = GL_RGBA;
			break;
		}
		
		GL_CHECK(glTextureSubImage2D(NewTex,
			0,
			0,
			0,
			Texture->Width,
			Texture->Height,
			Format,
			GL_UNSIGNED_BYTE,
			Texture->pBuffer.get()
		));

		
		Textures.insert({Id, NewTex});
		return NewTex;
	}
	GLBuffer AcquireTransformBuffer(const std::vector<EvaluatedModel>& Models)
	{
		if (Models.empty())
		{
			return {0,0, true};
		}
		
		GLBuffer Buffer = AcquireDynamicBuffer(sizeof(glm::mat4)*Models.size(), false);


		glm::mat4* pMappedData = reinterpret_cast<glm::mat4*>(glMapNamedBuffer(Buffer.Id, GL_WRITE_ONLY));

		for (auto& Model : Models)
		{
			*pMappedData = Model.Transformation;
			pMappedData += 1;
		}

		GL_CHECK(glUnmapNamedBuffer(Buffer.Id));

		return Buffer;
	}
	GLBuffer AcquireAnimationBuffer(const std::vector<glm::mat4> Matrices)
	{
		if (Matrices.empty())
		{
			return {0,0, true};
		}
		
		auto const BufferSize = sizeof(glm::mat4)*Matrices.size();
		GLBuffer Buffer = AcquireDynamicBuffer(BufferSize, true);
		GL_CHECK(glNamedBufferSubData(Buffer.Id, 0, BufferSize, Matrices.data()));

		return Buffer;
	}
	GLBuffer AcquireSceneBuffer(const glm::mat4& View, const glm::mat4& Projection)
	{
		GLBuffer Buffer = AcquireDynamicBuffer(sizeof(glm::mat4)*2, false);

		glm::mat4* pMappedData = reinterpret_cast<glm::mat4*>(glMapNamedBuffer(Buffer.Id, GL_WRITE_ONLY));

		for (auto& Matrix : {View, Projection})
		{
			*pMappedData = Matrix;
			pMappedData += 1;
		}

		GL_CHECK(glUnmapNamedBuffer(Buffer.Id));

		return Buffer;
	}

	GLBuffer AcquireDynamicBuffer(GLsizei Size, bool bIsStorage)
	{
		for (auto& [bIsActive, Buffer] : DynamicBuffers)
		{
			if (*bIsActive || Buffer.bIsStorage != bIsStorage)
			{
				continue;
			}
			
			if (Buffer.Size >= Size)
			{
				
				//Check if the Acquire was successfull
				bool bWasInActive = bIsActive->fetch_or(1);

				if (bWasInActive)
				{
					continue;
				}

				return Buffer;
			}
		}

		GLBuffer NewBuffer;
		NewBuffer.Size = Size;
		GL_CHECK(glCreateBuffers(1, &NewBuffer.Id));
		GL_CHECK(glNamedBufferData(NewBuffer.Id, Size, NULL, GL_DYNAMIC_DRAW));
		
		DynamicBuffers.push_back({std::make_unique<std::atomic_char>(), NewBuffer});
		return NewBuffer;
	}
	void ReleaseDynamicBuffer(const GLBuffer& Buffer)
	{
		auto It = std::find_if
		(
			DynamicBuffers.begin(),
			DynamicBuffers.end(), 
			[Buffer](auto& a)
			{
				return a.second.Id == Buffer.Id;
			}
		);

		if (It == DynamicBuffers.end())
		{
			return;
		}

		*It->first = false;
	}
	
	std::tuple<GLuint, GLuint, GLuint, GLuint> AcquireFrameBuffer(const RenderSettings& Settings)
	{
		GLuint TexColorBuffer;
		GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &TexColorBuffer));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, TexColorBuffer));
		
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Settings.Width, Settings.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		GLuint TexNormalBuffer;
		GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &TexNormalBuffer));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, TexNormalBuffer));
		
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Settings.Width, Settings.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL));
		
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		GLuint TexDepthBuffer;
		GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &TexDepthBuffer));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, TexDepthBuffer));
		
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Settings.Width, Settings.Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL));
		
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		GLuint FrameBuffer = 0;
		GL_CHECK(glCreateFramebuffers(1, &FrameBuffer));
		
		GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)FrameBuffer));
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexColorBuffer, 0));
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, TexNormalBuffer, 0));
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexDepthBuffer, 0));
		
		GLenum status = GL_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER));
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			return {0, 0, 0, 0};
		}
	
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, 0));


		constexpr unsigned char const ColorClear[] = { 0, 0, 0, 0 };
		GL_CHECK(glClearTexImage(TexColorBuffer, 0, GL_RGBA, GL_UNSIGNED_BYTE, ColorClear));

		constexpr unsigned char const NormalClear[] = {127, 127, 255};
		GL_CHECK(glClearTexImage(TexNormalBuffer, 0, GL_RGB, GL_UNSIGNED_BYTE, NormalClear));

		GLenum const DrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

		GL_CHECK(glNamedFramebufferDrawBuffers(FrameBuffer, sizeof(DrawBuffers)/sizeof(DrawBuffers[0]), DrawBuffers));

		glFinish();
		return {FrameBuffer, TexColorBuffer, TexNormalBuffer, TexDepthBuffer};
	}
	
	void ReleaseFrameBuffer(GLuint FrameBuffer)
	{
		GLint Object = 0;
		GLuint Texture = 0;

		//Color
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		Texture = Object;
		GL_CHECK(glDeleteTextures(1, &Texture));

		//Normal
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		Texture = Object;
		GL_CHECK(glDeleteTextures(1, &Texture));

		//Depth
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		Texture = Object;
		GL_CHECK(glDeleteTextures(1, &Texture));

		glDeleteFramebuffers(1, &FrameBuffer);
	}
	
	void Shutdown()
	{
		GLFWwindow* pPrevWindow = glfwGetCurrentContext();
		glfwMakeContextCurrent(pWindow);
		for (auto& [x, Buffer] : DynamicBuffers)
		{
			GL_CHECK(glDeleteBuffers(1, &Buffer.Id));
		}

		if (FallbackTexture)
		{
			GL_CHECK(glDeleteTextures(1, &FallbackTexture));
		}
		for (auto& [x, Texture] : Textures)
		{
			GL_CHECK(glDeleteTextures(1, &Texture));
		}
		
		for (auto& [x, Buffers] : Meshes)
		{
			auto& [Vertex, Index] = Buffers;
			GL_CHECK(glDeleteVertexArrays(1, &Vertex.VAO));
			GL_CHECK(glDeleteBuffers(1, &Vertex.VBO));
			GL_CHECK(glDeleteBuffers(1, &Index.Id));
		}

		for (auto& [x, Program] : Shaders)
		{
			GL_CHECK(glDeleteProgram(Program));
		}
		
		glfwDestroyWindow(pWindow);
		glfwMakeContextCurrent(pPrevWindow);
	}

	RenderResult GetResults(GLuint FrameBuffer)
	{
		GLint Object = 0;
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		GLuint const ColorTexture = Object;

		GLint Width, Height;
		GL_CHECK(glGetTextureLevelParameteriv(ColorTexture, 0, GL_TEXTURE_WIDTH, &Width));
		GL_CHECK(glGetTextureLevelParameteriv(ColorTexture, 0, GL_TEXTURE_HEIGHT, &Height));

		GLsizei ColorSize = Width*Height*4*sizeof(unsigned char);
		std::vector<char> ColorBuffer(ColorSize, 0);
		GLsizei NormalSize = Width*Height*3*sizeof(unsigned char);
		std::vector<char> NormalBuffer(NormalSize, 0);
		GLsizei DepthSize = Width*Height*sizeof(unsigned char);
		std::vector<char> DepthBuffer(DepthSize, 0);

		//Color		
		GL_CHECK(glGetTextureImage(ColorTexture, 0, GL_RGBA, GL_UNSIGNED_BYTE, ColorSize, ColorBuffer.data()));

		//Normal
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		GLuint const NormalTexture = Object;
		GL_CHECK(glGetTextureImage(NormalTexture, 0, GL_RGB, GL_UNSIGNED_BYTE, NormalSize, NormalBuffer.data()));

		//Depth
		GL_CHECK(glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object));
		GLuint const DepthTexture = Object;
		GL_CHECK(glGetTextureImage(DepthTexture, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, DepthSize, DepthBuffer.data()));
		
		
		RenderResult Result = 
		{
			std::make_unique<Image>(ColorBuffer.data(), Width, Height, 4),
			std::make_unique<Image>(NormalBuffer.data(), Width, Height, 3),
			std::make_unique<Image>(DepthBuffer.data(), Width, Height, 1)
		};
		
		return Result;
	}

	void PostProcessing(GLuint ColorTexture, const RenderSettings& Settings, const std::vector<GLuint>& ComputeShaders)
	{
		GLuint TexColorBuffer;
		GL_CHECK(glCreateTextures(GL_TEXTURE_2D, 1, &TexColorBuffer));
		GL_CHECK(glBindTexture(GL_TEXTURE_2D, TexColorBuffer));
		
		GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Settings.Width, Settings.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		
		GL_CHECK
		(
			glCopyImageSubData(ColorTexture,
			GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			TexColorBuffer,
			GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			Settings.Width,
			Settings.Height,
			1)
		);

		
		for (auto& Program : ComputeShaders)
		{
			if (Program == 0)
			{
				continue;
			}
			
			GL_CHECK(glUseProgram(Program));
			GL_CHECK(glBindImageTexture(0, ColorTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8));
			GL_CHECK(glUniform1i(0, 0)); 
			GL_CHECK(glBindImageTexture(1, TexColorBuffer, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8));
			GL_CHECK(glUniform1i(1, 1)); 
			glDispatchCompute(Settings.Width, Settings.Height, 1);
			glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		}
		glUseProgram(0);

		GL_CHECK
		(
			glCopyImageSubData(TexColorBuffer,
			GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			ColorTexture,
			GL_TEXTURE_2D,
			0,
			0,
			0,
			0,
			Settings.Width,
			Settings.Height,
			1)
		);

		glDeleteTextures(1, &TexColorBuffer);
	}
private:
	GLFWwindow* pWindow;
	std::mutex RenderMutex;
	std::unordered_map<AssetID, MeshBuffers> Meshes;
	std::unordered_map<AssetID, GLuint> Shaders;
	std::unordered_map<GLuint, bool> ShaderTextureUsage;
	std::unordered_map<AssetID, GLuint> Textures;
	std::vector<std::pair<std::unique_ptr<std::atomic_char>, GLBuffer>> DynamicBuffers;
	GLuint FallbackTexture = 0;
};

bool OpenGLBackend::Startup()
{
	if (pImpl)
	{
		return true;
	}

	pImpl = std::make_unique<Pimpl>();
	return pImpl->Startup();
}

void OpenGLBackend::Shutdown()
{
	pImpl->Shutdown();
	pImpl.reset();
}

RenderResult OpenGLBackend::Render(const EvaluatedScene& Scene, const RenderSettings& Settings)
{
	return pImpl->Render(Scene, Settings);
}

GLenum GetFormatType(EVertexAttributeFormatType Format)
{
	switch (Format)
	{
	case EVertexAttributeFormatType::Float:
			return GL_FLOAT;
		break;
	case EVertexAttributeFormatType::Vector2D:
			return GL_FLOAT;
		break;
	case EVertexAttributeFormatType::Vector3D:
			return GL_FLOAT;
		break;
	case EVertexAttributeFormatType::Vector4D:
			return GL_FLOAT;
		break;
	case EVertexAttributeFormatType::IVector2D:
			return GL_INT;
		break;
	case EVertexAttributeFormatType::IVector3D:
			return GL_INT;
		break;
	case EVertexAttributeFormatType::IVector4D:
			return GL_INT;
		break;
	default:
			return GL_INT;
		break;
	}
}
GLsizei GetFormatSize(EVertexAttributeFormatType Format)
{
	switch (Format)
	{
	case EVertexAttributeFormatType::Float:
			return 1;
		break;
	case EVertexAttributeFormatType::Vector2D:
			return 2;
		break;
	case EVertexAttributeFormatType::Vector3D:
			return 3;
		break;
	case EVertexAttributeFormatType::Vector4D:
			return 4;
		break;
	case EVertexAttributeFormatType::IVector2D:
			return 2;
		break;
	case EVertexAttributeFormatType::IVector3D:
			return 3;
		break;
	case EVertexAttributeFormatType::IVector4D:
			return 4;
		break;
	default:
			return 1;
		break;
	}
}

bool ValidateShader(GLuint shader, const char* label)
{
	char buf[512]; GLsizei len = 0; GLint ok = 0;
	glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
	GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &ok));
	if (ok != GL_TRUE && len > 0)
	{
		std::cerr << "Shader [" << label << "] compile error:\n" << buf << '\n';
		return false;
	}
	else
	{
		std::cout << "Shader [" << label << "] compiled OK.\n";
	}

	return true;
}
 
bool ValidateProgram(GLuint program)
{
	char buf[512]; GLsizei len = 0; GLint ok = 0;
	GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &ok));
	if (ok != GL_TRUE)
	{
		glGetProgramInfoLog(program, sizeof(buf), &len, buf);
		std::cerr << "Program " << program << " link error:\n" << buf << '\n';
		return false;
	}
	else
	{
		std::cout << "Program " << program << " linked OK.\n";
	}
 
	GL_CHECK(glValidateProgram(program));
	GL_CHECK(glGetProgramiv(program, GL_VALIDATE_STATUS, &ok));
	if (ok == GL_FALSE)
	{
		glGetProgramInfoLog(program, sizeof(buf), &len, buf);
		
		std::cerr << "Program " << program << " validation failed: "<< buf << "\n";
		return false;
	}
	else
	{
		std::cout << "Program " << program << " validated OK.\n";
	}
	
	return true;
}

OpenGLBackend::OpenGLBackend() : pImpl(nullptr)
{
	
}
OpenGLBackend::~OpenGLBackend() = default;

const char* GLErrorString(GLenum err)
{
    switch (err)
    {
        case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
        case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
        case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
        case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
        case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
        case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
        case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
        default: return "UNKNOWN";
    }
}
