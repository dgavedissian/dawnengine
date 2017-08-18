/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#pragma once

#include "renderer/Material.h"
#include "ecs/Component.h"

namespace dw {
class DW_API Renderable : public Object {
public:
    DW_OBJECT(Renderable);

    Renderable(Context* context);
    virtual ~Renderable();

    /// Returns the material of this Renderable.
    /// @return The material currently assigned to this Renderable.
    Material* material() const;

    /// Changes the material used to render this Renderable object.
    /// @param material The material to assign to this Renderable.
    void setMaterial(SharedPtr<Material> material);

    /// Draws this renderable to the specified view.
    virtual void draw(Renderer* renderer, uint view, const Mat4& modelMatrix) = 0;

protected:
    SharedPtr<Material> material_;
};

struct RenderableComponent : public Component {
    RenderableComponent(SharedPtr<Renderable> r) : renderable{r} {
    }
    SharedPtr<Renderable> renderable;
};
}  // namespace dw
