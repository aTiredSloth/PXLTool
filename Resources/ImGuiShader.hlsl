
struct VertexInput
{
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(1)]] float2 TexCoords : TEXCOORD0;
    [[vk::location(2)]] float4 Color : TEXCOORD1;
};

struct SceneData
{
    float3x3 View;
    float3x3 Projection;
    float Time;
};

struct ModelData
{
    float3x3 ModelMatrix;
    float3x3 UVMatrix;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float2 TexCoords : TEXCOORD0;
    [[vk::location(1)]] float4 Color : TEXCOORD1;
    [[vk::location(2)]] uint InstanceID : TEXCOORD2;
};

struct ProjectionData
{
	float4x4 Proj;
};

#ifdef VERTEX_SHADER
ConstantBuffer<ProjectionData> Projection : register(b0, space0);

VertexOutput Main(VertexInput Input, uint InstanceId : SV_InstanceID)
{
	VertexOutput Out;
	
	Out.Position = mul(Projection.Proj, float4(Input.Position, 1.0));
	Out.TexCoords = Input.TexCoords;
	Out.Color = Input.Color;
	
	return Out;
}
#else
#ifdef PIXEL_SHADER
Texture2D Texture : register(t0, space1);

struct PSOutput
{
    float4 ColorOutput : SV_TARGET0;
};

PSOutput Main(VertexOutput VSOut)
{
	SamplerState Sampler
	{
    	Filter = MIN_MAG_MIP_POINT;
    	AddressU = Clamp;
    	AddressV = Clamp;
	};

	PSOutput Out;
	float4 TexColor = Texture.Sample(Sampler, VSOut.TexCoords).rgba;

	Out.ColorOutput = TexColor * VSOut.Color;
    
	return Out;
}

#endif
#endif
