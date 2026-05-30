#ifndef TEXTUREASSET_HPP
#define TEXTUREASSET_HPP
#include <memory>

struct TextureAsset
{
	int Width, Height;
	unsigned ChannelCount;
	std::shared_ptr<char[]> pBuffer;
};
#endif
