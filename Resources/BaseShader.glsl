#ifdef VERTEX_SHADER
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec2 aBoneIds;
layout (location = 4) in vec2 aBoneWeights;

layout (std140, binding = 0) uniform Camera
{
    mat4 View;
    mat4 Projection;
};

layout (std140, binding = 1) uniform Model
{
	 mat4 ModelMatrix;
};

layout (std430, binding = 2) buffer Animation
{
	mat4 Matrices[];
};

out vec2 TexCoord;

void main()
{
	gl_Position = Projection * View * ModelMatrix * vec4(aPos, 1.0);
	TexCoord = aTexCoord;
}
#else

layout (location = 3) uniform sampler2D Texture;

in vec2 TexCoord;
layout(location = 0) out vec4 FinalColor;
layout(location = 1) out vec4 NormalColor;

void main()
{
	FinalColor = texture(Texture, TexCoord);
	NormalColor = vec4(0,0,0, 0);
}
#endif
