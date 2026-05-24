#ifndef IMAGERESOURCE_HPP
#define IMAGERESOURCE_HPP

#include <cstring>
class Image
{
public:
	Image(char* pBuffer, size_t Size) :
	pBuffer(pBuffer),
	Size(Size)
	{
		
	}

	Image(const Image&) = delete;
	Image(Image&& Temp) :
	pBuffer(Temp.pBuffer),
	Size(Temp.Size)
	{
		Temp.pBuffer = nullptr;
	}
	~Image()
	{
		if (pBuffer)
		{
			delete[] pBuffer;
		}
	}
	
	size_t GetSize() const
	{
		return Size;
	}
	
	void Copy(char* pDestination, size_t Size)
	{
		std::memcpy(pDestination, pBuffer, Size);
	}
private:
	char* pBuffer;
	size_t Size;
};

#endif
