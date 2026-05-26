#ifndef OPENGLBACKEND_HPP
#define OPENGLBACKEND_HPP
#include "Renderer/Backends/RenderBackend.hpp"
#include <memory>

class OpenGLBackend final : public IRenderBackend
{
public:
	class Pimpl;
	OpenGLBackend();
	~OpenGLBackend();
	bool Startup() override;
	void Shutdown() override;
	RenderResult Render(const EvaluatedScene& Scene, const RenderSettings& Settings) override;
private:
	std::unique_ptr<Pimpl> pImpl;
};

#endif
