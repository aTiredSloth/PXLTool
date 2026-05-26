#ifndef OFFLINERENDERER_HPP
#define OFFLINERENDERER_HPP
#include "Core/Runtime/SceneDescription.hpp"
#include "Renderer/Backends/RenderSettings.hpp"
#include "Renderer/ImageResource.hpp"
#include <future>
#include <memory>
using ImageResource = std::tuple<std::unique_ptr<Image>, std::unique_ptr<Image>, std::unique_ptr<Image>>;

enum class RenderBackend
{
	OpenGL
};

namespace OfflineRenderer
{
	bool Initialize(RenderBackend Backend);
	void Shutdown();

	std::promise<ImageResource> DispatchRender(const SceneDescription& Scene, const RenderSettings& Settings);
}

#endif
