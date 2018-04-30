/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#include "Common.h"
#include "renderer/Renderable.h"
#include "scene/SceneManager.h"
#include "renderer/MeshBuilder.h"
#include "resource/ResourceCache.h"
#include "PhysicsScene.h"

namespace dw {
SceneManager::SceneManager(Context* ctx) : Module(ctx), entity_id_allocator_(1) {
    setDependencies<ResourceCache>();

    background_renderable_root_ = makeShared<SceneNode>();
    background_entity_ = &createEntity(0, Position::origin, Quat::identity, nullptr)
                              .addComponent<RenderableComponent>(background_renderable_root_);
    background_entity_->transform()->setRelativeToCamera(true);

    physics_scene_ = makeUnique<PhysicsScene>(ctx, this);
}

SceneManager::~SceneManager() {
}

void SceneManager::createStarSystem() {
    auto vs = module<ResourceCache>()->get<VertexShader>("base:space/skybox.vs");
    auto fs = module<ResourceCache>()->get<FragmentShader>("base:space/skybox_starfield.fs");
    auto background_material =
        makeShared<Material>(context(), makeShared<Program>(context(), vs, fs));
    background_material->setTexture(
        module<ResourceCache>()->get<Texture>("base:space/starfield.jpg"));
    background_material->setUniform<int>("starfield_sampler", 0);
    auto skybox = MeshBuilder{context()}.normals(false).texcoords(true).createBox(-10000.0f);
    skybox->setMaterial(background_material);
    background_renderable_root_->addChild(skybox);
}

Entity& SceneManager::createEntity(EntityType type) {
    return createEntity(type, reserveEntityId());
}

Entity& SceneManager::createEntity(EntityType type, const Position& p, const Quat& o,
                                   Entity* parent) {
    Entity& e = createEntity(type);
    if (parent) {
        e.addComponent<TransformComponent>(p, o, parent->transform());
    } else {
        e.addComponent<TransformComponent>(p, o, nullptr);
    }
    return e;
}

Entity& SceneManager::createEntity(EntityType type, const Position& p, const Quat& o) {
    return createEntity(type, p, o, nullptr);
}

Entity& SceneManager::createEntity(EntityType type, EntityId reserved_entity_id) {
    // Add to entity lookup table if reserved from elsewhere (i.e. server).
    if (entity_lookup_table_.find(reserved_entity_id) == entity_lookup_table_.end()) {
        entity_lookup_table_[reserved_entity_id] = nullptr;
    }

    // Look up slot and move new entity into it.
    auto entity_slot = entity_lookup_table_.find(reserved_entity_id);
    assert(entity_slot != entity_lookup_table_.end() && entity_slot->second == nullptr);
    auto entity =
        makeUnique<Entity>(context(), ontology_world_.getEntityManager(), reserved_entity_id, type);
    auto entity_ptr = entity.get();
    entity_slot->second = std::move(entity);
    return *entity_ptr;
}

EntityId SceneManager::reserveEntityId() {
    EntityId new_entity_id = entity_id_allocator_++;
    entity_lookup_table_[new_entity_id] = nullptr;
    return new_entity_id;
}

Entity* SceneManager::findEntity(EntityId id) {
    auto it = entity_lookup_table_.find(id);
    if (it != entity_lookup_table_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void SceneManager::removeEntity(Entity* entity) {
    entity_lookup_table_.erase(entity->id());
}

void SceneManager::beginMainLoop() {
    ontology_world_.getSystemManager().initialise();
}

void SceneManager::update(float dt) {
    last_dt_ = dt;
    physics_scene_->update(dt, nullptr);
    ontology_world_.update();
}

PhysicsScene* SceneManager::physicsScene() const {
    return physics_scene_.get();
}

float SceneManager::lastDeltaTime_internal() const {
    return last_dt_;
}
}  // namespace dw
