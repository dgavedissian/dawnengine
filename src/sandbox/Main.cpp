/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "Core.h"
#include "Renderer.h"
#include "Resource.h"
#include "Scene.h"
#include "UI.h"
#include "scene/space/PlanetLod.h"

using namespace dw;

class SandboxSession : public GameSession {
public:
    DW_OBJECT(SandboxSession);

    SharedPtr<CameraController> camera_controller;
    SharedPtr<PlanetLod> planet_;

    SandboxSession(Context* ctx, const GameSessionInfo& gsi) : GameSession(ctx, gsi) {
        module<Input>()->registerEventSystem(event_system_.get());
        const float radius = 1000.0f;

        // Create frame.
        auto frame = scene_graph_->addFrame(scene_graph_->root().newChild());

        // Create a camera.
        auto& camera = scene_manager_->createEntity(0, Vec3(0.0f, 0.0f, radius * 2.0f),
                                                    Quat::identity, *frame);
        camera.addComponent<CCamera>(0.1f, 10000.0f, 60.0f, 1280.0f / 800.0f);
        camera_controller = makeShared<CameraController>(context(), event_system_.get(), 300.0f);
        camera_controller->possess(&camera);

        // Create a planet.
        planet_ = makeShared<PlanetLod>(context(), scene_graph_.get(), radius, 40.0f, &camera);
    }

    ~SandboxSession() override {
        module<Input>()->unregisterEventSystem(event_system_.get());
    }

    void update(float dt) override {
        GameSession::update(dt);

        // Calculate distance to planet and adjust acceleration accordingly.
        auto& a = camera_controller->possessed()->transform()->position;
        auto& b = planet_->position();
        float altitude = SystemPosition{a}.getRelativeTo(b).Length() - planet_->radius();
        camera_controller->setAcceleration(altitude);

        camera_controller->update(dt);
        planet_->update(dt);
    }

    void render(float dt, float interpolation) override {
        GameSession::render(dt, interpolation);

        module<Renderer>()->rhi()->setViewClear(0, {0.0f, 0.0f, 0.1f, 0.2f});

        // Calculate average FPS.
        float current_fps = 1.0 / dt;
        static const int FPS_HISTORY_COUNT = 100;
        static float fps_history[FPS_HISTORY_COUNT];
        for (int i = 1; i < FPS_HISTORY_COUNT; ++i) {
            fps_history[i - 1] = fps_history[i];
        }
        fps_history[FPS_HISTORY_COUNT - 1] = current_fps;
        float average_fps = 0.0f;
        for (int i = 0; i < FPS_HISTORY_COUNT; ++i) {
            average_fps += fps_history[i] / FPS_HISTORY_COUNT;
        }

        // Update displayed FPS information every 100ms.
        static double accumulated_time = 0.0;
        static float displayed_fps = 60.0f;
        accumulated_time += dt;
        if (accumulated_time > 1.0f / 30.0f) {
            accumulated_time = 0;
            displayed_fps = average_fps;
        }

        // Display FPS information.
        ImGui::SetNextWindowPos({10, 10});
        ImGui::SetNextWindowSize({140, 40});
        if (!ImGui::Begin("FPS", nullptr,
                          ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::End();
            return;
        }
        ImGui::Text("FPS:   %.1f", displayed_fps);
        ImGui::Text("Frame: %.4f ms", 1000.0f / displayed_fps);
        ImGui::End();
    }
};

class Sandbox : public App {
public:
    DW_OBJECT(Sandbox);

    Sandbox() : App("Sandbox", DW_VERSION_STR) {
    }

    void init(const CommandLine& cmdline) override {
        auto rc = module<ResourceCache>();
        assert(rc);
        rc->addPath("base", "../media/base");
        rc->addPath("sandbox", "../media/sandbox");

        engine_->addSession(makeUnique<SandboxSession>(context(), GameSessionInfo{}));
    }

    void shutdown() override {
    }
};

DW_IMPLEMENT_MAIN(Sandbox);
