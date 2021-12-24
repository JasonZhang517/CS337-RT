//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"
#include "RayTracing/XUSGRayTracing.h"

class TVRayTracer
{
public:
    enum MeshIndex : uint32_t
    {
        GROUND,
        MODEL_OBJ,

        NUM_MESH
    };

    TVRayTracer(const XUSG::RayTracing::Device::sptr& device);
    virtual ~TVRayTracer();

    bool Init(XUSG::RayTracing::CommandList* pCommandList, uint32_t width, uint32_t height,
        std::vector<XUSG::Resource::uptr>& uploaders, XUSG::RayTracing::GeometryBuffer* pGeometries,
        const char* fileName, const wchar_t* envFileName, XUSG::Format rtFormat,
        const DirectX::XMFLOAT4& posScale = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f));
    void UpdateFrame(uint8_t frameIndex, DirectX::CXMVECTOR eyePt, DirectX::CXMMATRIX viewProj, float timeStep, uint32_t tessFactor);
    void Render(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex, const XUSG::Descriptor& rtv, uint32_t numBarriers, XUSG::ResourceBarrier* pBarriers);
    void UpdateAccelerationStructures(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex);

    static const uint8_t FrameCount = 3;
    static const uint8_t MinTessFactor = 1;
    static const uint8_t MaxTessFactor = 5;
private:
    enum PipelineLayoutIndex : uint8_t
    {
        Z_PRE_LAYOUT,
        ENV_PRE_LAYOUT,
        TESSELLATION_LAYOUT,

        RT_GLOBAL_LAYOUT,
        RAY_GEN_LAYOUT,
        HIT_RADIANCE_LAYOUT,

        GRAPHICS_LAYOUT,
        TONEMAP_LAYOUT,

        NUM_PIPELINE_LAYOUT
    };

    enum PipelineIndex : uint8_t
    {
        Z_PREPASS,
        ENV_PREPASS,
        TESSELLATION,
        RAY_TRACING,
        GRAPHICS,
        TONEMAP,

        NUM_PIPELINE
    };

    enum PipelineLayoutSlot : uint8_t
    {
        VERTEX_COLOR,
        ACCELERATION_STRUCTURE,
        SAMPLER,
        INDEX_BUFFERS,
        VERTEX_BUFFERS,
        MATERIALS,
        CONSTANTS,
        TESS_CONSTS,
        TESS_DOMS,
        ENV_TEXTURE,
        OUTPUT_VIEW
    };

    enum SRVTable : uint8_t
    {
        SRV_TABLE_IB,
        SRV_TABLE_VB,
        SRV_TABLE_ENV,
        SRV_TABLE_VCOLOR,
        SRV_TABLE_OUTPUT,
        SRV_TABLE_TESSDOMS,

        NUM_SRV_TABLE
    };

    enum UAVTable : uint8_t
    {
        UAV_TABLE_RT,
        UAV_TABLE_OUTPUT,
        UAV_TABLE_TESSDOMS,

        NUM_UAV_TABLE
    };

    enum HitGroup : uint8_t
    {
        HIT_GROUP_RADIANCE,
        HIT_GROUP_SHADOW,

        NUM_HIT_GROUP
    };

    enum ShaderIndex : uint8_t
    {
        VS_DEPTH,
        VS_SQUAD,
        PS_ENV,
        CS_RT,
        VS_IDENT,
        HS_GRAPHICS,
        DS_DEPTH,
        DS_TESS,
        DS_GRAPHICS,
        PS_EMPTY,
        PS_GRAPHICS,
        PS_TONEMAP
    };

    bool createVB(XUSG::RayTracing::CommandList* pCommandList, uint32_t numVert,
        uint32_t stride, const uint8_t* pData, std::vector<XUSG::Resource::uptr>& uploaders);
    bool createIB(XUSG::RayTracing::CommandList* pCommandList, uint32_t numIndices,
        const uint32_t* pData, std::vector<XUSG::Resource::uptr>& uploaders);
    bool createGroundMesh(XUSG::RayTracing::CommandList* pCommandList,
        std::vector<XUSG::Resource::uptr>& uploaders);
    bool createInputLayout();
    bool createPipelineLayouts();
    bool createPipelines(XUSG::Format rtFormat, XUSG::Format dsFormat);
    bool createDescriptorTables();
    bool buildAccelerationStructures(const XUSG::RayTracing::CommandList* pCommandList,
        XUSG::RayTracing::GeometryBuffer* pGeometries);
    bool buildShaderTables();

    void zPrepass(const XUSG::CommandList* pCommandList, uint8_t frameIndex);
    void envPrepass(const XUSG::CommandList* pCommandList, uint8_t frameIndex);
    void tessellate(const XUSG::CommandList* pCommandList, uint8_t frameIndex);
    void raytrace(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex);
    void rasterize(const XUSG::CommandList* pCommandList, uint8_t frameIndex);
    void toneMap(const XUSG::CommandList* pCommandList, const XUSG::Descriptor& rtv, uint32_t numBarriers, XUSG::ResourceBarrier* pBarriers);

    XUSG::RayTracing::Device::sptr m_device;

    uint32_t            m_numIndices[NUM_MESH];
    uint32_t            m_numVerts[NUM_MESH];
    uint32_t            m_numMaxTessVerts[NUM_MESH];
    uint32_t            m_tessFactor;
    uint32_t            m_maxVertPerPatch;

    DirectX::XMUINT2    m_viewport;
    DirectX::XMFLOAT4   m_posScale;
    DirectX::XMFLOAT4X4 m_worlds[NUM_MESH];

    XUSG::RayTracing::BottomLevelAS::uptr m_bottomLevelASs[NUM_MESH];
    XUSG::RayTracing::TopLevelAS::uptr    m_topLevelAS;

    const XUSG::InputLayout*        m_pInputLayout;
    XUSG::PipelineLayout            m_pipelineLayouts[NUM_PIPELINE_LAYOUT];
    XUSG::Pipeline                  m_pipelines[NUM_PIPELINE];

    XUSG::DescriptorTable           m_srvTables[NUM_SRV_TABLE];
    XUSG::DescriptorTable           m_uavTables[NUM_UAV_TABLE];
    XUSG::DescriptorTable           m_samplerTable;

    XUSG::VertexBuffer::uptr        m_vertexBuffers[NUM_MESH];
    XUSG::IndexBuffer::uptr         m_indexBuffers[NUM_MESH];

    XUSG::DepthStencil::uptr        m_depth;
    XUSG::Texture2D::uptr           m_outputView;
    XUSG::StructuredBuffer::uptr    m_tessColors[NUM_MESH];
    XUSG::StructuredBuffer::uptr    m_tessDoms[NUM_MESH];

    XUSG::ConstantBuffer::uptr      m_cbMaterials;
    XUSG::ConstantBuffer::uptr      m_cbGlobal;
    XUSG::ConstantBuffer::uptr      m_cbGraphics[NUM_MESH];
    XUSG::ConstantBuffer::uptr      m_cbEnv;
    XUSG::ConstantBuffer::uptr      m_cbTessellation;

    XUSG::Resource::uptr            m_scratch;
    XUSG::Resource::uptr            m_instances[FrameCount];

    XUSG::ShaderResource::sptr      m_lightProbe;

    // Shader tables
    static const wchar_t* HitGroupNames[NUM_HIT_GROUP];
    static const wchar_t* RaygenShaderName;
    static const wchar_t* ClosestHitShaderNames[NUM_HIT_GROUP];
    static const wchar_t* MissShaderNames[NUM_HIT_GROUP];

    XUSG::RayTracing::ShaderTable::uptr     m_missShaderTable;
    XUSG::RayTracing::ShaderTable::uptr     m_hitGroupShaderTable;
    XUSG::RayTracing::ShaderTable::uptr     m_rayGenShaderTables[FrameCount];

    XUSG::ShaderPool::uptr                  m_shaderPool;
    XUSG::RayTracing::PipelineCache::uptr   m_rayTracingPipelineCache;
    XUSG::Graphics::PipelineCache::uptr     m_graphicsPipelineCache;
    XUSG::PipelineLayoutCache::uptr         m_pipelineLayoutCache;
    XUSG::DescriptorTableCache::uptr        m_descriptorTableCache;
};
