/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2015 (avedissian.david@gmail.com)
 */
#include "Common.h"
#include "SpriteManager.h"

#define MIN_HARDWARE_BUFFER_SIZE 120
#define WRITE_VERTEX(BUFFER, VERTEX) \
    *BUFFER = VERTEX.p.x;            \
    buffer++;                        \
    *BUFFER = VERTEX.p.y;            \
    buffer++;                        \
    *BUFFER = z;                     \
    buffer++;                        \
    *BUFFER = VERTEX.c.r;            \
    buffer++;                        \
    *BUFFER = VERTEX.c.g;            \
    buffer++;                        \
    *BUFFER = VERTEX.c.b;            \
    buffer++;                        \
    *BUFFER = VERTEX.c.a;            \
    buffer++;                        \
    *BUFFER = VERTEX.tc.x;           \
    buffer++;                        \
    *BUFFER = VERTEX.tc.y;           \
    buffer++;
#define WRITE_TRIANGLE(BUFFER, SPRITE, V0, V1, V2) \
    WRITE_VERTEX(BUFFER, SPRITE.vertex[V0])        \
    WRITE_VERTEX(BUFFER, SPRITE.vertex[V1])        \
    WRITE_VERTEX(BUFFER, SPRITE.vertex[V2])

NAMESPACE_BEGIN

SpriteManager::SpriteManager(Ogre::Viewport* viewport, Ogre::SceneManager* sceneMgr)
    : mRenderSystem(Ogre::Root::getSingleton().getRenderSystem()),
      mViewport(viewport),
      mSceneManager(sceneMgr),
      mSpriteCount(0)
{
    mHwBuffer.setNull();
    sceneMgr->addRenderQueueListener(this);
}

SpriteManager::~SpriteManager()
{
    mSceneManager->removeRenderQueueListener(this);
    if (!mHwBuffer.isNull())
        DestroyHardwareBuffer();
}

void SpriteManager::renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation,
                                     bool& repeatThisInvocation)
{
    if (mRenderSystem->_getViewport() == mViewport && queueGroupId == Ogre::RENDER_QUEUE_OVERLAY)
        Render();
}

void SpriteManager::LoadSprite(const string& textureName)
{
    // Set up the material
    string materialName = "Sprite/" + textureName;
    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().getByName(materialName);
    if (material.isNull())
    {
        material = Ogre::MaterialManager::getSingleton().create(
            materialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
        pass->setLightingEnabled(false);
        pass->setDepthCheckEnabled(false);
        pass->setDepthWriteEnabled(false);
        pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

        Ogre::TextureUnitState* texUnit = pass->createTextureUnitState();
        texUnit->setTextureName(textureName);
        texUnit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
        texUnit->setTextureFiltering(Ogre::FO_NONE, Ogre::FO_NONE, Ogre::FO_NONE);
        texUnit->setHardwareGammaEnabled(true);

        // Load material
        material->load();
    }
}

void SpriteManager::DrawSprite(const string& textureName, const Vec2& position,
                               SpriteOrigin origin /*= SO_TOP_LEFT*/, float angle /*= 0.0f*/)
{
    // Ensure that the material exists
    LoadSprite(textureName);

    // Determine size
    Ogre::TexturePtr tp = Ogre::TextureManager::getSingleton().getByName(textureName);
    Vec2 size(tp->getWidth(), tp->getHeight());

    // Calculate origin based on size
    Vec2 originCoord;
    switch (origin)
    {
    case SO_TOP_LEFT:
        originCoord = Vec2(0.0f, 0.0f);
        break;

    case SO_TOP_RIGHT:
        originCoord = Vec2(size.x, 0.0f);
        break;

    case SO_BOTTOM_LEFT:
        originCoord = Vec2(0.0f, size.y);
        break;

    case SO_BOTTOM_RIGHT:
        originCoord = size;
        break;

    case SO_CENTRE:
        originCoord = size * 0.5f;
        break;

    default:
        break;
    }

    // Call drawSprite with these parameters
    SpriteDesc s;
    s.textureName = textureName;
    s.position = position;
    s.size = size;
    s.uv = {Vec2(0.0f), Vec2(1.0f)};
    s.colour = Colour(1.0f, 1.0f, 1.0f, 1.0f);
    s.origin = originCoord;
    s.angle = angle;
    DrawSprite(s);
}

void SpriteManager::DrawSprite(const string& textureName, const Vec2& position, const Vec2& origin,
                               float angle /*= 0.0f*/)
{
    // Ensure that the material exists
    LoadSprite(textureName);

    // Determine size
    Ogre::TexturePtr tp = Ogre::TextureManager::getSingleton().getByName(textureName);
    Vec2 size(tp->getWidth(), tp->getHeight());

    // Call drawSprite with these parameters
    SpriteDesc s;
    s.textureName = textureName;
    s.position = position;
    s.size = size;
    s.uv = {Vec2(0.0f), Vec2(1.0f)};
    s.colour = Colour(1.0f, 1.0f, 1.0f, 1.0f);
    s.origin = origin;
    s.angle = angle;
    DrawSprite(s);
}

void SpriteManager::DrawSprite(const string& textureName, const Vec2& position, const Vec2& size,
                               const Vec2& origin, float angle /*= 0.0f*/)
{
    // Ensure that the material exists
    LoadSprite(textureName);

    // Call drawSprite with these parameters
    SpriteDesc s;
    s.textureName = textureName;
    s.position = position;
    s.size = size;
    s.uv = {Vec2(0.0f), Vec2(1.0f)};
    s.colour = Colour(1.0f, 1.0f, 1.0f, 1.0f);
    s.origin = origin;
    s.angle = angle;
    DrawSprite(s);
}

void SpriteManager::DrawSprite(const SpriteDesc& s)
{
    // Ensure that the material exists
    LoadSprite(s.textureName);

    // Calculate corners
    // Vertex order:
    //  0--1
    //  | /|
    //  |/ |
    //  2--3
    SpriteChunk::Vertex c[4] = {
        {Vec2(0.0f, 0.0f) - s.origin, s.colour, Vec2(s.uv.begin.x, s.uv.begin.y)},
        {Vec2(s.size.x, 0.0f) - s.origin, s.colour, Vec2(s.uv.end.x, s.uv.begin.y)},
        {Vec2(0.0f, s.size.y) - s.origin, s.colour, Vec2(s.uv.begin.x, s.uv.end.y)},
        {s.size - s.origin, s.colour, Vec2(s.uv.end.x, s.uv.end.y)}};

    // Set up the sprite
    SpriteChunk spr;
    for (int i = 0; i < 4; ++i)
    {
        // Transform point
        if (!math::EqualAbs(s.angle, 0.0f))
            c[i].p = Rotate(c[i].p, s.angle);
        c[i].p = ToScreenCoord(c[i].p + s.position);

        // Copy vertex
        spr.vertex[i] = c[i];
    }

    // Add to sprites list
    mChunks[s.textureName].push_back(spr);
    mSpriteCount++;
}

void SpriteManager::Render()
{
    uint newSize = mSpriteCount * 6;
    if (newSize < MIN_HARDWARE_BUFFER_SIZE)
        newSize = MIN_HARDWARE_BUFFER_SIZE;

    // Grow hardware buffer if needed
    if (mHwBuffer.isNull() || mHwBuffer->getNumVertices() < newSize)
    {
        if (!mHwBuffer.isNull())
            DestroyHardwareBuffer();
        CreateHardwareBuffer(newSize);
    }

    // Bail if there are no sprites to render
    if (mSpriteCount == 0)
        return;

    // Lock hardware buffer
    float z = -1.0f;
    float* buffer = (float*)mHwBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);

    // Write quads to the hardware buffer
    std::vector<VertexChunk> chunks;
    for (auto p = mChunks.begin(); p != mChunks.end(); ++p)
    {
        VertexChunk thisChunk;
        thisChunk.vertexCount = 0;
        thisChunk.pass = Ogre::MaterialManager::getSingleton()
                             .getByName("Sprite/" + p->first)
                             ->getTechnique(0)
                             ->getPass(0);

        // Write vertices
        for (auto s = p->second.begin(); s != p->second.end(); ++s)
        {
            auto& sprite = *s;
            WRITE_TRIANGLE(buffer, sprite, 0, 2, 1);
            WRITE_TRIANGLE(buffer, sprite, 1, 2, 3);
            thisChunk.vertexCount += 6;
        }

        // Add to chunks
        chunks.push_back(thisChunk);
    }

    mHwBuffer->unlock();

    // Prepare the render system
    mRenderSystem->_setWorldMatrix(Ogre::Matrix4::IDENTITY);
    mRenderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);
    mRenderSystem->_setProjectionMatrix(Ogre::Matrix4::IDENTITY);
    mRenderOp.vertexData->vertexStart = 0;

    // Render the sprites
    for (auto c = chunks.begin(); c != chunks.end(); c++)
    {
        mRenderOp.vertexData->vertexCount = c->vertexCount;
        mSceneManager->_setPass(c->pass);
        mRenderSystem->_render(mRenderOp);
        mRenderOp.vertexData->vertexStart += c->vertexCount;
    }

    // Clear sprites list
    mChunks.clear();
    mSpriteCount = 0;
}

void SpriteManager::CreateHardwareBuffer(uint size)
{
    mRenderOp.vertexData = new Ogre::VertexData;
    mRenderOp.vertexData->vertexStart = 0;

    size_t offset = 0;
    Ogre::VertexDeclaration* vd = mRenderOp.vertexData->vertexDeclaration;

    vd->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
    vd->addElement(0, offset, Ogre::VET_FLOAT4, Ogre::VES_DIFFUSE);
    offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT4);
    vd->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

    mHwBuffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
        vd->getVertexSize(0), size, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE,
        false);

    mRenderOp.vertexData->vertexBufferBinding->setBinding(0, mHwBuffer);
    mRenderOp.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    mRenderOp.useIndexes = false;
}

void SpriteManager::DestroyHardwareBuffer()
{
    delete mRenderOp.vertexData;
    mRenderOp.vertexData = nullptr;
    mHwBuffer.setNull();
}

Vec2 SpriteManager::Rotate(const Vec2& c, float angle)
{
    // This is just an expansion of the matrix multiplication:
    // |x'| = |cos(a), -sin(a)||x|
    // |y'| = |sin(a),  cos(a)||y|
    Vec2 out;
    out.x = math::Cos(angle) * c.x - math::Sin(angle) * c.y;
    out.y = math::Sin(angle) * c.x + math::Cos(angle) * c.y;
    return out;
}

Vec2 SpriteManager::ToScreenCoord(const Vec2& pos)
{
    float vpWidth = (float)mViewport->getActualWidth();
    float vpHeight = (float)mViewport->getActualHeight();
    float vpHalfWidth = vpWidth * 0.5f;
    float vpHalfHeight = vpHeight * 0.5f;
    return Vec2(pos.x / vpHalfWidth - 1.0f, 1.0f - pos.y / vpHalfHeight);
}

NAMESPACE_END
