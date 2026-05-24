#ifndef RENDERBACKEND_HPP
#define RENDERBACKEND_HPP
#include "Core/Evaluated/EvaluatedScene.hpp"
#include "Renderer/Backends/RenderSettings.hpp"
#include "Renderer/ImageResource.hpp"
#include <memory>


using RenderResult = std::tuple<std::unique_ptr<Image>, std::unique_ptr<Image>, std::unique_ptr<Image>>;

class IRenderBackend
{
public:
	static constexpr auto SceneDataBinding = 0u;
	static constexpr auto ModelDataBinding = 1u;
	static constexpr auto AnimationDataBinding = 2u;
public:
	virtual ~IRenderBackend() = default;
	virtual bool Startup() = 0;
	virtual void Shutdown() = 0;
	virtual RenderResult Render(const EvaluatedScene& Scene, const RenderSettings& Settings) = 0;
};

#endif
