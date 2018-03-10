/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#pragma once

#include "renderer/Renderer.h"

namespace dw {
class DW_API NullRenderContext : public RenderContext {
public:
    DW_OBJECT(NullRenderContext);

    NullRenderContext(Context* ctx);
    virtual ~NullRenderContext();

    // Window management. Executed on the main thread.
    void createWindow(u16 width, u16 height, const String& title) override;
    void destroyWindow() override;
    void processEvents() override;
	bool isWindowClosed() const override;
	virtual Vec2i windowSize() const;
	virtual Vec2 windowScale() const;
	virtual Vec2i backbufferSize() const;

    // Command buffer processing. Executed on the render thread.
    void startRendering() override;
    void stopRendering() override;
    void processCommandList(Vector<RenderCommand>& command_list) override;
    bool frame(const Frame* frame) override;

private:
};

}  // namespace dw