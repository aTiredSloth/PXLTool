#include "OpenGLBackend.hpp"
#include "Core/Assets/AssetManager.hpp"
#include "Core/Assets/MeshAsset.hpp"
#include "Core/Assets/ShaderAsset.hpp"
#include "Renderer/Backends/GL/GLBuffer.hpp"
#include "Renderer/Backends/RenderBackend.hpp"
#include <atomic>
#include <cstddef>
#include <iostream>
#include <utility>
#include <memory>
#include <unordered_map>
#include <glm/fwd.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

static inline GLenum GetFormatType(EVertexAttributeFormatType Format);
static inline GLsizei GetFormatSize(EVertexAttributeFormatType Format);
static inline bool ValidateShader(GLuint shader, const char* label);
static inline bool ValidateProgram(GLuint program);

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
	virtual bool Startup()
	{
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		pWindow = glfwCreateWindow(32, 32, "OpenGLBackend", 0, 0);

		if (!pWindow)
		{
			std::cout << "Failed to Create GLFW Window" << std::endl;
			return false;
		}
		glfwMakeContextCurrent(pWindow);

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to Load Glad" << std::endl;
			return false;
		}

		return true;
	}
	virtual RenderResult Render(const EvaluatedScene& Scene, const RenderSettings& Settings)
	{
		std::vector<std::tuple<MeshBuffers, GLuint, GLBuffer>> Meshes;

		Meshes.reserve(Scene.Models.size());
		for (auto& Model : Scene.Models)
		{
			Meshes.push_back
			(
			   {
					UploadMesh(Model.MeshId),
			     	UploadShader(Model.ShaderId),
				   AquireAnimationBuffer(Model.Skeleton.Matrices)
				}
			);

			auto& [Mesh, Shader, AnimBuffer] = Meshes.back();
			if (!Shader)
			{
				return RenderResult();
			}
		}

		GLBuffer const SceneBuffer = AquireSceneBuffer(Scene.ViewMatrix, Scene.ProjectionMatrix);
		GLBuffer const TransformBuffer = AquireTransformBuffer(Scene.Models);

		GLuint FrameBuffer = AquireFrameBuffer(Settings);

		glBindFramebuffer(GL_DRAW_BUFFER, FrameBuffer);
		uint32_t BufferOffset = 0;
		for (auto& [MeshBuffer, Shader, AnimationBuffer] : Meshes)
		{
			auto& [VertexBuffer, IndexBuffer] = MeshBuffer;

			glUseProgram(Shader);

			glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer.VBO);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer.Id);

			glBindBufferBase(
			   GL_UNIFORM_BUFFER,
			   IRenderBackend::SceneDataBinding,
			   SceneBuffer.Id);

			glBindBufferRange(
			   GL_UNIFORM_BUFFER,
			   IRenderBackend::ModelDataBinding,
			   TransformBuffer.Id,
			   BufferOffset,
			   TransformBuffer.Size - BufferOffset);

			glBindBufferBase(
			   GL_SHADER_STORAGE_BUFFER,
			   IRenderBackend::AnimationDataBinding,
			   AnimationBuffer.Id);

			glDrawElements(GL_TRIANGLES, IndexBuffer.Size / (3 * sizeof(uint32_t)), GL_UNSIGNED_INT, 0);

			glBindBufferBase(GL_UNIFORM_BUFFER, IRenderBackend::SceneDataBinding, 0);
			glBindBufferBase(GL_UNIFORM_BUFFER, IRenderBackend::ModelDataBinding, 0);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, IRenderBackend::AnimationDataBinding, 0);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glUseProgram(0);
			
			ReleaseDynamicBuffer(AnimationBuffer);
		}
		glBindFramebuffer(GL_DRAW_BUFFER, 0);
		ReleaseDynamicBuffer(TransformBuffer);
		ReleaseDynamicBuffer(SceneBuffer);

		RenderResult Result = GetResults(FrameBuffer);

		ReleaseFrameBuffer(FrameBuffer);
		
		return std::move(Result);
	}

	MeshBuffers UploadMesh(AssetID Id)
	{
		if (Meshes.contains(Id))
		{
			return Meshes[Id];
		}

		MeshAsset Mesh = *AssetManager::GetMesh(Id);
		auto& Vertices = Mesh.Vertices;
		auto& Indices = Mesh.Indices;

		MeshBuffers NewBuffer;
		auto& [VertexBuffer, IndexBuffer] = NewBuffer;

		VertexBuffer.Size = Vertices.size() * sizeof(Vertex);
		IndexBuffer.Size = Indices.size() * sizeof(uint32_t);
		
		glGenVertexArrays(1, &VertexBuffer.VAO);
		glBindVertexArray(VertexBuffer.VAO);

		glGenBuffers(1, &VertexBuffer.VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer.VBO);
		
		/*  Copy vertex attributes to GPU */
		glBufferData(GL_ARRAY_BUFFER,
		             VertexBuffer.Size,
		             Vertices.data(),
		             GL_STATIC_DRAW);

		glGenBuffers(1, &IndexBuffer.Id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBuffer.Id);
		/*  Copy vertex indices to GPU */
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		             IndexBuffer.Size,
		             Indices.data(),
		             GL_STATIC_DRAW);

		
		/*  Send vertex attributes to shaders */
		for (auto i = 0; i < Vertex::GetAttributeCount(); ++i)
		{
			auto& Attribute = Vertex::GetAttributes()[i];
			glEnableVertexAttribArray(Attribute.Location);
			glVertexAttribPointer
			(
				Attribute.Location, 
				GetFormatSize(Attribute.Format),
				GetFormatType(Attribute.Format),
				false,
				sizeof(Vertex), 
				reinterpret_cast<const void*>(Attribute.Offset)
			);
		}

		Meshes.insert({ Id, NewBuffer });


		glBindVertexArray(0);

		return NewBuffer;
	}
	GLuint UploadShader(AssetID Id)
	{
		if (Shaders.contains(Id))
		{
			return Shaders[Id];
		}

		ShaderAsset Shader = *AssetManager::GetShader(Id);
		
		GLuint VertexShader = glCreateShader(GL_COMPUTE_SHADER);
		const char* VertexShaderSources[] = {"#define VERTEX_SHADER", Shader.SourceCode.c_str()};
		glShaderSource(VertexShader, sizeof(VertexShaderSources)/sizeof(VertexShaderSources[0]), VertexShaderSources, nullptr);
		glCompileShader(VertexShader);
		std::string const VertexName = std::string("Vertex Shader: " + Shader.Name);

		if (!ValidateShader(VertexShader, VertexName.c_str()))
		{
			return 0;
		}

		GLuint FragmentShader = glCreateShader(GL_COMPUTE_SHADER);
		const char* FragmentShaderSources[] = {"#define FRAGMENT_SHADER", Shader.SourceCode.c_str()};
		glShaderSource(FragmentShader, sizeof(FragmentShaderSources)/sizeof(FragmentShaderSources[0]), FragmentShaderSources, nullptr);
		glCompileShader(FragmentShader);
		std::string const FragmentName = std::string("Fragment Shader: " + Shader.Name);
		if (!ValidateShader(FragmentShader, FragmentName.c_str()))
		{
			return 0;
		}
		
		GLuint Program = glCreateProgram();
		glAttachShader(Program, VertexShader);
		glAttachShader(Program, FragmentShader);
		glLinkProgram(Program);
		if (!ValidateProgram(Program))
		{
			return 0;
		}
		
		glDeleteShader(VertexShader);
		glDeleteShader(FragmentShader);

		return Program;
	}
	GLBuffer AquireTransformBuffer(const std::vector<EvaluatedModel> Models)
	{
		GLBuffer Buffer = AquireDynamicBuffer(sizeof(glm::mat4)*Models.size(), false);


		glm::mat4* pMappedData = reinterpret_cast<glm::mat4*>(glMapNamedBuffer(Buffer.Id, GL_WRITE_ONLY));

		for (auto& Model : Models)
		{
			*pMappedData = Model.Transformation;
			pMappedData += 1;
		}

		glUnmapNamedBuffer(Buffer.Id);

		return Buffer;
	}
	GLBuffer AquireAnimationBuffer(const std::vector<glm::mat4> Matrices)
	{
		auto const BufferSize = sizeof(glm::mat4)*Matrices.size();
		GLBuffer Buffer = AquireDynamicBuffer(BufferSize, true);
		glNamedBufferSubData(Buffer.Id, 0, BufferSize, Matrices.data());

		return Buffer;
	}
	GLBuffer AquireSceneBuffer(const glm::mat4& View, const glm::mat4& Projection)
	{
		GLBuffer Buffer = AquireDynamicBuffer(sizeof(glm::mat4)*2, false);

		glm::mat4* pMappedData = reinterpret_cast<glm::mat4*>(glMapNamedBuffer(Buffer.Id, GL_WRITE_ONLY));

		for (auto& Matrix : {View, Projection})
		{
			*pMappedData = Matrix;
			pMappedData += 1;
		}

		glUnmapNamedBuffer(Buffer.Id);

		return Buffer;
	}

	GLBuffer AquireDynamicBuffer(GLsizei Size, bool bIsStorage)
	{
		for (auto& [bIsActive, Buffer] : DynamicBuffers)
		{
			if (*bIsActive || Buffer.bIsStorage == bIsStorage)
			{
				continue;
			}
			
			if (Buffer.Size >= Size)
			{
				//Check if the Aquire was successfull
				bool bWasInActive = std::atomic_fetch_or(bIsActive.get(), 1);

				if (bWasInActive)
				{
					continue;
				}

				return Buffer;
			}
		}

		GLBuffer NewBuffer;
		NewBuffer.Size = Size;
		glGenBuffers(1, &NewBuffer.Id);
		glNamedBufferData(NewBuffer.Id, Size, NULL, bIsStorage ? GL_SHADER_STORAGE_BUFFER : GL_UNIFORM_BUFFER);
		
		DynamicBuffers.push_back({std::make_unique<std::atomic_bool>(), NewBuffer});
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
	
	GLuint AquireFrameBuffer(const RenderSettings& Settings)
	{
		GLuint TexColorBuffer;
		glGenTextures(1, &TexColorBuffer);
		glBindTexture(GL_TEXTURE_2D, TexColorBuffer);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, Settings.Width, Settings.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLuint TexNormalBuffer;
		glGenTextures(1, &TexNormalBuffer);
		glBindTexture(GL_TEXTURE_2D, TexNormalBuffer);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Settings.Width, Settings.Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLuint TexDepthBuffer;
		glGenTextures(1, &TexDepthBuffer);
		glBindTexture(GL_TEXTURE_2D, TexDepthBuffer);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Settings.Width, Settings.Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		GLuint FrameBuffer = 0;
		glGenFramebuffers(1, &FrameBuffer);
		
		glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)FrameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, TexColorBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, TexNormalBuffer, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, TexDepthBuffer, 0);
		
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			return 0;
		}
	
		glBindTexture(GL_TEXTURE_2D, 0);

		return FrameBuffer;
	}
	
	void ReleaseFrameBuffer(GLuint FrameBuffer)
	{
		GLint Object = 0;
		GLuint Texture = 0;

		//Color
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		Texture = Object;
		glDeleteTextures(1, &Texture);

		//Normal
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		Texture = Object;
		glDeleteTextures(1, &Texture);

		//Depth
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		Texture = Object;
		glDeleteTextures(1, &Texture);
	}
	
	virtual void Shutdown()
	{
		for (auto& [x, Buffer] : DynamicBuffers)
		{
			glDeleteBuffers(1, &Buffer.Id);
		}

		for (auto& [Id, Buffers] : Meshes)
		{
			auto& [Vertex, Index] = Buffers;
			glDeleteVertexArrays(1, &Vertex.VAO);
			glDeleteBuffers(1, &Vertex.VBO);
			glDeleteBuffers(1, &Index.Id);
		}

		for (auto& [Id, Program] : Shaders)
		{
			glDeleteProgram(Program);
		}
		
		glfwDestroyWindow(pWindow);
	}

	RenderResult GetResults(GLuint FrameBuffer)
	{
		
		GLint Object = 0;
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		GLuint const ColorTexture = Object;

		GLint Width, Height;
		glGetTextureLevelParameteriv(ColorTexture, 0, GL_TEXTURE_WIDTH, &Width);
		glGetTextureLevelParameteriv(ColorTexture, 0, GL_TEXTURE_HEIGHT, &Height);

		GLsizei ColorSize = Width*Height*4*sizeof(char);
		char* const pColorBuffer = new char[ColorSize];
		GLsizei NormalSize = Width*Height*3*sizeof(char);
		char* const pNormalBuffer = new char[NormalSize];
		GLsizei DepthSize = Width*Height*sizeof(float);
		char* const pDepthBuffer = new char[DepthSize];
		
		//Normal
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		GLuint const NormalTexture = Object;

		//Depth
		glGetNamedFramebufferAttachmentParameteriv(FrameBuffer, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &Object);
		GLuint const DepthTexture = Object;

		
		return
		{
			std::make_unique<Image>(pColorBuffer, ColorSize),
			std::make_unique<Image>(pNormalBuffer, NormalSize),
			std::make_unique<Image>(pDepthBuffer, DepthSize)
		};
	}
private:
	GLFWwindow* pWindow;
	std::unordered_map<AssetID, MeshBuffers> Meshes;
	std::unordered_map<AssetID, GLuint> Shaders;
	std::vector<std::pair<std::unique_ptr<std::atomic_bool>, GLBuffer>> DynamicBuffers;
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
			return sizeof(float);
		break;
	case EVertexAttributeFormatType::Vector2D:
			return sizeof(glm::vec2);
		break;
	case EVertexAttributeFormatType::Vector3D:
			return sizeof(glm::vec3);
		break;
	case EVertexAttributeFormatType::Vector4D:
			return sizeof(glm::vec4);
		break;
	case EVertexAttributeFormatType::IVector2D:
			return sizeof(glm::ivec2);
		break;
	case EVertexAttributeFormatType::IVector3D:
			return sizeof(glm::ivec3);
		break;
	case EVertexAttributeFormatType::IVector4D:
			return sizeof(glm::ivec4);
		break;
	default:
			return sizeof(int);
		break;
	}
}

bool ValidateShader(GLuint shader, const char* label)
{
	char buf[512]; GLsizei len = 0; GLint ok = 0;
	glGetShaderInfoLog(shader, sizeof(buf), &len, buf);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
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
	glGetProgramInfoLog(program, sizeof(buf), &len, buf);
	glGetProgramiv(program, GL_LINK_STATUS, &ok);
	if (ok != GL_TRUE && len > 0)
	{
		std::cerr << "Program " << program << " link error:\n" << buf << '\n';
		return false;
	}
	else
	{
		std::cout << "Program " << program << " linked OK.\n";
	}
 
	glValidateProgram(program);
	glGetProgramiv(program, GL_VALIDATE_STATUS, &ok);
	if (ok == GL_FALSE)
	{
		std::cerr << "Program " << program << " validation failed.\n";
		return false;
	}
	else
	{
		std::cout << "Program " << program << " validated OK.\n";
	}
	
	return true;
}
