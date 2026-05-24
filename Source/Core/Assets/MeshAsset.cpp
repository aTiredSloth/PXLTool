#include "MeshAsset.hpp"

static VertexInputDesc Bindings[] = 
{
	{
		.Binding = 0,
		.Stride = sizeof(Vertex)
	}
};
	
static VertexAttributeDesc Attribs[] = 
{
	//Position
	{
		.Binding = 0,
		.Location = 0,
		.Format = EVertexAttributeFormatType::Vector3D,
		.Offset = 0
	},
	{
		.Binding = 0,
		.Location = 1,
		.Format = EVertexAttributeFormatType::Vector3D,
		.Offset = offsetof(Vertex, Normal)
	},
	//TexCoords
	{
		.Binding = 0,
		.Location = 2,
		.Format = EVertexAttributeFormatType::Vector2D,
		.Offset = offsetof(Vertex, TexCoords)
	},
	{
		.Binding = 0,
		.Location = 3,
		.Format = EVertexAttributeFormatType::IVector4D,
		.Offset = offsetof(Vertex, BoneIds)
	},
	{
		.Binding = 0,
		.Location = 4,
		.Format = EVertexAttributeFormatType::Vector4D,
		.Offset = offsetof(Vertex, BoneWeights)
	}
};
	
const VertexInputDesc* Vertex::GetBindings()
{
	return Bindings;
}
unsigned Vertex::GetBindingCount()
{
	return sizeof(Bindings)/sizeof(Bindings[0]);
}

const VertexAttributeDesc* Vertex::GetAttributes()
{
	return Attribs;
}

unsigned Vertex::GetAttributeCount()
{
	return sizeof(Attribs)/sizeof(Attribs[0]);
}
