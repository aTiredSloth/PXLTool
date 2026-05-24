#ifndef GLBUFFER_HPP
#define GLBUFFER_HPP
#include "glad/glad.h"

struct GLBuffer
{
	GLuint Id; //Buffer Id
	GLsizei Size; //Size in bytes
	bool bIsStorage;
};

#endif
