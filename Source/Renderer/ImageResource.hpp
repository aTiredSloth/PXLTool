#ifndef IMAGERESOURCE_HPP
#define IMAGERESOURCE_HPP
#include <cstring>
#include <vector>

class Image
{
public:
	Image(char* pBuffer, size_t Width, size_t Height, size_t ComponentCount);

	Image(const Image&) = delete;
	Image(Image&& Temp);
	~Image() = default;
	
	size_t GetSize() const;
	void Copy(char* pDestination);
private:
	std::vector<uint8_t> Buffer;
};

#endif
