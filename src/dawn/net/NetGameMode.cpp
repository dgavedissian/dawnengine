/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "Base.h"
#include "net/NetInstance.h"
#include "net/NetGameMode.h"
#include "core/GameSession.h"

namespace dw {
NetGameMode::NetGameMode(Context* ctx, GameSession* session)
    : GameMode(ctx, session), server_started_(false) {
    session_->eventSystem()->addListener(this, &NetGameMode::eventOnJoinServer);
    session_->eventSystem()->addListener(this, &NetGameMode::eventOnServerClientConnected);
    session_->eventSystem()->addListener(this, &NetGameMode::eventOnServerClientDisconnected);
}

NetGameMode::~NetGameMode() {
    session_->eventSystem()->removeAllListeners(this);
}

void NetGameMode::clientOnJoinServer() {
}

void NetGameMode::serverOnStart() {
}

void NetGameMode::serverOnEnd() {
}

void NetGameMode::serverOnClientConnected() {
}

void NetGameMode::serverOnClientDisconnected() {
}

void NetGameMode::onStart() {
    if (session_->net() && session_->net()->netMode() == NetMode::Server) {
        serverOnStart();
        server_started_ = true;
    }
}

void NetGameMode::onEnd() {
    if (server_started_) {
        serverOnEnd();
    }
}

void NetGameMode::update(float) {
    if (session_->net() && session_->net()->netMode() == NetMode::Server && !server_started_) {
        serverOnStart();
        server_started_ = true;
    }
    if (session_->net() && session_->net()->netMode() != NetMode::Server && server_started_) {
        serverOnEnd();
        server_started_ = false;
    }
}

void NetGameMode::eventOnJoinServer(const JoinServerEvent&) {
    clientOnJoinServer();
}

void NetGameMode::eventOnServerClientConnected(const ServerClientConnectedEvent&) {
    serverOnClientConnected();
}

void NetGameMode::eventOnServerClientDisconnected(const ServerClientDisconnectedEvent&) {
    serverOnClientDisconnected();
}

bool NetGameMode::runningAsServer() const {
    return server_started_;
}
}  // namespace dw
