//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"
#include "RayTracing/XUSGRayTracing.h"

class PRayTracer
{
public:
    enum MeshIndex : uint32_t
    {
        GROUND,
        MODEL_OBJ,

        NUM_MESH
    };

    PRayTracer(const XUSG::RayTracing::Device::sptr& device);
    virtual ~PRayTracer();

    bool Init(XUSG::RayTracing::CommandList* pCommandList, uint32_t width, uint32_t height,
        std::vector<XUSG::Resource::uptr>& uploaders, XUSG::RayTracing::GeometryBuffer* pGeometries,
        const char* fileName, const wchar_t* envFileName, XUSG::Format rtFormat,
        const DirectX::XMFLOAT4& posScale = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f),
        uint8_t maxGBufferMips = 1);
    void UpdateFrame(uint8_t frameIndex, DirectX::CXMVECTOR eyePt, DirectX::CXMMATRIX viewProj, float timeStep);
    void Render(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex);
    void UpdateAccelerationStructures(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex);

    const XUSG::Texture2D* GetRayTracingOutput() const;

    static const uint8_t FrameCount = 3;
private:
    enum PipelineLayoutIndex : uint8_t
    {
        RT_GLOBAL_LAYOUT,
        RAY_GEN_LAYOUT,
        HIT_RADIANCE_LAYOUT,

        NUM_PIPELINE_LAYOUT
    };

    enum PipelineIndex : uint8_t
    {
        RAY_TRACING,

        NUM_PIPELINE
    };

    enum GlobalPipelineLayoutSlot : uint8_t
    {
        OUTPUT_VIEW,
        ACCELERATION_STRUCTURE,
        SAMPLER,
        INDEX_BUFFERS,
        VERTEX_BUFFERS,
        MATERIALS,
        CONSTANTS,
        G_BUFFERS
    };

    enum SRVTable : uint8_t
    {
        SRV_TABLE_IB,
        SRV_TABLE_VB,
        SRV_TABLE_GB,

        NUM_SRV_TABLE
    };

    enum HitGroup : uint8_t
    {
        HIT_GROUP_RADIANCE,
        HIT_GROUP_SHADOW,

        NUM_HIT_GROUP
    };

    enum ShaderIndex : uint8_t
    {
        CS,

        NUM_SHADER
    };

    bool createVB(XUSG::RayTracing::CommandList* pCommandList, uint32_t numVert,
        uint32_t stride, const uint8_t* pData, std::vector<XUSG::Resource::uptr>& uploaders);
    bool createIB(XUSG::RayTracing::CommandList* pCommandList, uint32_t numIndices,
        const uint32_t* pData, std::vector<XUSG::Resource::uptr>& uploaders);
    bool createGroundMesh(XUSG::RayTracing::CommandList* pCommandList,
        std::vector<XUSG::Resource::uptr>& uploaders);
    bool createInputLayout();
    bool createPipelineLayouts();
    bool createPipelines(XUSG::Format rtFormat);
    bool createDescriptorTables();
    bool buildAccelerationStructures(const XUSG::RayTracing::CommandList* pCommandList,
        XUSG::RayTracing::GeometryBuffer* pGeometries);
    bool buildShaderTables();

    void rayTrace(const XUSG::RayTracing::CommandList* pCommandList, uint8_t frameIndex);

    XUSG::RayTracing::Device::sptr m_device;

    uint32_t            m_numIndices[NUM_MESH];

    DirectX::XMUINT2    m_viewport;
    DirectX::XMFLOAT4   m_posScale;
    DirectX::XMFLOAT4X4 m_worlds[NUM_MESH];

    XUSG::RayTracing::BottomLevelAS::uptr m_bottomLevelASs[NUM_MESH];
    XUSG::RayTracing::TopLevelAS::uptr    m_topLevelAS;

    const XUSG::InputLayout*    m_pInputLayout;
    XUSG::PipelineLayout        m_pipelineLayouts[NUM_PIPELINE_LAYOUT];
    XUSG::Pipeline              m_pipelines[NUM_PIPELINE];

    XUSG::DescriptorTable       m_srvTables[NUM_SRV_TABLE];
    XUSG::DescriptorTable       m_uavTable;
    XUSG::DescriptorTable       m_samplerTable;
    XUSG::Framebuffer           m_framebuffer;

    XUSG::VertexBuffer::uptr    m_vertexBuffers[NUM_MESH];
    XUSG::IndexBuffer::uptr     m_indexBuffers[NUM_MESH];

    XUSG::Texture2D::uptr       m_outputView;

    XUSG::ConstantBuffer::uptr  m_cbMaterials;
    XUSG::ConstantBuffer::uptr  m_cbRaytracing;

    XUSG::Resource::uptr        m_scratch;
    XUSG::Resource::uptr        m_instances[FrameCount];

    XUSG::ShaderResource::sptr  m_lightProbe;

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
