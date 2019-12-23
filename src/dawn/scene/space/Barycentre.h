/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "scene/space/SystemBody.h"

namespace dw {
/// A body that only serves as an empty point where other bodies orbit around.
class DW_API Barycentre : public SystemBody {
public:
    DW_OBJECT(Barycentre);

    using SystemBody::SystemBody;
};
}  // namespace dw
