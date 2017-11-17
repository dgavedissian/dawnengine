/*
* Dawn Engine
* Written by David Avedissian (c) 2012-2016 (git@davedissian.com)
*/
#include "Common.h"
#include "DawnEngine.h"

NAMESPACE_BEGIN

Engine* gEngine = nullptr;

// Application entry point
Engine::Engine(const String& game, const String& version)
    : mInitialised(false),
      mRunning(true),
      mSaveConfigOnExit(true),
      mGameName(game),
      mGameVersion(version),
      mBasePath(""),
      mPrefPath(""),
      mLogFile("engine.log"),
      mConfigFile("engine.cfg"),
      mMainCamera(nullptr)
{
    gEngine = this;

    // Get the preferences path
    char* prefPath = SDL_GetPrefPath("", mGameName.c_str());
    mPrefPath = prefPath;
    SDL_free(prefPath);

    // Get the base path
    char* basePath = SDL_GetBasePath();
    mBasePath = basePath;
    // HACK: Override pref path.
    mPrefPath = mBasePath;
    SDL_free(basePath);
#if DW_PLATFORM == DW_WIN32
    mBasePath = replaceString(mBasePath, "\\", "/");
#endif
    mBasePath += "../";

    // Change the working directory
#if DW_PLATFORM == DW_WIN32
    SetCurrentDirectoryA(mBasePath.c_str());
#else
    chdir(mBasePath.c_str());
#endif

    // Initialise logging
    new Log(mPrefPath + mLogFile);
    LOG << "Starting " << mGameName << " " << mGameVersion;
#ifdef DW_DEBUG
    LOGWARN << "This is a debug build!";
#endif
    printSystemInfo();
}

Engine::~Engine()
{
    shutdown();
}

void Engine::setup()
{
    assert(!mInitialised);

    SDL_Init(SDL_INIT_VIDEO);

    // Create EventSystem
    mEventSystem = new EventSystem;

    // Load configuration
    Config::load(mPrefPath + mConfigFile);

    // Initialise the Lua VM first so bindings can be defined in Constructors
    mLuaState = new LuaState;
    bindToLua();

    // Build window title
    String gameTitle(mGameName);
    gameTitle += " ";
    gameTitle += mGameVersion;
#ifdef DW_DEBUG
    gameTitle += " (debug)";
#endif

    // Create the engine systems
    mInput = new Input;
    mRenderer = new Renderer(mBasePath, mPrefPath, mInput, gameTitle);
    mUI = new UI(mRenderer, mInput, mLuaState);
    mInput->setViewportSize(mRenderer->getViewportSize());
    mAudio = new Audio;
    mPhysicsWorld = new PhysicsWorld(mRenderer);
    mSceneMgr = new SceneManager(mPhysicsWorld, mRenderer->getSceneMgr());
    mStarSystem = new StarSystem(mRenderer, mPhysicsWorld);
    mStateMgr = new StateManager;

    // Enumerate available video modes
    Vector<SDL_DisplayMode> displayModes = mRenderer->getDeviceDisplayModes();
    LOG << "Available video modes:";
    for (auto i = displayModes.begin(); i != displayModes.end(); i++)
    {
        LOG << "\t" << (*i).w << "x" << (*i).h << "@" << (*i).refresh_rate << "Hz"
        << " - Pixel Format: " << SDL_GetPixelFormatName((*i).format);
    }

    // TODO: move this to UI system
    SDL_StartTextInput();

    // The engine is now initialised
    mInitialised = true;

    // Register event delegate
    ADD_LISTENER(Engine, EvtData_Exit);
}

void Engine::shutdown()
{
    if (!mInitialised)
        return;

    // Save config
    if (mSaveConfigOnExit)
        Config::save();

    // Shutdown the engine
    SAFE_DELETE(mStateMgr);
    SAFE_DELETE(mUI);
    SAFE_DELETE(mStarSystem);
    SAFE_DELETE(mSceneMgr);
    SAFE_DELETE(mPhysicsWorld);
    SAFE_DELETE(mAudio);
    SAFE_DELETE(mInput);
    SAFE_DELETE(mRenderer);
    SAFE_DELETE(mLuaState);

	// Shut down the event system
	mEventSystem = nullptr;
	EventSystem::release();

	// Close the log
    Log::release();

    // Shut down SDL
    SDL_Quit();

    // The engine is no longer initialised
    mInitialised = false;
}

void Engine::run(EngineTickCallback tickFunc)
{
    // Start the main loop
    const float dt = 1.0f / 60.0f;
    double previousTime = time::now();
    double accumulator = 0.0;
    while (mRunning)
    {
        mUI->beginFrame();

        // Update game logic
        while (accumulator >= dt)
        {
            update(dt, mMainCamera);
            tickFunc(dt);
            accumulator -= dt;
        }

        // Render a frame
        preRender(mMainCamera);
        mRenderer->renderFrame(mMainCamera);

        // Calculate frameTime
        double currentTime = time::now();
        accumulator += currentTime - previousTime;
        previousTime = currentTime;
    }

    // Ensure that all states have been exited so no crashes occur later
    mStateMgr->clear();
}

void Engine::setMainCamera(Camera *camera)
{
    mMainCamera = camera;
}

void Engine::printSystemInfo()
{
    LOG << "\tPlatform: " << SDL_GetPlatform();
    LOG << "\tBase Path: " << mBasePath;
    LOG << "\tPreferences Path: " << mPrefPath;
    // TODO: more system info
}

void Engine::update(float dt, Camera* camera)
{
    mPhysicsWorld->update(dt, camera);
    mStarSystem->update(dt);
    mEventSystem->update((uint64_t)20);
    mAudio->update(dt, camera);
    mStateMgr->update(dt);
    mUI->update(dt);
    mSceneMgr->update(dt);
}

void Engine::preRender(Camera* camera)
{
    mStarSystem->preRender(camera);
    mSceneMgr->preRender(camera);
    mStateMgr->preRender();
    mUI->preRender();
}

void Engine::handleEvent(EventDataPtr eventData)
{
    assert(eventIs<EvtData_Exit>(eventData));
    mRunning = false;
}

// Lua functions
void Lua_EnterSandbox()
{
    gEngine->getStateMgr()->clear();
    gEngine->getStateMgr()->push(S_SANDBOX);
}

void Engine::bindToLua()
{
    mLuaState->bind()
        .addFunction("EnterSandbox", &Lua_EnterSandbox)
        .beginClass<Vec2>("Vec2")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(float, float)>()
        .addData("x", &Vec2::x)
        .addData("y", &Vec2::y)
        .endClass()
        .beginClass<Vec3>("Vec3")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(float, float, float)>()
        .addData("x", &Vec3::x)
        .addData("y", &Vec3::y)
        .addData("z", &Vec3::z)
        .addFunction("add", (Vec3 (Vec3::*)(const Vec3&) const) &Vec3::Add)
        .addFunction("sub", (Vec3 (Vec3::*)(const Vec3&) const) &Vec3::Sub)
        .addFunction("mul", (Vec3 (Vec3::*)(float) const) &Vec3::Mul)
        .addFunction("div", (Vec3 (Vec3::*)(float) const) &Vec3::Div)
        .addFunction("normalise", &Vec3::Normalize)
        .endClass()
        .beginClass<Vec4>("Vec4")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(float, float, float, float)>()
        .addData("x", &Vec4::x)
        .addData("y", &Vec4::y)
        .addData("z", &Vec4::z)
        .addData("w", &Vec4::w)
        .endClass()
        .beginClass<Quat>("Quat")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(Vec3, float)>()
        .addConstructor<void (*)(float, float, float, float)>()
        .addData("x", &Quat::x)
        .addData("y", &Quat::y)
        .addData("z", &Quat::z)
        .addData("w", &Quat::w)
        .endClass()
        .beginClass<Colour>("Colour")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(float, float, float)>()
        .addConstructor<void (*)(float, float, float, float)>()
        .addData("r", &Colour::r)
        .addData("g", &Colour::g)
        .addData("b", &Colour::b)
        .addData("a", &Colour::a)
        .endClass()
        .beginClass<Position>("Position")
        .addConstructor<void (*)(void)>()
        .addConstructor<void (*)(double, double, double)>()
        .addFunction("add", (Position & (Position::*)(const Vec3&)) & Position::operator+=)
        .addFunction("getRelativeToPoint", &Position::getRelativeTo)
        .addFunction("toCameraSpace", &Position::toCameraSpace)
        .addStaticFunction("fromCameraSpace", &Position::fromCameraSpace)
        .addData("x", &Position::x)
        .addData("y", &Position::y)
        .addData("z", &Position::z)
        .endClass();
}

NAMESPACE_END
