#include "ImageResource.hpp"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stbi/stbi_image_write.h"
#include <vector>

void write_png_to_memory_callback(void* context, void* data, int size) {
    auto* buffer = static_cast<std::vector<uint8_t>*>(context);
    const auto* bytes = static_cast<const uint8_t*>(data);
    
    // Append the new data packet into your vector
    buffer->insert(buffer->end(), bytes, bytes + size);
}

Image::Image(char* pBuffer, size_t Width, size_t Height, size_t ComponentCount)
{
	int const Stride = Width*ComponentCount;
	stbi_write_png_to_func(write_png_to_memory_callback, &Buffer, Width, Height, ComponentCount, 
		pBuffer, Stride);
}

Image::Image(Image&& Temp) :
Buffer(std::move(Temp.Buffer))
{
}

size_t Image::GetSize() const
{
	return Buffer.size();
}

void Image::Copy(char* pDestination)
{
	std::memcpy(pDestination, Buffer.data(), Buffer.size());
}
