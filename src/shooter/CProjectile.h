/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#pragma once

#include "Base.h"
#include "core/math/Defs.h"
#include "scene/Component.h"
#include "scene/SceneManager.h"
#include "renderer/BillboardSet.h"
#include "net/CNetData.h"

using namespace dw;

class FreeListAllocator {
public:
    FreeListAllocator();
    explicit FreeListAllocator(int size);

    Option<int> allocate();
    void free(int index);

    // Must be larger than size().
    void resize(int new_size);

    int size() const;

private:
    int size_;
    List<int> free_;

#ifdef DW_DEBUG
    Set<int> allocated_;
#endif
};

struct ProjectileTypeInfo {
    float damage;
    Vec2 size;
    SharedPtr<Texture> texture;
};

struct CProjectile : public Component {
    int type;
    int particle_id;
    Vec3 position;
    Vec3 direction;
    Vec3 velocity;
    Colour colour;

    // Replication layout.
    static RepLayout repLayout() {
        return {{RepProperty::bind<CProjectile>(&CProjectile::type),
                 RepProperty::bind<CProjectile>(&CProjectile::position),
                 RepProperty::bind<CProjectile>(&CProjectile::direction),
                 RepProperty::bind<CProjectile>(&CProjectile::velocity),
                 RepProperty::bind<CProjectile>(&CProjectile::colour)},
                {}};
    }
};

class SProjectile : public EntitySystem<CProjectile>, public Object {
public:
    DW_OBJECT(SProjectile);

    SProjectile(Context* ctx, SceneManager* scene_manager, NetInstance* net, Frame* frame,
                const HashMap<int, ProjectileTypeInfo>& types);

    Entity* createNewProjectile(int type, const Vec3& position, const Vec3& direction,
                                const Vec3& velocity, const Colour& colour);

    void process(float dt) override;

private:
    struct ProjectileRenderData {
        Node* node;
        BillboardSet* billboard_set;
        FreeListAllocator free_billboards;
    };

    HashMap<int, ProjectileRenderData> render_data_;
    HashMap<int, ProjectileTypeInfo> types_;
    SceneManager* scene_manager_;
    NetInstance* net_;
};
