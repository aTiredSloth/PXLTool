#ifdef VERTEX_SHADER
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in ivec4 aBoneIds;
layout (location = 4) in vec4 aBoneWeights;

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
out vec3 Normal;

void main()
{
	vec4 Pos = vec4(0.0);
	bool HasInfluence = false;
	for (int i = 0; i < 4; ++i)
	{
		if (aBoneIds[i] == -1)
		{
			continue;
		}
		HasInfluence = true;
		
		vec4 LocalPos = Matrices[aBoneIds[i]] * vec4(aPos, 1.0);

		Pos += LocalPos * aBoneWeights[i];
	}

	if (!HasInfluence)
	{
		Pos = vec4(aPos, 1.0);
	}
	mat4 ModelView = View * ModelMatrix;
	gl_Position = Projection * ModelView * Pos;
	TexCoord = aTexCoord;
	Normal = (transpose(inverse(ModelView)) * vec4(aNormal, 0.0)).xyz;
}
#else

layout (location = 3) uniform sampler2D Texture;

in vec2 TexCoord;
in vec3 Normal;
layout(location = 0) out vec4 FinalColor;
layout(location = 1) out vec3 NormalColor;

void main()
{
	FinalColor = texture(Texture, TexCoord);
	NormalColor = fma(Normal, vec3(0.5), vec3(0.5)).xyz;
}
#endif
