/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2018 (git@dga.me.uk)
 */
#include "Common.h"
#include "GameSession.h"
#include "net/NetInstance.h"

namespace dw {
GameSession::GameSession(Context* ctx, const GameSessionInfo& gsi)
    : Object(ctx), new_game_mode_(nullptr), game_mode_(nullptr) {
    event_system_ = makeUnique<EventSystem>(ctx);
    ui_ = makeUnique<UserInterface>(ctx, event_system_.get());
    scene_manager_ = makeUnique<SceneManager>(ctx, event_system_.get());

    // Initialise networking.
    if (gsi.start_info.is<GameSessionInfo::CreateNetGame>()) {
        auto& info = gsi.start_info.get<GameSessionInfo::CreateNetGame>();
        net_instance_ = NetInstance::listen(ctx, this, info.host, info.port, info.max_clients);
    } else if (gsi.start_info.is<GameSessionInfo::JoinNetGame>()) {
        auto& info = gsi.start_info.get<GameSessionInfo::JoinNetGame>();
        net_instance_ = NetInstance::connect(ctx, this, info.host, info.port);
    }
}

void GameSession::update(float dt) {
    event_system_->update(1.0f);  // TODO: Specify maximum time.
    if (net_instance_) {
        net_instance_->update(dt);
    }
    scene_manager_->update(dt);

    // Update the game mode.
    if (game_mode_) {
        game_mode_->update(dt);
    }
    if (new_game_mode_) {
        if (game_mode_) {
            game_mode_->onEnd();
        }
        game_mode_ = new_game_mode_;
        game_mode_->onStart();
        new_game_mode_.reset();
    }

    ui_->update(dt);
}

void GameSession::preUpdate() {
    ui_->preUpdate();
}

void GameSession::postUpdate() {
    ui_->postUpdate();
}

void GameSession::preRender() {
    ui_->preRender();
}

void GameSession::postRender() {
    ui_->postRender();
    ui_->render();
}

GameMode* GameSession::gameMode() const {
    return game_mode_.get();
}

UserInterface* GameSession::ui() const {
    return ui_.get();
}

SceneManager* GameSession::sceneManager() const {
    return scene_manager_.get();
}

EventSystem* GameSession::eventSystem() const {
    return event_system_.get();
}

NetInstance* GameSession::net() const {
    return net_instance_.get();
}

void GameSession::setGameMode(SharedPtr<GameMode> game_mode) {
    new_game_mode_ = game_mode;
}
}  // namespace dw
