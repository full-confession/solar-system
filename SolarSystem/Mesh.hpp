#pragma once
#include <vector>
#include <d3d11.h>
#include <SimpleMath.h>
#include <string>


namespace SolarSystem
{
    struct VertexElement final
    {
        std::string semanticName;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        int offset = 0;
    };

    struct VertexBuffer final
    {
        std::vector<VertexElement> vertexElements;
        int vertexByteSize = 0;
        int vertexCount = 0;
        std::vector<char> data;
    };

    struct IndexBuffer final
    {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        int indexCount = 0;
        std::vector<char> data;
    };


    struct Mesh final
    {
        std::vector<VertexBuffer> vertexBuffers;
        IndexBuffer indexBuffer;
        D3D11_PRIMITIVE_TOPOLOGY topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    };


    namespace Procedural
    {

        struct PNUVertex final
        {
            DirectX::SimpleMath::Vector3 position;
            DirectX::SimpleMath::Vector3 normal;
            DirectX::SimpleMath::Vector2 uv;
        };

        auto inline CreateSphere(int const longitudeSides, int const latitudeSides) -> Mesh
        {
            auto vertices = std::vector<PNUVertex>((longitudeSides + 1) * (latitudeSides - 1) + longitudeSides * 2);
            auto indices = std::vector<uint32_t>(longitudeSides * 6 * (latitudeSides - 1));



            // Top vertices
            size_t nextVertex = 0;
            auto const deltaU = 1.0f / longitudeSides;
            auto const deltaV = 1.0f / latitudeSides;

            for(auto i = 0; i < longitudeSides; ++i)
            {
                auto& vertex = vertices[nextVertex++];
                vertex.position = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f);
                vertex.normal = vertex.position;
                vertex.uv = DirectX::SimpleMath::Vector2(
                    deltaU * i + deltaU / 2.0f,
                    0.0f
                );
            }

            // Middle vertices
            for(auto i = 1; i < latitudeSides; i++)
            {
                auto const theta = DirectX::XM_PI / latitudeSides * i;
                for(auto j = 0; j < longitudeSides + 1; j++)
                {
                    auto const phi = 2.0f * DirectX::XM_PI / longitudeSides * j;
                    auto& vertex = vertices[nextVertex++];
                    vertex.position = DirectX::SimpleMath::Vector3(
                        std::sin(theta) * std::cos(phi),
                        std::cos(theta),
                        std::sin(theta) * std::sin(phi)
                    );
                    vertex.normal = vertex.position;
                    vertex.uv = DirectX::SimpleMath::Vector2(
                        deltaU * j,
                        deltaV * i
                    );
                }
            }

            // Bottom vertices
            for(auto i = 0; i < longitudeSides; ++i)
            {
                auto& vertex = vertices[nextVertex++];
                vertex.position = DirectX::SimpleMath::Vector3(0.0f, -1.0f, 0.0f);
                vertex.normal = vertex.position;
                vertex.uv = DirectX::SimpleMath::Vector2(
                    deltaU * i + deltaU / 2.0f,
                    1.0f
                );
            }


            auto nextIndex = 0;
            // Top triangles
            for(auto i = 0; i < longitudeSides; i++)
            {
                indices[nextIndex++] = i;
                indices[nextIndex++] = i + longitudeSides + 1;
                indices[nextIndex++] = i + longitudeSides;
            }

            //Middle
            auto offset = longitudeSides;
            for(auto i = 0; i < latitudeSides - 2; i++)
            {
                for(auto j = 0; j < longitudeSides; j++)
                {
                    indices[nextIndex++] = offset + (longitudeSides + 1) * i + j;
                    indices[nextIndex++] = offset + (longitudeSides + 1) * i + j + 1;
                    indices[nextIndex++] = offset + (longitudeSides + 1) * (i + 1) + j;

                    indices[nextIndex++] = offset + (longitudeSides + 1) * i + j + 1;
                    indices[nextIndex++] = offset + (longitudeSides + 1) * (i + 1) + j + 1;
                    indices[nextIndex++] = offset + (longitudeSides + 1) * (i + 1) + j;
                }
            }

            //Bottom
            offset = longitudeSides + (longitudeSides + 1) * (latitudeSides - 2);
            for(auto i = 0; i < longitudeSides; i++)
            {
                indices[nextIndex++] = offset + i;
                indices[nextIndex++] = offset + i + 1;
                indices[nextIndex++] = offset + i + longitudeSides + 1;
            }



            VertexBuffer vb;
            vb.vertexByteSize = sizeof(PNUVertex);
            vb.vertexCount = static_cast<int>(vertices.size());
            vb.vertexElements.push_back({ "POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0 });
            vb.vertexElements.push_back({ "NORMAL", DXGI_FORMAT_R32G32B32_FLOAT, sizeof PNUVertex::position });
            vb.vertexElements.push_back({ "UV", DXGI_FORMAT_R32G32_FLOAT, sizeof PNUVertex::position + sizeof PNUVertex::normal });
            vb.data.resize(vertices.size() * sizeof(PNUVertex));
            std::memcpy(vb.data.data(), vertices.data(), vb.data.size());

            IndexBuffer ib;
            ib.format = DXGI_FORMAT_R32_UINT;
            ib.indexCount = static_cast<int>(indices.size());
            ib.data.resize(indices.size() * sizeof(uint32_t));
            std::memcpy(ib.data.data(), indices.data(), ib.data.size());


            Mesh mesh;
            mesh.vertexBuffers.push_back(std::move(vb));
            mesh.indexBuffer = std::move(ib);
            mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            return mesh;
        }


        struct PUVertex final
        {
            DirectX::SimpleMath::Vector3 position;
            DirectX::SimpleMath::Vector2 uv;
        };
        
        auto inline CreateRing(int const sides, float const innerRadius, float const outerRadius)
        {
            auto vertices = std::vector<PUVertex>(sides * 4);
            auto indices = std::vector<uint32_t>(sides * 6 * 2);

            // vertices
            for(auto i = 0; i < sides; i++)
            {
                auto angle1 = DirectX::XM_PI * 2.0f / sides * i;
                auto angle2 = DirectX::XM_PI * 2.0f / sides * (i + 1);

                auto const offset = i * 4;

                vertices[offset].position = DirectX::SimpleMath::Vector3(
                    std::cos(angle1) * innerRadius,
                    0.0f,
                    std::sin(angle1) * innerRadius
                );
                vertices[offset].uv = DirectX::SimpleMath::Vector2(0.0f, 0.0f);

                vertices[offset + 1].position = DirectX::SimpleMath::Vector3(
                    std::cos(angle2) * innerRadius,
                    0.0f,
                    std::sin(angle2) * innerRadius
                );
                vertices[offset + 1].uv = DirectX::SimpleMath::Vector2(0.0f, 1.0f);

                vertices[offset + 2].position = DirectX::SimpleMath::Vector3(
                    std::cos(angle2) * outerRadius,
                    0.0f,
                    std::sin(angle2) * outerRadius
                );
                vertices[offset + 2].uv = DirectX::SimpleMath::Vector2(1.0f, 1.0f);

                vertices[offset + 3].position = DirectX::SimpleMath::Vector3(
                    std::cos(angle1) * outerRadius,
                    0.0f,
                    std::sin(angle1) * outerRadius
                );
                vertices[offset + 3].uv = DirectX::SimpleMath::Vector2(1.0f, 0.0f);
            }

            // triangles
            for(auto i = 0; i < sides; i++)
            {
                auto offset = i * 12;
                auto const vertexOffset = i * 4;

                indices[offset++] = vertexOffset;
                indices[offset++] = vertexOffset + 1;
                indices[offset++] = vertexOffset + 2;

                indices[offset++] = vertexOffset;
                indices[offset++] = vertexOffset + 2;
                indices[offset++] = vertexOffset + 1;

                indices[offset++] = vertexOffset;
                indices[offset++] = vertexOffset + 2;
                indices[offset++] = vertexOffset + 3;

                indices[offset++] = vertexOffset;
                indices[offset++] = vertexOffset + 3;
                indices[offset] = vertexOffset + 2;
            }

            VertexBuffer vb;
            vb.vertexByteSize = sizeof(PUVertex);
            vb.vertexCount = static_cast<int>(vertices.size());
            vb.vertexElements.push_back({ "POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0 });
            vb.vertexElements.push_back({ "UV", DXGI_FORMAT_R32G32_FLOAT, sizeof PUVertex::position });
            vb.data.resize(vertices.size() * sizeof(PUVertex));
            std::memcpy(vb.data.data(), vertices.data(), vb.data.size());

            IndexBuffer ib;
            ib.format = DXGI_FORMAT_R32_UINT;
            ib.indexCount = static_cast<int>(indices.size());
            ib.data.resize(indices.size() * sizeof(uint32_t));
            std::memcpy(ib.data.data(), indices.data(), ib.data.size());


            Mesh mesh;
            mesh.vertexBuffers.push_back(std::move(vb));
            mesh.indexBuffer = std::move(ib);
            mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            return mesh;
        }
        
        struct PVertex final
        {
            DirectX::SimpleMath::Vector3 position;
        };

        auto inline FullscreenQuad() -> Mesh
        {
            auto vertices = std::vector<PVertex>(3);

            vertices[0].position = DirectX::SimpleMath::Vector3(-2.0f, -2.0f, 0.0f);
            vertices[1].position = DirectX::SimpleMath::Vector3(-2.0f,  5.0f, 0.0f);
            vertices[2].position = DirectX::SimpleMath::Vector3( 5.0f, -2.0f, 0.0f);


            VertexBuffer vb;
            vb.vertexByteSize = sizeof(PVertex);
            vb.vertexCount = static_cast<int>(vertices.size());
            vb.vertexElements.push_back({ "POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0 });
            vb.data.resize(vertices.size() * sizeof(PVertex));
            std::memcpy(vb.data.data(), vertices.data(), vb.data.size());

            Mesh mesh;
            mesh.vertexBuffers.push_back(std::move(vb));
            mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

            return mesh;
        }

        struct PCVertex final
        {
            DirectX::SimpleMath::Vector3 position;
            DirectX::SimpleMath::Color color;
        };

        auto inline Circle(float const radius, int const segments, DirectX::SimpleMath::Color const color) -> Mesh
        {
            auto const half = segments / 2;
            auto vertices = std::vector<PCVertex>(2 * (half + 1));


            auto const max = DirectX::XM_PI * 0.9f;
            for(auto i = 0; i < half + 1; ++i)
            {

                auto const t = (DirectX::XM_PI - max) + max / static_cast<float>(half) * i;
                vertices[i].position.z = std::cos(t) * radius;
                vertices[i].position.x = std::sin(t) * radius;

                auto const x = (t - DirectX::XM_PI + max) / DirectX::XM_PI;
                vertices[i].color = color;
                vertices[i].color.w = color.w * std::powf(x, 0.5f);
            }

            auto const offset = half + 1;
            for(auto i = 0; i < half + 1; ++i)
            {

                auto const t = DirectX::XM_PI + max / static_cast<float>(half) * i;
                vertices[i + offset].position.z = std::cos(t) * radius;
                vertices[i + offset].position.x = std::sin(t) * radius;

                auto const x = (t - DirectX::XM_PI) / max;
                vertices[i + offset].color = color;
                vertices[i + offset].color.w = color.w * std::powf(1 - x, 0.5f);
            }
            
            //auto const offset = half + 1;

            //for(auto i = 0; i < half + 1; ++i)
            //{

            //	auto const t = max / static_cast<float>(half) * i;
            //	vertices[i + offset].position.z = std::cos(-t);
            //	vertices[i + offset].position.x = std::sin(-t);

            //	auto const x = t / max;
            //	vertices[i + offset].color = color;
            //	vertices[i + offset].color.w = color.w * std::powf(1 - x, 0.3f);
            //}


            VertexBuffer vb;
            vb.vertexByteSize = sizeof(PCVertex);
            vb.vertexCount = static_cast<int>(vertices.size());
            vb.vertexElements.push_back({ "POSITION", DXGI_FORMAT_R32G32B32_FLOAT, 0 });
            vb.vertexElements.push_back({ "COLOR", DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof PCVertex::position });
            vb.data.resize(vertices.size() * sizeof(PCVertex));
            std::memcpy(vb.data.data(), vertices.data(), vb.data.size());

            
            Mesh mesh;
            mesh.vertexBuffers.push_back(std::move(vb));
            mesh.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;

            return mesh;
        }
    }
}
