//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Core/XUSG.h"
#include "RayTracing/XUSGRayTracing.h"

class PostProcessor
{
public:
    PostProcessor(const XUSG::Device::sptr& device);
    virtual ~PostProcessor();

    bool Init(XUSG::CommandList* pCommandList, uint32_t width, uint32_t height, XUSG::Format rtFormat,
        const XUSG::Texture2D* inputView);
    void ToneMap(const XUSG::CommandList* pCommandList, const XUSG::Descriptor& rtv,
        uint32_t numBarriers, XUSG::ResourceBarrier* pBarriers);

protected:
    bool createPipelineLayouts();
    bool createPipelines(XUSG::Format rtFormat);
    bool createDescriptorTables();

    XUSG::Device::sptr m_device;

    DirectX::XMUINT2			m_viewport;

    XUSG::PipelineLayout		m_pipelineLayout;
    XUSG::Pipeline				m_pipeline;

    XUSG::DescriptorTable		m_srvTable;
    XUSG::DescriptorTable		m_samplerTable;

    const XUSG::Texture2D* m_inputView;

    XUSG::ShaderPool::uptr				m_shaderPool;
    XUSG::Graphics::PipelineCache::uptr	m_graphicsPipelineCache;
    XUSG::PipelineLayoutCache::uptr		m_pipelineLayoutCache;
    XUSG::DescriptorTableCache::uptr	m_descriptorTableCache;
};
