/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2019 (git@dga.dev)
 */
#include "Base.h"
#include "core/io/InputStream.h"
#include "renderer/Mesh.h"
#include "renderer/Renderer.h"
#include "core/StringUtils.h"

#define ASSIMP_BUILD_BOOST_WORKAROUND
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>
#include <assimp/Logger.hpp>
#include <assimp/postprocess.h>
#include "resource/ResourceCache.h"

namespace dw {
namespace {
class DawnAssimpLogStream : public Assimp::LogStream {
public:
    DawnAssimpLogStream(Context* ctx) : logger{ctx->module<Logger>()} {
    }
    ~DawnAssimpLogStream() = default;

    void write(const char* message_cstr) {
        String message{message_cstr};
        logger->withObjectName("Mesh").info("Assimp Importer: {}",
                                            message.substr(0, message.length() - 1));
    }

private:
    Logger* logger;
};
}  // namespace

Mesh::Node::Node(Mat4 transform, Node* parent, Vector<SubMesh*> submeshes)
    : transform_(transform), parent_(parent), submeshes_(submeshes) {
}

void Mesh::Node::addChild(UniquePtr<Node> node) {
    children_.emplace_back(std::move(node));
}

Mesh::Node* Mesh::Node::child(int i) {
    return children_[i].get();
}

usize Mesh::Node::childCount() const {
    return children_.size();
}

void Mesh::Node::setTransform(const Mat4& transform) {
    transform_ = transform;
}

const Mat4& Mesh::Node::transform() const {
    return transform_;
}

void Mesh::Node::draw(Renderer* renderer, uint view, const Mat4& model_matrix,
                      const Mat4& view_projection_matrix) {
    const Mat4 current_model_matrix = model_matrix * transform_;
    for (auto& submesh : submeshes_) {
        submesh->draw(renderer, view, current_model_matrix, view_projection_matrix);
    }
    for (auto& child : children_) {
        child->draw(renderer, view, current_model_matrix, view_projection_matrix);
    }
}

Mesh::SubMesh::SubMesh(usize index_buffer_offset, usize index_count, SharedPtr<Material> material)
    : index_buffer_offset_(index_buffer_offset), index_count_(index_count), material_(material) {
}

void Mesh::SubMesh::draw(Renderer* renderer, uint view, const Mat4& model_matrix,
                         const Mat4& view_projection_matrix) {
    // TODO: Revamp material system.
    material_->applyRendererState(model_matrix, view_projection_matrix);
    renderer->rhi()->setStateDisable(gfx::RenderState::CullFace);
    renderer->rhi()->submit(view, material_->program()->internalHandle(), index_count_,
                            index_buffer_offset_);
}

Mesh::Mesh(Context* context)
    : Resource(context), vertex_buffer_(nullptr), index_buffer_(nullptr), root_node_(nullptr) {
}

Mesh::~Mesh() {
}

Result<void> Mesh::beginLoad(const String& asset_name, InputStream& is) {
    // Read stream.
    assert(is.size() > 0);
    u64 size = is.size();
    UniquePtr<byte[]> data{new byte[size]};
    is.readData(data.get(), size);
    assert(is.eof());

    const unsigned int severity = Assimp::Logger::Debugging | Assimp::Logger::Info |
                                  Assimp::Logger::Warn | Assimp::Logger::Err;

    // Run importer.
    Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE, 0);
    Assimp::DefaultLogger::get()->attachStream(new DawnAssimpLogStream(context()), severity);
    Assimp::Importer importer;
    auto flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate |
                 aiProcess_JoinIdenticalVertices | aiProcess_SortByPType;
    const aiScene* scene = importer.ReadFileFromMemory(data.get(), size, flags, asset_name.c_str());
    Assimp::DefaultLogger::kill();
    if (!scene) {
        return makeError(str::format("Unable to load mesh {}. Reason: {}", asset_name,
                                     importer.GetErrorString()));
    }

    // TODO: Load materials.
    // For now, load a basic material.
    material_ = makeShared<Material>(
        context(),
        makeShared<Program>(
            context(),
            *module<ResourceCache>()->get<VertexShader>("base:materials/basic-no-texture.vs"),
            *module<ResourceCache>()->get<FragmentShader>("base:materials/basic-no-texture.fs")));
    material_->program()->setUniform("light_direction", Vec3{1.0f, 1.0f, 1.0f}.Normalized());

    // Build a vertex and index buffer containing all the mesh data.
    struct Vertex {
        Vec3 position;
        Vec3 normal;
    };
    Vector<Vertex> vertices;
    Vector<u32> indices;
    for (uint i = 0; i < scene->mNumMeshes; ++i) {
        const auto* mesh = scene->mMeshes[i];

        // Check the mesh for any issues, and abort if so.
        if (!mesh->HasPositions()) {
            return makeError(
                str::format("Unable to load mesh {}. Submesh {} has no positions.", asset_name, i));
        }
        if (!mesh->HasNormals()) {
            return makeError(
                str::format("Unable to load mesh {}. Submesh {} has no normals.", asset_name, i));
        }
        if (!mesh->HasFaces()) {
            return makeError(
                str::format("Unable to load mesh {}. Submesh {} has no faces.", asset_name, i));
        }

        const u32 vertex_offset = static_cast<u32>(vertices.size());
        const u32 index_offset = static_cast<u32>(indices.size());

        for (usize v = 0; v < mesh->mNumVertices; ++v) {
            aiVector3D& position = mesh->mVertices[v];
            aiVector3D& normal = mesh->mNormals[v];
            vertices.emplace_back(
                Vertex{{position.x, position.y, position.z}, {normal.x, normal.y, normal.z}});
        }
        for (usize f = 0; f < mesh->mNumFaces; ++f) {
            aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                return makeError(str::format(
                    "Unable to load mesh {}. Face {} in submesh {} has {} indices (must be exactly "
                    "3).",
                    asset_name, f, i, face.mNumIndices));
            }
            indices.emplace_back(face.mIndices[0] + vertex_offset);
            indices.emplace_back(face.mIndices[1] + vertex_offset);
            indices.emplace_back(face.mIndices[2] + vertex_offset);
        }

        auto submesh = makeUnique<SubMesh>(index_offset, mesh->mNumFaces * 3, material_);
        submeshes_.emplace_back(std::move(submesh));
    }

    // Build GPU buffers.
    gfx::VertexDecl decl;
    decl.begin()
        .add(gfx::VertexDecl::Attribute::Position, 3, gfx::VertexDecl::AttributeType::Float)
        .add(gfx::VertexDecl::Attribute::Normal, 3, gfx::VertexDecl::AttributeType::Float)
        .end();
    vertex_buffer_ =
        makeShared<VertexBuffer>(context(), gfx::Memory(vertices), vertices.size(), decl);
    index_buffer_ =
        makeShared<IndexBuffer>(context(), gfx::Memory(indices), gfx::IndexBufferType::U32);

    // Set up node hierarchy.
    Function<UniquePtr<Node>(aiNode*, Node*)> create_node_tree =
        [this, &create_node_tree](aiNode* ai_node, Node* parent) -> UniquePtr<Node> {
        // Convert transform.
        const aiMatrix4x4 ai_transform = ai_node->mTransformation;
        Mat4 transform;
        transform[0][0] = ai_transform.a1;
        transform[0][1] = ai_transform.a2;
        transform[0][2] = ai_transform.a3;
        transform[0][3] = ai_transform.a4;
        transform[1][0] = ai_transform.b1;
        transform[1][1] = ai_transform.b2;
        transform[1][2] = ai_transform.b3;
        transform[1][3] = ai_transform.b4;
        transform[2][0] = ai_transform.c1;
        transform[2][1] = ai_transform.c2;
        transform[2][2] = ai_transform.c3;
        transform[2][3] = ai_transform.c4;
        transform[3][0] = ai_transform.d1;
        transform[3][1] = ai_transform.d2;
        transform[3][2] = ai_transform.d3;
        transform[3][3] = ai_transform.d4;

        // Create node.
        Vector<SubMesh*> submeshes;
        submeshes.resize(ai_node->mNumMeshes);
        for (usize i = 0; i < ai_node->mNumMeshes; ++i) {
            submeshes[i] = submeshes_[ai_node->mMeshes[i]].get();
        }
        auto node = makeUnique<Node>(transform, parent, submeshes);

        // Add children.
        for (usize c = 0; c < ai_node->mNumChildren; ++c) {
            node->addChild(create_node_tree(ai_node->mChildren[c], node.get()));
        }

        return node;
    };
    root_node_ = create_node_tree(scene->mRootNode, nullptr);

    return Result<void>();
}

void Mesh::draw(Renderer* renderer, uint view, detail::Transform&, const Mat4& model_matrix,
                const Mat4& view_projection_matrix) {
    // usize vertex_count = index_buffer_->indexCount();
    auto rhi = renderer->rhi();
    rhi->setVertexBuffer(vertex_buffer_->internalHandle());
    rhi->setIndexBuffer(index_buffer_->internalHandle());
    // TODO: Move this common "render vertex/index buffer + material" code somewhere to avoid
    // duplication with CustomMeshRenderable.
    // TODO: Support unset material.

    root_node_->draw(renderer, view, model_matrix, view_projection_matrix);
}

Mesh::Node* Mesh::rootNode() {
    return root_node_.get();
}
}  // namespace dw
