
layout(location = 0, rgba8) uniform readonly image2D InputImage;
layout(location = 1, rgba8) uniform writeonly image2D OutputImage;

float Quantize(float value, float levels)
{
    return round(value * (levels - 1.0)) / (levels - 1.0);
}

layout (local_size_x = 1, local_size_y = 1) in;
void main()
{
    ivec2 TexCoord = ivec2(gl_GlobalInvocationID.xy);

    vec4 Color = imageLoad(InputImage, TexCoord);
    
    Color.r = Quantize(Color.r, 2.0);
    Color.g = Quantize(Color.g, 2.0);
    Color.b = Quantize(Color.b, 2.0);
    
    imageStore(OutputImage, TexCoord, Color);
}
