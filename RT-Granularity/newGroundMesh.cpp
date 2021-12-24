#include "stdafx.h"
#include "PRayTracer.h"
#include "XUSGObjLoader.h"
#define _INDEPENDENT_DDS_LOADER_
#include "Advanced/XUSGDDSLoader.h"
#undef _INDEPENDENT_DDS_LOADER_
#include "DirectXPackedVector.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;
using namespace XUSG::RayTracing;





bool PRayTracer::createGroundMesh(
    RayTracing::CommandList* pCommandList,
    vector<Resource::uptr>& uploaders)
{
    const uint32_t n = 5;//size
    // Vertex buffer
    {
        // Cube vertices positions and corresponding triangle normals.
        Vertex vertices[n * n * 6];
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[n * i + j] = { XMFLOAT3((-1.0 + 2.0f * j / (n - 1)), 1.0f, (1.0 - 2.0f * i / (n - 1))), XMFLOAT3(0.0f, 1.0f, 0.0f) };
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[n * n + n * i + j] = { XMFLOAT3((-1.0 + 2.0f * j / (n - 1)), -1.0f, (1.0 - 2.0f * i / (n - 1))), XMFLOAT3(0.0f, -1.0f, 0.0f) };
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[2 * n * n + n * i + j] = { XMFLOAT3(-1.0f,(-1.0 + 2.0f * j / (n - 1)), (1.0 - 2.0f * i / (n - 1))), XMFLOAT3(-1.0f, 0.0f,0.0f) };
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[3 * n * n + n * i + j] = { XMFLOAT3(1.0f,(-1.0 + 2.0f * j / (n - 1)), (1.0 - 2.0f * i / (n - 1))), XMFLOAT3(1.0f, 0.0f,0.0f) };
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[4 * n * n + n * i + j] = { XMFLOAT3((-1.0 + 2.0f * j / (n - 1)), (1.0 - 2.0f * i / (n - 1)),-1.0f), XMFLOAT3(0.0f, 0.0f,-1.0f) };
            }
        }
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                vertices[5 * n * n + n * i + j] = { XMFLOAT3((-1.0 + 2.0f * j / (n - 1)), (1.0 - 2.0f * i / (n - 1)),1.0f), XMFLOAT3(0.0f, 0.0f,1.0f) };
            }
        }

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
        // indices[36 * (n - 1) * (n - 1)];
        uint32_t indices[6][n - 1][n - 1][2][3];
        for (int s = 0; s < 6; s = s + 2)
        {
            for (int i = 0; i < (n - 1); i++)
            {
                for (int j = 0; j < (n - 1); j++)
                {
                    indices[s][i][j][0][0] = i * n + j + s * n * n;
                    indices[s][i][j][0][1] = i * n + j + 1 + s * n * n;
                    indices[s][i][j][0][2] = (i + 1) * n + j + 1 + s * n * n;
                    indices[s][i][j][1][0] = i * n + j + s * n * n;
                    indices[s][i][j][1][1] = (i + 1) * n + j + 1 + s * n * n;
                    indices[s][i][j][1][2] = (i + 1) * n + j + s * n * n;
                    indices[s + 1][i][j][0][0] = i * n + j + (s + 1) * n * n;
                    indices[s + 1][i][j][0][1] = (i + 1) * n + j + 1 + (s + 1) * n * n;
                    indices[s + 1][i][j][0][2] = i * n + j + 1 + (s + 1) * n * n;
                    indices[s + 1][i][j][1][0] = i * n + j + (s + 1) * n * n;
                    indices[s + 1][i][j][1][1] = (i + 1) * n + j + (s + 1) * n * n;
                    indices[s + 1][i][j][1][2] = (i + 1) * n + j + 1 + (s + 1) * n * n;
                }
            }
        }
        auto numIndices = 36 * (n - 1) * (n - 1);// static_cast<uint32_t>(size(indices));
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