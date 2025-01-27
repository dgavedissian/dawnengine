/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "scene/Entity.h"
#include "net/NetRole.h"

namespace dw {
class DW_API NetEntityPipeline : public Object {
public:
    DW_OBJECT(NetEntityPipeline);

    NetEntityPipeline(Context* ctx);
    virtual ~NetEntityPipeline() = default;

    virtual Entity* createEntityFromType(EntityType type, NetRole role) = 0;
};
}  // namespace dw