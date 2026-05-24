
struct VertexInput
{
    [[vk::location(0)]] float3 Position : POSITION0;
    [[vk::location(1)]] float2 TexCoords : TEXCOORD0;
    [[vk::location(3)]] float3 Normal : TEXCOORD1;
};

struct VertexOutput
{
    float4 Position : SV_POSITION;
    [[vk::location(0)]] float3 Normal : TEXCOORD0;
    [[vk::location(1)]] float2 TexCoords : TEXCOORD1;
};

struct SceneData
{
	float4x4 ViewProj;
	float Time;
};

struct ModelData
{
	float4x4 ModelMatrix;
	float4x4 NormalMatrix;
};

struct AnimationData
{
    float4x4 AnimationMatrix;
};

#ifdef VERTEX_SHADER
ConstantBuffer<SceneData> Scene : register(b0, space0);
ConstantBuffer<ModelData> Model : register(b0, space1);
StructuredBuffer<AnimationData> Animation : register(t1, space1);

VertexOutput Main(VertexInput Input, uint InstanceId : SV_InstanceID)
{
	VertexOutput Out;

	float4 ModelPos = mul(Model.ModelMatrix, float4(Input.Position, 1.0));
    Out.Position = mul(Scene.ViewProj, ModelPos);
	Out.Normal = mul(Model.NormalMatrix, float4(Input.Normal, 0.0));
	Out.TexCoords = Input.TexCoords;
	
	return Out;
}
#else
#ifdef PIXEL_SHADER

struct PSOutput
{
    float4 ColorOutput : SV_TARGET0;
};

PSOutput Main(VertexOutput VSOut)
{
	PSOutput Out;
	Out.ColorOutput = float4(1,1,1,1);
    
	return Out;
}

#endif
#endif
