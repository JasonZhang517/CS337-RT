//--------------------------------------------------------------------------------------
// Copyright (c) XU, Tianchen. All rights reserved.
//--------------------------------------------------------------------------------------

#include "stdafx.h"
#include "PostProcessor.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;

PostProcessor::PostProcessor(const Device::sptr& device) :
    m_device(device)
{
    m_shaderPool = ShaderPool::MakeUnique();
    m_graphicsPipelineCache = Graphics::PipelineCache::MakeUnique(device.get());
    m_pipelineLayoutCache = PipelineLayoutCache::MakeUnique(device.get());
    m_descriptorTableCache = DescriptorTableCache::MakeUnique(device.get(), L"PostProcessorDescriptorTableCache");
}

PostProcessor::~PostProcessor()
{
}

bool PostProcessor::Init(CommandList* pCommandList, uint32_t width, uint32_t height, Format rtFormat,
    const Texture2D* inputView)
{
    m_viewport = XMUINT2(width, height);
    m_inputView = inputView;

    // Create pipelines
    N_RETURN(createPipelineLayouts(), false);
    N_RETURN(createPipelines(rtFormat), false);
    N_RETURN(createDescriptorTables(), false);

    return true;
}


void PostProcessor::ToneMap(const CommandList* pCommandList, const Descriptor& rtv,
    uint32_t numBarriers, ResourceBarrier* pBarriers)
{
    // Bind the heaps, acceleration structure and dispatch rays.
    const DescriptorPool descriptorPools[] =
    {
        m_descriptorTableCache->GetDescriptorPool(CBV_SRV_UAV_POOL)
    };
    pCommandList->SetDescriptorPools(static_cast<uint32_t>(size(descriptorPools)), descriptorPools);

    pCommandList->Barrier(numBarriers, pBarriers);

    // Set render target
    pCommandList->OMSetRenderTargets(1, &rtv);

    // Set descriptor tables
    pCommandList->SetGraphicsPipelineLayout(m_pipelineLayout);
    pCommandList->SetGraphicsDescriptorTable(0, m_srvTable);

    // Set pipeline state
    pCommandList->SetPipelineState(m_pipeline);

    // Set viewport
    Viewport viewport(0.0f, 0.0f, static_cast<float>(m_viewport.x), static_cast<float>(m_viewport.y));
    RectRange scissorRect(0, 0, m_viewport.x, m_viewport.y);
    pCommandList->RSSetViewports(1, &viewport);
    pCommandList->RSSetScissorRects(1, &scissorRect);

    pCommandList->IASetPrimitiveTopology(PrimitiveTopology::TRIANGLELIST);
    pCommandList->Draw(3, 1, 0, 0);
}

bool PostProcessor::createPipelineLayouts()
{
    // This is a pipeline layout for tone mapping
    {
        const auto pipelineLayout = Util::PipelineLayout::MakeUnique();
        pipelineLayout->SetRange(0, DescriptorType::SRV, 1, 0);
        pipelineLayout->SetShaderStage(0, Shader::Stage::PS);
        X_RETURN(m_pipelineLayout, pipelineLayout->GetPipelineLayout(m_pipelineLayoutCache.get(),
            PipelineLayoutFlag::NONE, L"ToneMappingPipelineLayout"), false);
    }

    return true;
}

bool PostProcessor::createPipelines(Format rtFormat)
{
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::VS, 0, L"VSScreenQuad.cso"), false);
    N_RETURN(m_shaderPool->CreateShader(Shader::Stage::PS, 0, L"PSToneMapNew.cso"), false);

    // Tone mapping
    {
        const auto state = Graphics::State::MakeUnique();
        state->SetPipelineLayout(m_pipelineLayout);
        state->SetShader(Shader::Stage::VS, m_shaderPool->GetShader(Shader::Stage::VS, 0));
        state->SetShader(Shader::Stage::PS, m_shaderPool->GetShader(Shader::Stage::PS, 0));
        state->DSSetState(Graphics::DEPTH_STENCIL_NONE, m_graphicsPipelineCache.get());
        state->IASetPrimitiveTopologyType(PrimitiveTopologyType::TRIANGLE);
        state->OMSetNumRenderTargets(1);
        state->OMSetRTVFormat(0, rtFormat);
        X_RETURN(m_pipeline, state->GetPipeline(m_graphicsPipelineCache.get(), L"ToneMapping"), false);
    }

    return true;
}

bool PostProcessor::createDescriptorTables()
{
    {
        const auto descriptorTable = Util::DescriptorTable::MakeUnique();
        descriptorTable->SetDescriptors(0, 1, &m_inputView->GetSRV());
        X_RETURN(m_srvTable, descriptorTable->GetCbvSrvUavTable(m_descriptorTableCache.get()), false);
    }

    return true;
}
