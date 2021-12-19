//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "TVRayTracer.h"
#include "XUSGObjLoader.h"
#define _INDEPENDENT_DDS_LOADER_
#include "Advanced/XUSGDDSLoader.h"
#undef _INDEPENDENT_DDS_LOADER_
#include "DirectXPackedVector.h"

#define USE_COMPLEX_TESSELLATOR 0

using namespace std;
using namespace DirectX;
using namespace XUSG;
using namespace XUSG::RayTracing;

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT3 Nrm;
};

struct RayGenConstants
{
    XMMATRIX ProjToWorld;
    XMVECTOR EyePt;
};

struct CBGlobal
{
    XMFLOAT3X4 WorldITs[TVRayTracer::NUM_MESH - 1];
    float      WorldIT[11];
    uint32_t   FrameIndex;
};

struct CBBasePass
{
    XMFLOAT4X4 WorldViewProj;
    XMFLOAT4X4 WorldViewProjPrev;
    XMFLOAT3X4 WorldIT;
    XMFLOAT2   ProjBias;
};

struct CBMaterial
{
    XMFLOAT4 BaseColors[TVRayTracer::NUM_MESH];
    XMFLOAT4 RoughMetals[TVRayTracer::NUM_MESH];
};

const wchar_t* TVRayTracer::HitGroupNames[] = { L"hitGroupReflection", L"hitGroupDiffuse" };
const wchar_t* TVRayTracer::RaygenShaderName = L"raygenMain";
const wchar_t* TVRayTracer::ClosestHitShaderNames[] = { L"closestHitReflection", L"closestHitDiffuse" };
const wchar_t* TVRayTracer::MissShaderName = L"missMain";

TVRayTracer::TVRayTracer(const RayTracing::Device::sptr& device) :
    m_device(device),
    m_instances()
{
    m_shaderPool = ShaderPool::MakeUnique();
    m_rayTracingPipelineCache = RayTracing::PipelineCache::MakeUnique(device.get());
    m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(device.get());
    m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(device.get());
    m_descriptorTableCache = DescriptorTableCache::MakeUnique(device.get(), L"RayTracerDescriptorTableCache");

    AccelerationStructure::SetUAVCount(NUM_MESH + NUM_HIT_GROUP + 1);
}

TVRayTracer::~TVRayTracer()
{
}

bool TVRayTracer::Init(
    RayTracing::CommandList* pCommandList,
    uint32_t                 width,
    uint32_t                 height,
    vector<Resource::uptr>& uploaders,
    GeometryBuffer* pGeometries,
    const char* fileName,
    const wchar_t* envFileName,
    Format                   rtFormat,
    const XMFLOAT4& posScale,
    uint8_t                  maxGBufferMips)
{
    m_viewport = XMUINT2(width, height);
    m_posScale = posScale;

    // Load inputs
    ObjLoader objLoader;
    if (!objLoader.Import(fileName, true, true)) return false;
    auto numVertices = objLoader.GetNumVertices();
    auto numIndices = objLoader.GetNumIndices();
    N_RETURN(createVB(pCommandList, numVertices, objLoader.GetVertexStride(), objLoader.GetVertices(), uploaders), false);
    N_RETURN(createIB(pCommandList, numIndices, objLoader.GetIndices(), uploaders), false);

    N_RETURN(createGroundMesh(pCommandList, uploaders), false);

    // Create output view
    {
        m_outputView = Texture2D::MakeUnique();
        N_RETURN(m_outputView->Create(m_device.get(), width, height, Format::R11G11B10_FLOAT, 1,
            ResourceFlag::ALLOW_UNORDERED_ACCESS, 1, 1, false, MemoryFlag::NONE,
            L"RayTracingOut"), false);
    }

    uint8_t mipCount = max<uint8_t>(Log2((max)(width, height)), 0) + 1;
    mipCount = (min)(mipCount, maxGBufferMips);
    const auto resourceFlags = mipCount > 1 ? ResourceFlag::ALLOW_UNORDERED_ACCESS : ResourceFlag::NONE;
    for (auto& renderTarget : m_gbuffers) renderTarget = RenderTarget::MakeUnique();
    N_RETURN(m_gbuffers[BASE_COLOR]->Create(m_device.get(), width, height, Format::R8G8B8A8_UNORM,
        1, resourceFlags, 1, 1, nullptr, false, MemoryFlag::NONE, L"BaseColor"), false);
    N_RETURN(m_gbuffers[NORMAL]->Create(m_device.get(), width, height, Format::R10G10B10A2_UNORM,
        1, resourceFlags, mipCount, 1, nullptr, false, MemoryFlag::NONE, L"Normal"), false);
    N_RETURN(m_gbuffers[ROUGH_METAL]->Create(m_device.get(), width, height, Format::R8G8_UNORM,
        1, resourceFlags, mipCount, 1, nullptr, false, MemoryFlag::NONE, L"RoughnessMetallic"), false);
    N_RETURN(m_gbuffers[VELOCITY]->Create(m_device.get(), width, height, Format::R16G16_FLOAT,
        1, ResourceFlag::NONE, 1, 1, nullptr, false, MemoryFlag::NONE, L"Velocity"), false);

    const auto dsFormat = Format::D24_UNORM_S8_UINT;
    m_depth = DepthStencil::MakeShared();
    N_RETURN(m_depth->Create(m_device.get(), width, height, dsFormat, ResourceFlag::NONE,
        1, 1, 1, 1.0f, 0, false, MemoryFlag::NONE, L"Depth"), false);

    // Constant buffers
    for (auto i = 0u; i < NUM_MESH; ++i)
    {
        auto& cbBasePass = m_cbBasePass[i];
        cbBasePass = ConstantBuffer::MakeUnique();
        N_RETURN(cbBasePass->Create(m_device.get(), sizeof(CBBasePass[FrameCount]), FrameCount, nullptr,
            MemoryType::UPLOAD, MemoryFlag::NONE, (L"CBBasePass" + to_wstring(i)).c_str()), false);
    }

    m_cbRaytracing = ConstantBuffer::MakeUnique();
    N_RETURN(m_cbRaytracing->Create(m_device.get(), sizeof(CBGlobal[FrameCount]), FrameCount,
        nullptr, MemoryType::UPLOAD, MemoryFlag::NONE, L"CBGlobal"), false);

    m_cbMaterials = ConstantBuffer::MakeUnique();
    N_RETURN(m_cbMaterials->Create(m_device.get(), sizeof(CBMaterial), 1,
        nullptr, MemoryType::UPLOAD, MemoryFlag::NONE, L"CBMaterial"), false);
    {
        const auto pCbData = reinterpret_cast<CBMaterial*>(m_cbMaterials->Map());
        const auto Silver = XMFLOAT4(0.95f, 0.93f, 0.88f, 1.0f);
        const auto RoseGold1 = XMFLOAT4(0.84f, 0.45f, 0.42f, 1.0f);
        const auto RoseGold2 = XMFLOAT4(0.72f, 0.43f, 0.47f, 1.0f);
        const auto Gold = XMFLOAT4(1.0f, 0.71f, 0.29f, 1.0f);
        pCbData->BaseColors[GROUND] = Silver;
        pCbData->BaseColors[MODEL_OBJ] = Gold;
        pCbData->RoughMetals[GROUND] = XMFLOAT4(0.5f, 1.0f, 0.0f, 0.0f);
        pCbData->RoughMetals[MODEL_OBJ] = XMFLOAT4(0.16f, 1.0f, 0.0f, 0.0f);
    }

    // Load input image
    {
        DDS::Loader textureLoader;
        DDS::AlphaMode alphaMode;

        uploaders.emplace_back(Resource::MakeUnique());
        N_RETURN(textureLoader.CreateTextureFromFile(m_device.get(), static_cast<XUSG::CommandList*>(pCommandList), envFileName,
            8192, false, m_lightProbe, uploaders.back().get(), &alphaMode), false);
    }

    // Create raytracing pipelines
    N_RETURN(createInputLayout(), false);
    N_RETURN(createPipelineLayouts(), false);
    N_RETURN(createPipelines(rtFormat, dsFormat), false);

    // Build acceleration structures
    N_RETURN(buildAccelerationStructures(pCommandList, pGeometries), false);
    N_RETURN(buildShaderTables(), false);

    return true;
}

static const XMFLOAT2& IncrementalHalton()
{
    static auto haltonBase = XMUINT2(0, 0);
    static auto halton = XMFLOAT2(0.0f, 0.0f);

    // Base 2
    {
        // Bottom bit always changes, higher bits
        // Change less frequently.
        auto change = 0.5f;
        auto oldBase = haltonBase.x++;
        auto diff = haltonBase.x ^ oldBase;

        // Diff will be of the form 0*1+, i.e. one bits up until the last carry.
        // Expected iterations = 1 + 0.5 + 0.25 + ... = 2
        do
        {
            halton.x += (oldBase & 1) ? -change : change;
            change *= 0.5f;

            diff = diff >> 1;
            oldBase = oldBase >> 1;
        } while (diff);
    }

    // Base 3
    {
        const auto oneThird = 1.0f / 3.0f;
        auto mask = 0x3u;	// Also the max base 3 digit
        auto add = 0x1u;	// Amount to add to force carry once digit == 3
        auto change = oneThird;
        ++haltonBase.y;

        // Expected iterations: 1.5
        while (true)
        {
            if ((haltonBase.y & mask) == mask)
            {
                haltonBase.y += add;	// Force carry into next 2-bit digit
                halton.y -= 2 * change;

                mask = mask << 2;
                add = add << 2;

                change *= oneThird;
            }
            else
            {
                halton.y += change;	// We know digit n has gone from a to a + 1
                break;
            }
        };
    }

    return halton;
}

void TVRayTracer::UpdateFrame(
    uint8_t   frameIndex,
    CXMVECTOR eyePt,
    CXMMATRIX viewProj,
    float     timeStep)
{
    const auto halton = IncrementalHalton();
    XMFLOAT2 projBias =
    {
        (halton.x * 2.0f - 1.0f) / m_viewport.x,
        (halton.y * 2.0f - 1.0f) / m_viewport.y
    };

    {
        const auto projToWorld = XMMatrixInverse(nullptr, viewProj);
        RayGenConstants cbRayGen = { XMMatrixTranspose(projToWorld), eyePt };

        m_rayGenShaderTables[frameIndex]->Reset();
        m_rayGenShaderTables[frameIndex]->AddShaderRecord(ShaderRecord::MakeUnique(m_device.get(),
            m_pipelines[RAY_TRACING], RaygenShaderName, &cbRayGen, sizeof(RayGenConstants)).get());
    }

    {
        static auto angle = 0.0f;
        angle += 16.0f * timeStep * XM_PI / 180.0f;
        const auto rot = XMMatrixRotationY(angle);

        {
            static auto s_frameIndex = 0u;
            const auto pCbData = reinterpret_cast<CBGlobal*>(m_cbRaytracing->Map(frameIndex));
            const auto n = 256u;
            for (auto i = 0u; i < NUM_MESH; ++i) XMStoreFloat3x4(&pCbData->WorldITs[i], i ? rot : XMMatrixIdentity());
            pCbData->FrameIndex = s_frameIndex++;
            s_frameIndex %= n;
        }

        XMMATRIX worlds[NUM_MESH] =
        {
            XMMatrixScaling(10.0f, 0.5f, 10.0f) * XMMatrixTranslation(0.0f, -0.5f, 0.0f),
            XMMatrixScaling(m_posScale.w, m_posScale.w, m_posScale.w) * rot *
            XMMatrixTranslation(m_posScale.x, m_posScale.y, m_posScale.z)
        };

        for (auto i = 0u; i < NUM_MESH; ++i)
        {
            const auto pCbData = reinterpret_cast<CBBasePass*>(m_cbBasePass[i]->Map(frameIndex));
            pCbData->ProjBias = projBias;
            pCbData->WorldViewProjPrev = m_worldViewProjs[i];
            XMStoreFloat4x4(&m_worlds[i], XMMatrixTranspose(worlds[i]));
            XMStoreFloat4x4(&pCbData->WorldViewProj, XMMatrixTranspose(worlds[i] * viewProj));
            XMStoreFloat3x4(&pCbData->WorldIT, i ? rot : XMMatrixIdentity());
            m_worldViewProjs[i] = pCbData->WorldViewProj;
        }
    }
}

void TVRayTracer::Render(
    const RayTracing::CommandList* pCommandList,
    uint8_t                        frameIndex)
{
    renderGeometry(pCommandList, frameIndex);
    rayTrace(pCommandList, frameIndex);
}

void TVRayTracer::UpdateAccelerationStructures(
    const RayTracing::CommandList* pCommandList,
    uint8_t                        frameIndex)
{
    // Set instance
    float* const transforms[] =
    {
        reinterpret_cast<float*>(&m_worlds[GROUND]),
        reinterpret_cast<float*>(&m_worlds[MODEL_OBJ])
    };
    const BottomLevelAS* pBottomLevelASs[NUM_MESH];
    for (auto i = 0u; i < NUM_MESH; ++i) pBottomLevelASs[i] = m_bottomLevelASs[i].get();
    TopLevelAS::SetInstances(m_device.get(), m_instances[frameIndex].get(), NUM_MESH, pBottomLevelASs, transforms);

    // Update top level AS
    const auto& descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);
    m_topLevelAS->Build(pCommandList, m_scratch.get(), m_instances[frameIndex].get(), descriptorPool, true);
}

void TVRayTracer::renderGeometry(
    const RayTracing::CommandList* pCommandList,
    uint8_t                        frameIndex)
{
    // Bind the heaps
    const DescriptorPool descriptorPools[] =
    {
        m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL),
        m_descriptorTableCache->GetDescriptorPool(SAMPLER_POOL)
    };
    pCommandList->SetDescriptorPools(static_cast<uint32_t>(size(descriptorPools)), descriptorPools);

    zPrepass(pCommandList, frameIndex);
    gbufferPass(pCommandList, frameIndex);

    ResourceBarrier barriers[1 + NUM_GBUFFER];
    auto numBarriers = 0u;
    numBarriers = m_outputView->SetBarrier(barriers, ResourceState::UNORDERED_ACCESS, numBarriers);
    for (uint8_t i = 0; i < VELOCITY; ++i)
        numBarriers = m_gbuffers[i]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE, numBarriers, 0);
    numBarriers = m_gbuffers[VELOCITY]->SetBarrier(barriers, ResourceState::NON_PIXEL_SHADER_RESOURCE,
        numBarriers, BARRIER_ALL_SUBRESOURCES, BarrierFlag::BEGIN_ONLY);
    pCommandList->Barrier(numBarriers, barriers);
}

const Texture2D* TVRayTracer::GetRayTracingOutput() const
{
    return m_outputView.get();
}

bool TVRayTracer::createVB(
    RayTracing::CommandList* pCommandList,
    uint32_t                 numVert,
    uint32_t                 stride,
    const uint8_t* pData,
    vector<Resource::uptr>& uploaders)
{
    auto& vertexBuffer = m_vertexBuffers[MODEL_OBJ];
    vertexBuffer = VertexBuffer::MakeUnique();
    N_RETURN(vertexBuffer->Create(m_device.get(), numVert, sizeof(Vertex),
        ResourceFlag::NONE, MemoryType::DEFAULT, 1, nullptr, 1,
        nullptr, 1, nullptr, MemoryFlag::NONE, L"MeshVB"), false);

    uploaders.emplace_back(Resource::MakeUnique());
    return vertexBuffer->Upload(pCommandList, uploaders.back().get(), pData,
        sizeof(Vertex) * numVert, 0, ResourceState::NON_PIXEL_SHADER_RESOURCE);
}

bool TVRayTracer::createIB(
    RayTracing::CommandList* pCommandList,
    uint32_t                 numIndices,
    const uint32_t* pData,
    vector<Resource::uptr>& uploaders)
{
    m_numIndices[MODEL_OBJ] = numIndices;

    auto& indexBuffer = m_indexBuffers[MODEL_OBJ];
    const uint32_t byteWidth = sizeof(uint32_t) * numIndices;
    indexBuffer = IndexBuffer::MakeUnique();
    N_RETURN(indexBuffer->Create(m_device.get(), byteWidth, Format::R32_UINT, ResourceFlag::NONE,
        MemoryType::DEFAULT, 1, nullptr, 1, nullptr, 1, nullptr, MemoryFlag::NONE, L"MeshIB"), false);

    uploaders.emplace_back(Resource::MakeUnique());
    return indexBuffer->Upload(pCommandList, uploaders.back().get(), pData,
        byteWidth, 0, ResourceState::NON_PIXEL_SHADER_RESOURCE);
}

bool TVRayTracer::createGroundMesh(
    RayTracing::CommandList* pCommandList,
    vector<Resource::uptr>& uploaders)
{
    const auto Scale = 1.0f;
    // Vertex buffer
    {
        // Cube vertices positions and corresponding triangle normals.
        Vertex vertices[] =
        {
            { XMFLOAT3(-Scale, Scale, -Scale), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(Scale, Scale, -Scale), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(Scale, Scale, Scale), XMFLOAT3(0.0f, 1.0f, 0.0f) },
            { XMFLOAT3(-Scale, Scale, Scale), XMFLOAT3(0.0f, 1.0f, 0.0f) },

            { XMFLOAT3(-Scale, -Scale, -Scale), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3(Scale, -Scale, -Scale), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3(Scale, -Scale, Scale), XMFLOAT3(0.0f, -1.0f, 0.0f) },
            { XMFLOAT3(-Scale, -Scale, Scale), XMFLOAT3(0.0f, -1.0f, 0.0f) },

            { XMFLOAT3(-Scale, -Scale, Scale), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-Scale, -Scale, -Scale), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-Scale, Scale, -Scale), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(-Scale, Scale, Scale), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

            { XMFLOAT3(Scale, -Scale, Scale), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(Scale, -Scale, -Scale), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(Scale, Scale, -Scale), XMFLOAT3(1.0f, 0.0f, 0.0f) },
            { XMFLOAT3(Scale, Scale, Scale), XMFLOAT3(1.0f, 0.0f, 0.0f) },

            { XMFLOAT3(-Scale, -Scale, -Scale), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3(Scale, -Scale, -Scale), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3(Scale, Scale, -Scale), XMFLOAT3(0.0f, 0.0f, -1.0f) },
            { XMFLOAT3(-Scale, Scale, -Scale), XMFLOAT3(0.0f, 0.0f, -1.0f) },

            { XMFLOAT3(-Scale, -Scale, Scale), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3(Scale, -Scale, Scale), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3(Scale, Scale, Scale), XMFLOAT3(0.0f, 0.0f, 1.0f) },
            { XMFLOAT3(-Scale, Scale, Scale), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        };

        auto& vertexBuffer = m_vertexBuffers[GROUND];
        vertexBuffer = VertexBuffer::MakeUnique();
        auto numVert = static_cast<uint32_t>(size(vertices));
        N_RETURN(vertexBuffer->Create(m_device.get(), numVert, sizeof(Vertex),
            ResourceFlag::NONE, MemoryType::DEFAULT, 1, nullptr, 1, nullptr,
            1, nullptr, MemoryFlag::NONE, L"GroundVB"), false);

        uploaders.push_back(Resource::MakeUnique());
        N_RETURN(vertexBuffer->Upload(pCommandList, uploaders.back().get(), vertices,
            sizeof(vertices), 0, ResourceState::NON_PIXEL_SHADER_RESOURCE), false);
    }

    // Index Buffer
    {
        // Cube indices.
        uint32_t indices[] =
        {
            3,1,0,
            2,1,3,

            6,4,5,
            7,4,6,

            11,9,8,
            10,9,11,

            14,12,13,
            15,12,14,

            19,17,16,
            18,17,19,

            22,20,21,
            23,20,22
        };

        auto numIndices = static_cast<uint32_t>(size(indices));
        m_numIndices[GROUND] = numIndices;

        auto& indexBuffer = m_indexBuffers[GROUND];
        indexBuffer = IndexBuffer::MakeUnique();
        N_RETURN(indexBuffer->Create(m_device.get(), sizeof(indices), Format::R32_UINT, ResourceFlag::NONE,
            MemoryType::DEFAULT, 1, nullptr, 1, nullptr, 1, nullptr, MemoryFlag::NONE, L"GroundIB"), false);

        uploaders.push_back(Resource::MakeUnique());
        N_RETURN(indexBuffer->Upload(pCommandList, uploaders.back().get(), indices,
            sizeof(indices), 0, ResourceState::NON_PIXEL_SHADER_RESOURCE), false);
    }

    return true;
}

bool TVRayTracer::createInputLayout()
{
    // Define the vertex input layout.
    const InputElement inputElements[] =
    {
        { "POSITION",	0, Format::R32G32B32_FLOAT,	0, 0,								InputClassification::PER_VERTEX_DATA, 0 },
        { "NORMAL",		0, Format::R32G32B32_FLOAT,	0, D3D12_APPEND_ALIGNED_ELEMENT,	InputClassification::PER_VERTEX_DATA, 0 }
    };

    X_RETURN(m_pInputLayout, m_graphicsPipelineCache->CreateInputLayout(inputElements, static_cast<uint32_t>(size(inputElements))), false);

    return true;
}

bool TVRayTracer::createPipelineLayouts()
{
    // This is a pipeline layout for Z prepass
    {
        // Get pipeline layout
        const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
        pipelineLayout->SetRootCBV(0, 0, 0, Shader::Stage::VS);
        X_RETURN(m_pipelineLayouts[Z_PREPASS_LAYOUT], pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
            PipelineLayoutFlag::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, L"ZPrepassLayout"), false);
    }

    // This is a pipeline layout for g-buffer pass
    {
        const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
        pipelineLayout->SetRootCBV(0, 0, 0, Shader::Stage::PS);
        pipelineLayout->SetRootCBV(1, 0, 0, Shader::Stage::VS);
        pipelineLayout->SetConstants(2, SizeOfInUint32(uint32_t), 1, 0, Shader::Stage::PS);
        auto pipelineLayoutFlags = PipelineLayoutFlag::ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        X_RETURN(m_pipelineLayouts[GBUFFER_PASS_LAYOUT], pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(), pipelineLayoutFlags, L"GBufferPipelineLayout"), false);
    }

    // Global pipeline layout
    // This is a pipeline layout that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        const auto pipelineLayout = RayTracing::PipelineLayout::MakeUnique();
        pipelineLayout->SetRange(OUTPUT_VIEW, DescriptorType::UAV, 2, 0);
        pipelineLayout->SetRootSRV(ACCELERATION_STRUCTURE, 0, 0, DescriptorFlag::DATA_STATIC);
        pipelineLayout->SetRange(SAMPLER, DescriptorType::SAMPLER, 1, 0);
        pipelineLayout->SetRange(INDEX_BUFFERS, DescriptorType::SRV, NUM_MESH, 0, 1);
        pipelineLayout->SetRange(VERTEX_BUFFERS, DescriptorType::SRV, NUM_MESH, 0, 2);
        pipelineLayout->SetRootCBV(MATERIALS, 0);
        pipelineLayout->SetRootCBV(CONSTANTS, 1);
        pipelineLayout->SetRange(G_BUFFERS, DescriptorType::SRV, 5, 1);
        X_RETURN(m_pipelineLayouts[RT_GLOBAL_LAYOUT], pipelineLayout->GetPipelineLayout(
            m_device.get(), m_pipelineLayoutCache.get(), PipelineLayoutFlag::NONE,
            L"RayTracerGlobalPipelineLayout"), false);
    }

    // Local pipeline layout for RayGen shader
    // This is a pipeline layout that enables a shader to have unique arguments that come from shader tables.
    {
        const auto pipelineLayout = RayTracing::PipelineLayout::MakeUnique();
        pipelineLayout->SetConstants(0, SizeOfInUint32(RayGenConstants), 2);
        X_RETURN(m_pipelineLayouts[RAY_GEN_LAYOUT], pipelineLayout->GetPipelineLayout(
            m_device.get(), m_pipelineLayoutCache.get(), PipelineLayoutFlag::LOCAL_PIPELINE_LAYOUT,
            L"RayTracerRayGenPipelineLayout"), false);
    }

    return true;
}

bool TVRayTracer::createPipelines(
    Format rtFormat,
    Format dsFormat)
{
    // Create shaders
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::VS, ShaderIndex::VS, L"VSBasePassNew.cso"), false);
#if USE_COMPLEX_TESSELLATOR
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::HS, ShaderIndex::HS, L"HullShader.cso"), false);
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::DS, ShaderIndex::DS, L"DomainShader.cso"), false);
#else
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::HS, ShaderIndex::HS, L"HSSimple.cso"), false);
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::DS, ShaderIndex::DS, L"DSSimple.cso"), false);
#endif
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::PS, ShaderIndex::PS_DEPTH, L"PSDepth.cso"), false);
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::PS, ShaderIndex::PS_GBUFFER, L"PSGBufferNew.cso"), false);

    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::CS, ShaderIndex::CS, L"PRayTracing.cso"), false);

    // Z prepass
    {
        const auto state = Graphics::State::MakeUnique();
        state->SetPipelineLayout(m_pipelineLayouts[Z_PREPASS_LAYOUT]);
        state->SetShader(Shader::Stage::VS, m_shaderPool->GetShader(Shader::Stage::VS, ShaderIndex::VS));
        state->SetShader(Shader::Stage::HS, m_shaderPool->GetShader(Shader::Stage::HS, ShaderIndex::HS));
        state->SetShader(Shader::Stage::DS, m_shaderPool->GetShader(Shader::Stage::DS, ShaderIndex::DS));
        state->SetShader(Shader::Stage::PS, m_shaderPool->GetShader(Shader::Stage::PS, ShaderIndex::PS_DEPTH));
        state->IASetInputLayout(m_pInputLayout);
        state->IASetPrimitiveTopologyType(PrimitiveTopologyType::PATCH);
        state->OMSetDSVFormat(dsFormat);
        X_RETURN(m_pipelines[Z_PREPASS], state->GetPipeline(m_graphicsPipelineCache.get(), L"ZPrepass"), false);
    }

    // G-buffer pass
    {
        const auto state = Graphics::State::MakeUnique();
        state->IASetInputLayout(m_pInputLayout);
        state->SetPipelineLayout(m_pipelineLayouts[GBUFFER_PASS_LAYOUT]);
        state->SetShader(Shader::Stage::VS, m_shaderPool->GetShader(Shader::Stage::VS, ShaderIndex::VS));
        state->SetShader(Shader::Stage::HS, m_shaderPool->GetShader(Shader::Stage::HS, ShaderIndex::HS));
        state->SetShader(Shader::Stage::DS, m_shaderPool->GetShader(Shader::Stage::DS, ShaderIndex::DS));
        state->SetShader(Shader::Stage::PS, m_shaderPool->GetShader(Shader::Stage::PS, ShaderIndex::PS_GBUFFER));
        state->IASetPrimitiveTopologyType(PrimitiveTopologyType::PATCH);
        state->DSSetState(Graphics::DEPTH_READ_EQUAL, m_graphicsPipelineCache.get());
        state->OMSetNumRenderTargets(4);
        state->OMSetRTVFormat(0, Format::R8G8B8A8_UNORM);
        state->OMSetRTVFormat(1, Format::R10G10B10A2_UNORM);
        state->OMSetRTVFormat(2, Format::R8G8_UNORM);
        state->OMSetRTVFormat(3, Format::R16G16_FLOAT);
        state->OMSetDSVFormat(dsFormat);
        X_RETURN(m_pipelines[GBUFFER_PASS], state->GetPipeline(m_graphicsPipelineCache.get(), L"GBufferPass"), false);
    }

    // Ray tracing pass
    {
        const auto state = RayTracing::State::MakeUnique();
        state->SetShaderLibrary(m_shaderPool->GetShader(Shader::Stage::CS, ShaderIndex::CS));
        state->SetHitGroup(HIT_GROUP_REFLECTION, HitGroupNames[HIT_GROUP_REFLECTION], ClosestHitShaderNames[HIT_GROUP_REFLECTION]);
        state->SetHitGroup(HIT_GROUP_DIFFUSE, HitGroupNames[HIT_GROUP_DIFFUSE], ClosestHitShaderNames[HIT_GROUP_DIFFUSE]);
        state->SetShaderConfig(sizeof(XMFLOAT4), sizeof(XMFLOAT2));
        state->SetLocalPipelineLayout(0, m_pipelineLayouts[RAY_GEN_LAYOUT],
            1, reinterpret_cast<const void**>(&RaygenShaderName));
        state->SetGlobalPipelineLayout(m_pipelineLayouts[RT_GLOBAL_LAYOUT]);
        state->SetMaxRecursionDepth(1);
        X_RETURN(m_pipelines[RAY_TRACING], state->GetPipeline(m_rayTracingPipelineCache.get(), L"Raytracing"), false);
    }

    return true;
}

bool TVRayTracer::createDescriptorTables()
{
    // Output UAV
    {
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, 1, &m_outputView->GetUAV());
        X_RETURN(m_uavTable, descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
    }

    // Index buffer SRVs
    {
        Descriptor descriptors[NUM_MESH];
        for (auto i = 0u; i < NUM_MESH; ++i) descriptors[i] = m_indexBuffers[i]->GetSRV();
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
        X_RETURN(m_srvTables[SRV_TABLE_IB], descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
    }

    // Vertex buffer SRVs
    {
        Descriptor descriptors[NUM_MESH];
        for (auto i = 0u; i < NUM_MESH; ++i) descriptors[i] = m_vertexBuffers[i]->GetSRV();
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
        X_RETURN(m_srvTables[SRV_TABLE_VB], descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
    }

    // G-buffer SRVs
    {
        const Descriptor descriptors[] =
        {
            m_gbuffers[BASE_COLOR]->GetSRV(),
            m_gbuffers[NORMAL]->GetSRV(),
            m_gbuffers[ROUGH_METAL]->GetSRV(),
            m_depth->GetSRV(),
            m_lightProbe->GetSRV()
        };
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
        X_RETURN(m_srvTables[SRV_TABLE_GB], descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
    }

    // RTV table
    {
        const Descriptor descriptors[] =
        {
            m_gbuffers[BASE_COLOR]->GetRTV(),
            m_gbuffers[NORMAL]->GetRTV(),
            m_gbuffers[ROUGH_METAL]->GetRTV(),
            m_gbuffers[VELOCITY]->GetRTV()
        };
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, static_cast<uint32_t>(size(descriptors)), descriptors);
        m_framebuffer = descriptorTable->GetFramebuffer(m_descriptorTableCache.get(), &m_depth->GetDSV());
    }

    // Create the sampler
    {
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        const auto samplerAnisoWrap = SamplerPreset::ANISOTROPIC_WRAP;
        descriptorTable->SetSamplers(0, 1, &samplerAnisoWrap, m_descriptorTableCache.get());
        X_RETURN(m_samplerTable, descriptorTable->GetSamplerTable(m_descriptorTableCache.get()), false);
    }

    return true;
}

bool TVRayTracer::buildAccelerationStructures(
    const RayTracing::CommandList* pCommandList,
    GeometryBuffer* pGeometries)
{
    // Set geometries
    VertexBufferView vertexBufferViews[NUM_MESH];
    IndexBufferView indexBufferViews[NUM_MESH];
    for (auto i = 0u; i < NUM_MESH; ++i)
    {
        vertexBufferViews[i] = m_vertexBuffers[i]->GetVBV();
        indexBufferViews[i] = m_indexBuffers[i]->GetIBV();
        BottomLevelAS::SetTriangleGeometries(pGeometries[i], 1, Format::R32G32B32_FLOAT,
            &vertexBufferViews[i], &indexBufferViews[i]);
    }

    // Descriptor index in descriptor pool
    const auto bottomLevelASIndex = 0u;
    const auto topLevelASIndex = bottomLevelASIndex + NUM_MESH;

    // Prebuild
    for (auto i = 0u; i < NUM_MESH; ++i)
    {
        m_bottomLevelASs[i] = BottomLevelAS::MakeUnique();
        N_RETURN(m_bottomLevelASs[i]->PreBuild(m_device.get(), 1, pGeometries[i], bottomLevelASIndex + i, BuildFlag::NONE), false);
    }
    m_topLevelAS = TopLevelAS::MakeUnique();
    N_RETURN(m_topLevelAS->PreBuild(m_device.get(), NUM_MESH, topLevelASIndex,
        BuildFlag::ALLOW_UPDATE), false);

    // Create scratch buffer
    auto scratchSize = m_topLevelAS->GetScratchDataMaxSize();
    for (const auto& bottomLevelAS : m_bottomLevelASs)
        scratchSize = (max)(bottomLevelAS->GetScratchDataMaxSize(), scratchSize);
    m_scratch = Resource::MakeUnique();
    N_RETURN(AccelerationStructure::AllocateUAVBuffer(m_device.get(), m_scratch.get(), scratchSize), false);

    // Get descriptor pool and create descriptor tables
    N_RETURN(createDescriptorTables(), false);
    const auto& descriptorPool = m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL);

    // Set instance
    XMFLOAT3X4 matrices[NUM_MESH];
    XMStoreFloat3x4(&matrices[GROUND], (XMMatrixScaling(8.0f, 0.5f, 8.0f) * XMMatrixTranslation(0.0f, -0.5f, 0.0f)));
    XMStoreFloat3x4(&matrices[MODEL_OBJ], (XMMatrixScaling(m_posScale.w, m_posScale.w, m_posScale.w) *
        XMMatrixTranslation(m_posScale.x, m_posScale.y, m_posScale.z)));
    float* const transforms[] =
    {
        reinterpret_cast<float*>(&matrices[GROUND]),
        reinterpret_cast<float*>(&matrices[MODEL_OBJ])
    };
    for (auto& instances : m_instances) instances = Resource::MakeUnique();
    auto& instances = m_instances[FrameCount - 1];
    const BottomLevelAS* pBottomLevelASs[NUM_MESH];
    for (auto i = 0u; i < NUM_MESH; ++i) pBottomLevelASs[i] = m_bottomLevelASs[i].get();
    TopLevelAS::SetInstances(m_device.get(), instances.get(), NUM_MESH, pBottomLevelASs, transforms);

    // Build bottom level ASs
    for (auto& bottomLevelAS : m_bottomLevelASs)
        bottomLevelAS->Build(pCommandList, m_scratch.get(), descriptorPool);

    // Build top level AS
    m_topLevelAS->Build(pCommandList, m_scratch.get(), instances.get(), descriptorPool);

    return true;
}

bool TVRayTracer::buildShaderTables()
{
    // Get shader identifiers.
    const auto shaderIDSize = ShaderRecord::GetShaderIDSize(m_device.get());
    const auto cbRayGen = RayGenConstants();

    for (uint8_t i = 0; i < FrameCount; ++i)
    {
        // Ray gen shader table
        m_rayGenShaderTables[i] = ShaderTable::MakeUnique();
        N_RETURN(m_rayGenShaderTables[i]->Create(m_device.get(), 1, shaderIDSize + sizeof(RayGenConstants),
            (L"RayGenShaderTable" + to_wstring(i)).c_str()), false);
        N_RETURN(m_rayGenShaderTables[i]->AddShaderRecord(ShaderRecord::MakeUnique(m_device.get(),
            m_pipelines[RAY_TRACING], RaygenShaderName, &cbRayGen, sizeof(RayGenConstants)).get()), false);
    }

    // Hit group shader table
    m_hitGroupShaderTable = ShaderTable::MakeUnique();
    N_RETURN(m_hitGroupShaderTable->Create(m_device.get(), 2, shaderIDSize, L"HitGroupShaderTable"), false);
    N_RETURN(m_hitGroupShaderTable->AddShaderRecord(ShaderRecord::MakeUnique(m_device.get(),
        m_pipelines[RAY_TRACING], HitGroupNames[HIT_GROUP_REFLECTION]).get()), false);
    N_RETURN(m_hitGroupShaderTable->AddShaderRecord(ShaderRecord::MakeUnique(m_device.get(),
        m_pipelines[RAY_TRACING], HitGroupNames[HIT_GROUP_DIFFUSE]).get()), false);

    // Miss shader table
    m_missShaderTable = ShaderTable::MakeUnique();
    N_RETURN(m_missShaderTable->Create(m_device.get(), 1, shaderIDSize, L"MissShaderTable"), false);
    N_RETURN(m_missShaderTable->AddShaderRecord(ShaderRecord::MakeUnique(m_device.get(),
        m_pipelines[RAY_TRACING], MissShaderName).get()), false);

    return true;
}

void TVRayTracer::zPrepass(
    const XUSG::CommandList* pCommandList,
    uint8_t                  frameIndex)
{
    // Set depth barrier to write
    ResourceBarrier barrier;
    const auto numBarriers = m_depth->SetBarrier(&barrier, ResourceState::DEPTH_WRITE);
    pCommandList->Barrier(numBarriers, &barrier);

    //Clear depth
    pCommandList->OMSetRenderTargets(0, nullptr, &m_depth->GetDSV());
    pCommandList->ClearDepthStencilView(m_depth->GetDSV(), ClearFlag::DEPTH, 1.0f);

    // Set pipeline state
    pCommandList->SetGraphicsPipelineLayout(m_pipelineLayouts[Z_PREPASS_LAYOUT]);
    pCommandList->SetPipelineState(m_pipelines[Z_PREPASS]);

    // Set viewport
    Viewport viewport(0.0f, 0.0f, static_cast<float>(m_viewport.x), static_cast<float>(m_viewport.y));
    RectRange scissorRect(0, 0, m_viewport.x, m_viewport.y);
    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    // Record commands.
#if USE_COMPLEX_TESSELLATOR
    pCommandList->IASetPrimitiveTopology(PrimitiveTopology::CONTROL_POINT13_PATCHLIST);
#else
    pCommandList->IASetPrimitiveTopology(PrimitiveTopology::CONTROL_POINT3_PATCHLIST);
#endif

    for (auto i = 0u; i < NUM_MESH; ++i)
    {
        // Set descriptor tables
        pCommandList->SetGraphicsRootConstantBufferView(0, m_cbBasePass[i].get(), m_cbBasePass[i]->GetCBVOffset(frameIndex));
        pCommandList->IASetVertexBuffers(0, 1, &m_vertexBuffers[i]->GetVBV());
        pCommandList->IASetIndexBuffer(m_indexBuffers[i]->GetIBV());
        pCommandList->DrawIndexed(m_numIndices[i], 1, 0, 0, 0);
    }
}

void TVRayTracer::gbufferPass(
    const XUSG::CommandList* pCommandList,
    uint8_t                  frameIndex,
    bool                     depthClear)
{
    // Set barriers
    ResourceBarrier barriers[5];
    const auto depthState = depthClear ? ResourceState::DEPTH_WRITE :
        ResourceState::DEPTH_READ | ResourceState::NON_PIXEL_SHADER_RESOURCE;
    auto numBarriers = m_depth->SetBarrier(barriers, depthState);
    for (auto& gbuffer : m_gbuffers)
        numBarriers = gbuffer->SetBarrier(barriers, ResourceState::RENDER_TARGET, numBarriers, 0);
    pCommandList->Barrier(numBarriers, barriers);

    // Set framebuffer
    pCommandList->OMSetFramebuffer(m_framebuffer);

    // Clear render target
    const float clearColor[4] = {};
    pCommandList->ClearRenderTargetView(m_gbuffers[NORMAL]->GetRTV(), clearColor);
    pCommandList->ClearRenderTargetView(m_gbuffers[ROUGH_METAL]->GetRTV(), clearColor);
    pCommandList->ClearRenderTargetView(m_gbuffers[VELOCITY]->GetRTV(), clearColor);
    if (depthClear) pCommandList->ClearDepthStencilView(m_depth->GetDSV(), ClearFlag::DEPTH, 1.0f);

    // Set pipeline state
    pCommandList->SetGraphicsPipelineLayout(m_pipelineLayouts[GBUFFER_PASS_LAYOUT]);
    pCommandList->SetPipelineState(m_pipelines[GBUFFER_PASS]);
    pCommandList->SetGraphicsRootConstantBufferView(0, m_cbMaterials.get());

    // Set viewport
    Viewport viewport(0.0f, 0.0f, static_cast<float>(m_viewport.x), static_cast<float>(m_viewport.y));
    RectRange scissorRect(0, 0, m_viewport.x, m_viewport.y);
    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

#if USE_COMPLEX_TESSELLATOR
    pCommandList->IASetPrimitiveTopology(PrimitiveTopology::CONTROL_POINT13_PATCHLIST);
#else
    pCommandList->IASetPrimitiveTopology(PrimitiveTopology::CONTROL_POINT3_PATCHLIST);
#endif

    for (auto i = 0u; i < NUM_MESH; ++i)
    {
        // Set descriptor tables
        pCommandList->SetGraphicsRootConstantBufferView(1, m_cbBasePass[i].get(), m_cbBasePass[i]->GetCBVOffset(frameIndex));
        pCommandList->SetGraphics32BitConstant(2, i);

        pCommandList->IASetVertexBuffers(0, 1, &m_vertexBuffers[i]->GetVBV());
        pCommandList->IASetIndexBuffer(m_indexBuffers[i]->GetIBV());

        pCommandList->DrawIndexed(m_numIndices[i], 1, 0, 0, 0);
    }
}

void TVRayTracer::rayTrace(
    const RayTracing::CommandList* pCommandList,
    uint8_t                        frameIndex)
{
    // Bind the acceleration structure and dispatch rays.
    pCommandList->SetComputePipelineLayout(m_pipelineLayouts[RT_GLOBAL_LAYOUT]);
    pCommandList->SetComputeDescriptorTable(OUTPUT_VIEW, m_uavTable);
    pCommandList->SetTopLevelAccelerationStructure(ACCELERATION_STRUCTURE, m_topLevelAS.get());
    pCommandList->SetComputeDescriptorTable(SAMPLER, m_samplerTable);
    pCommandList->SetComputeDescriptorTable(INDEX_BUFFERS, m_srvTables[SRV_TABLE_IB]);
    pCommandList->SetComputeDescriptorTable(VERTEX_BUFFERS, m_srvTables[SRV_TABLE_VB]);
    pCommandList->SetComputeRootConstantBufferView(MATERIALS, m_cbMaterials.get());
    pCommandList->SetComputeRootConstantBufferView(CONSTANTS, m_cbRaytracing.get(), m_cbRaytracing->GetCBVOffset(frameIndex));
    pCommandList->SetComputeDescriptorTable(G_BUFFERS, m_srvTables[SRV_TABLE_GB]);

    // Fallback layer has no depth
    pCommandList->DispatchRays(m_pipelines[RAY_TRACING], m_viewport.x, m_viewport.y, 1,
        m_hitGroupShaderTable.get(), m_missShaderTable.get(), m_rayGenShaderTables[frameIndex].get());
}
