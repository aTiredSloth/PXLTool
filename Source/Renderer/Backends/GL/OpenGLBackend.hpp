#ifndef OPENGLBACKEND_HPP
#define OPENGLBACKEND_HPP
#include "Renderer/Backends/RenderBackend.hpp"
#include <memory>

class OpenGLBackend final : public IRenderBackend
{
public:
	virtual ~OpenGLBackend() = default;
	virtual bool Startup();
	virtual void Shutdown();
	virtual RenderResult Render(const EvaluatedScene& Scene, const RenderSettings& Settings);
private:
	class Pimpl;
	std::unique_ptr<Pimpl> pImpl;
};

#endif
