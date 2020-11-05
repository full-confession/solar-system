#pragma once
#include "ECS.hpp"
#include "Window.hpp"
#include "Graphics.hpp"
#include "Mesh.hpp"
#include "ShaderReflection.hpp"
#include "Transform.hpp"
#include "Camera.hpp"
#include "BloomModule.hpp"

namespace SolarSystem
{

    struct Material final
    {
        ResourceHandle<VertexShader> vertexShader;
        ResourceHandle<PixelShader> pixelShader;
        ResourceHandle<ShaderResouceView> pixelShaderResourceViews[4] = { };
        ResourceHandle<Buffer> pixelBuffer;
    };

    struct RendererComponent final
    {
        ResourceHandle<Mesh> mesh;
        ResourceHandle<Material> material;
    };

    struct MeshProvider final { };

    struct MeshRenderer final
    {
        ResourceHandle<PixelShader> pixelShader;
        ResourceHandle<Buffer> pixelCBuffer;
        ResourceHandle<ShaderResouceView> pixelSRV[4] = { };
    };

    class RendererSystem final : public ECSSystem<RendererSystem>
    {
    public:
        auto Initialize() -> void override
        {
            windowSystem = context->GetSystem<WindowSystem>();
            graphicsSystem = context->GetSystem<GraphicsSystem>();
            shaderReflectionSystem = context->GetSystem<ShaderReflectionSystem>();
            worldSystem = context->GetSystem<WorldSystem>();
            cameraSystem = context->GetSystem<CameraSystem>();

            width = windowSystem->GetWidth();
            height = windowSystem->GetHeight();

            swapChain = graphicsSystem->CreateSwapChain({
                static_cast<UINT>(width),
                static_cast<UINT>(height),
                DXGI_FORMAT_B8G8R8A8_UNORM,
                false,
                { 1, 0 },
                DXGI_USAGE_RENDER_TARGET_OUTPUT,
                3,
                DXGI_SCALING_STRETCH,
                DXGI_SWAP_EFFECT_FLIP_DISCARD,
                DXGI_ALPHA_MODE_UNSPECIFIED,
                DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
                }, windowSystem->GetHandle());

            //swapChainBackbuffer = graphicsSystem->GetSwapChainBackbuffer(swapChain);

            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Texture2D.MipSlice = 0;

            scRTV = graphicsSystem->CreateRenderTargetView(swapChain, &rtvDesc);


            //scRTV = graphicsSystem->CreateRenderTargetView(swapChain);

            auto const swapChainDesc = graphicsSystem->GetSwapChainDescription(swapChain);

            auto msaaQuality = graphicsSystem->GetMSAAQuality(DXGI_FORMAT_R16G16B16A16_FLOAT, 4);
            hdrRenderTargetMS = graphicsSystem->CreateTexture2D({
                swapChainDesc.Width,
                swapChainDesc.Height,
                1,
                1,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                //{ 4, static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN) },
                {4, msaaQuality - 1},
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_RENDER_TARGET,
                0,
                0
                });

            dsvMS = graphicsSystem->CreateTexture2D({
                swapChainDesc.Width,
                swapChainDesc.Height,
                1,
                1,
                DXGI_FORMAT_D32_FLOAT,
                //{ 4, static_cast<UINT>(D3D11_STANDARD_MULTISAMPLE_PATTERN) },
                {4, msaaQuality - 1},
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_DEPTH_STENCIL,
                0,
                0
                });

            D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
            dsvDesc.Format = DXGI_FORMAT_UNKNOWN;
            dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
            dsvDesc.Flags = 0;
            msDSV = graphicsSystem->CreateDepthStencilView(dsvMS, dsvDesc);

            hdrRenderTarget = graphicsSystem->CreateTexture2D({
                swapChainDesc.Width,
                swapChainDesc.Height,
                1,
                1,
                DXGI_FORMAT_R16G16B16A16_FLOAT,
                { 1, 0 },
                D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE,
                0,
                0
                });

            uint8_t ditherPattern[] = {
                0, 32,  8, 40,  2, 34, 10, 42,
                48, 16, 56, 24, 50, 18, 58, 26,
                12, 44,  4, 36, 14, 46,  6, 38,
                60, 28, 52, 20, 62, 30, 54, 22,
                3, 35, 11, 43,  1, 33,  9, 41,
                51, 19, 59, 27, 49, 17, 57, 25,
                15, 47,  7, 39, 13, 45,  5, 37,
                63, 31, 55, 23, 61, 29, 53, 21
            };

            D3D11_SUBRESOURCE_DATA ditherData;
            ditherData.pSysMem = ditherPattern;
            ditherData.SysMemPitch = 8;

            auto ditherTex = graphicsSystem->CreateTexture2D({
                8,
                8,
                1,
                1,
                DXGI_FORMAT_R8_UINT,
                {1,0 },
                D3D11_USAGE_IMMUTABLE,
                D3D11_BIND_SHADER_RESOURCE,
                0,
                0
                }, &ditherData);

            auto ditherSRV = graphicsSystem->CreateShaderResourceView(ditherTex);

            //D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtvDesc.Format = DXGI_FORMAT_UNKNOWN;
            rtvDesc.Texture2D.MipSlice = 0;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.MostDetailedMip = 0;



            rtvDesc.Format = DXGI_FORMAT_UNKNOWN;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
            //rtvDesc.Format = DXGI_FORMAT_UNKNOWN;
            //rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
            msRTV = graphicsSystem->CreateRenderTargetView(hdrRenderTargetMS, rtvDesc);


            auto const hdrSRV = graphicsSystem->CreateShaderResourceView(hdrRenderTarget);
            auto const quadVS = graphicsSystem->CreateVertexShader(LoadBytecode("Shaders/FullscreenQuad_vs.cso"));


            pixelBuffer = graphicsSystem->CreateBuffer({
                sizeof(PixelBuffer),
                D3D11_USAGE_DYNAMIC,
                D3D11_BIND_CONSTANT_BUFFER,
                D3D11_CPU_ACCESS_WRITE,
                0,
                0
                }, nullptr);


            auto const quadMesh = CreateMesh(Procedural::FullscreenQuad());



            fullscreenQuad = CreateMeshProvider(
                quadMesh,
                quadVS
            );

            bloomModule.Initialize(graphicsSystem);
            bloomModule.SetSource(hdrSRV);



            tonemappingPass = CreateMeshRenderer({
                graphicsSystem->CreatePixelShader(LoadBytecode("Shaders/ToneMapping_ps.cso")),
                pixelBuffer,
                { hdrSRV, bloomModule.GetDestination(), ditherSRV }
                });



            D3D11_SAMPLER_DESC samplerDesc;
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            samplerDesc.MipLODBias = 0;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.MinLOD = -FLT_MAX;
            samplerDesc.MaxLOD = FLT_MAX;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.BorderColor[0] = 1.0f;
            samplerDesc.BorderColor[1] = 1.0f;
            samplerDesc.BorderColor[2] = 1.0f;
            samplerDesc.BorderColor[3] = 1.0f;

            linearSamplerWrap = graphicsSystem->CreateSamplerState(samplerDesc);
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

            pointSamplerWrap = graphicsSystem->CreateSamplerState(samplerDesc);

            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            pointSamplerClamp = graphicsSystem->CreateSamplerState(samplerDesc);

            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            linearSamplerClamp = graphicsSystem->CreateSamplerState(samplerDesc);

            perObjectVertexCBuffer = graphicsSystem->CreateBuffer({
                sizeof(PerObjectVertexCBuffer),
                D3D11_USAGE_DYNAMIC,
                D3D11_BIND_CONSTANT_BUFFER,
                D3D11_CPU_ACCESS_WRITE,
                0,
                0
                }, nullptr);

            perObjectPixelCBuffer = graphicsSystem->CreateBuffer({
                sizeof(PerObjectPixelCBuffer),
                D3D11_USAGE_DYNAMIC,
                D3D11_BIND_CONSTANT_BUFFER,
                D3D11_CPU_ACCESS_WRITE,
                0,
                0
                }, nullptr);

            msRasterizer = graphicsSystem->CreateRasterizerState({
                D3D11_FILL_SOLID,
                D3D11_CULL_BACK,
                false,
                0,
                0.0f,
                0.0f,
                true,
                false,
                true,
                false
                });


            D3D11_BLEND_DESC blendDesc;
            blendDesc.AlphaToCoverageEnable = false;
            blendDesc.IndependentBlendEnable = false;
            blendDesc.RenderTarget[0].BlendEnable = true;
            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

            alphaBlendState = graphicsSystem->CreateBlendState(blendDesc);


            blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
            blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

            blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;

            blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
            blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;

            addBlendState = graphicsSystem->CreateBlendState(blendDesc);
        }



        auto Update(float, float) -> void override
        {
            graphicsSystem->WaitSwapChain(swapChain);

            //ResizeIfNeeded();

            graphicsSystem->ClearRenderTargetView(msRTV, { 0.0f, 0.0f, 0.0f, 1.0f });
            graphicsSystem->ClearDepthStencilView(msDSV, 1.0f);
            graphicsSystem->SetRasterizerState(msRasterizer);
            graphicsSystem->SetRenderTargets(msRTV, msDSV);
            graphicsSystem->SetViewport({ 0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f });
            graphicsSystem->SetPixelSamplerStates({ linearSamplerWrap, pointSamplerWrap, linearSamplerClamp, pointSamplerClamp });

            //components.Each([this](Entity const entity, RendererComponent const& rendererComponent)
            //{
            //	this->DrawEntity(entity, rendererComponent);
            //});

            for(size_t i = 0; i < replaceComponents.size(); ++i)
            {
                DrawEntity(components.GetEntityFromComponent(replaceComponents[i]), components[replaceComponents[i]]);
            }

            graphicsSystem->SetBlendState(addBlendState);

            for(size_t i = 0; i < addComponents.size(); ++i)
            {
                DrawEntity(components.GetEntityFromComponent(addComponents[i]), components[addComponents[i]]);
            }

            graphicsSystem->SetBlendState(alphaBlendState);

            for(size_t i = 0; i < alphaComponents.size(); ++i)
            {
                DrawEntity(components.GetEntityFromComponent(alphaComponents[i]), components[alphaComponents[i]]);
            }

            graphicsSystem->SetBlendState({ });

            // Resolving multisampling
            graphicsSystem->ResolveMultisampling(hdrRenderTargetMS, hdrRenderTarget, DXGI_FORMAT_R16G16B16A16_FLOAT);
            graphicsSystem->SetRasterizerState({ });


            PixelBuffer buffer = { { static_cast<float>(width), static_cast<float>(height) } };
            graphicsSystem->WriteBuffer(pixelBuffer, &buffer, sizeof(PixelBuffer));


            BindMeshProvider(fullscreenQuad);
            bloomModule.Draw();


            buffer.texSize = DirectX::SimpleMath::Vector2{ static_cast<float>(width), static_cast<float>(height) };
            graphicsSystem->WriteBuffer(pixelBuffer, &buffer, sizeof(PixelBuffer));
            graphicsSystem->SetRenderTargets(scRTV);
            BindMeshRenderer(tonemappingPass);
            Draw();



            graphicsSystem->PresentSwapChain(swapChain);
        }



        auto CreateMesh(Mesh mesh) -> ResourceHandle<Mesh>
        {
            auto& rm = meshes.emplace_back();
            rm.mesh = std::move(mesh);

            assert(rm.mesh.vertexBuffers.size() < 4);
            for(size_t i = 0; i < rm.mesh.vertexBuffers.size(); ++i)
            {
                auto& buffer = rm.mesh.vertexBuffers[i];

                auto data = D3D11_SUBRESOURCE_DATA{ buffer.data.data(), 0, 0 };

                rm.vertexBuffers[i] = graphicsSystem->CreateBuffer({
                    static_cast<UINT>(buffer.vertexByteSize * buffer.vertexCount),
                    D3D11_USAGE_IMMUTABLE,
                    D3D11_BIND_VERTEX_BUFFER,
                    0,
                    0,
                    0
                    }, &data);

                rm.vertexSizes[i] = rm.mesh.vertexBuffers[i].vertexByteSize;
            }

            if(rm.mesh.indexBuffer.indexCount > 0)
            {

                auto data = D3D11_SUBRESOURCE_DATA{ rm.mesh.indexBuffer.data.data(), 0, 0 };

                rm.indexBuffer = graphicsSystem->CreateBuffer({
                    static_cast<UINT>(rm.mesh.indexBuffer.indexCount * GetDXGIFormatSize(rm.mesh.indexBuffer.format)),
                    D3D11_USAGE_IMMUTABLE,
                    D3D11_BIND_INDEX_BUFFER,
                    0,
                    0,
                    0
                    }, &data);
            }

            return ResourceHandle<Mesh>(meshes.size() - 1);
        }


        auto CreateMeshProvider(ResourceHandle<Mesh> const mesh, ResourceHandle<VertexShader> const vertexShader)
            -> ResourceHandle<MeshProvider>
        {
            auto& rmp = meshProviders.emplace_back();

            rmp.mesh = mesh;
            rmp.vertexShader = vertexShader;
            rmp.inputLayout = CreateInputLayout(mesh, vertexShader);

            return ResourceHandle<MeshProvider>(meshProviders.size() - 1);
        }


        auto BindMeshProvider(ResourceHandle<MeshProvider> const meshProvider) -> void
        {
            if(meshProvider.IsNull() || meshProvider.GetValue() > meshProviders.size())
            {
                return;
            }

            auto& rmp = meshProviders[meshProvider.GetValue()];
            auto& rm = GetMesh(rmp.mesh);

            graphicsSystem->BindIndexBuffer(rm.indexBuffer, rm.mesh.indexBuffer.format);
            graphicsSystem->BindVertexBuffers(rm.vertexBuffers, rm.vertexSizes);
            graphicsSystem->BindInputLayout(rmp.inputLayout);
            graphicsSystem->BindPrimitiveTopology(rm.mesh.topology);

            graphicsSystem->BindVertexShader(rmp.vertexShader);


            if(rm.mesh.indexBuffer.indexCount > 0)
            {
                drawType = DrawType::Index;
                vertexCount = rm.mesh.indexBuffer.indexCount;
            }
            else
            {
                drawType = DrawType::Vertex;
                vertexCount = rm.mesh.vertexBuffers[0].vertexCount;
            }
        }

        auto CreateMeshRenderer(MeshRenderer const meshRenderer)
            -> ResourceHandle<MeshRenderer>
        {
            auto& rmr = meshRenderers.emplace_back();

            rmr.meshRenderer = meshRenderer;

            return ResourceHandle<MeshRenderer>(meshRenderers.size() - 1);
        }

        auto BindMeshRenderer(ResourceHandle<MeshRenderer> const meshRenderer) -> void
        {
            if(meshRenderer.IsNull() || meshRenderer.GetValue() > meshRenderers.size())
            {
                return;
            }

            auto& rmr = meshRenderers[meshRenderer.GetValue()];

            graphicsSystem->BindPixelShader(rmr.meshRenderer.pixelShader);
            graphicsSystem->BindPixelConstantBuffer(rmr.meshRenderer.pixelCBuffer);
            graphicsSystem->BindPixelShaderResourceViews(rmr.meshRenderer.pixelSRV);
        }


        auto Draw() -> void
        {
            switch(drawType)
            {
            case DrawType::Vertex:
                graphicsSystem->Draw(vertexCount);
                break;
            case DrawType::Index:
                graphicsSystem->DrawIndexed(vertexCount);
                break;
            }
        }

        auto CreateMaterial(Material const& material) -> ResourceHandle<Material>
        {
            auto& rm = materials.emplace_back();
            rm.material = material;

            auto const bytecode = graphicsSystem->GetVertexShaderBytecode(material.vertexShader);
            rm.vertexShaderReflection = shaderReflectionSystem->Reflect(bytecode.first, bytecode.second);

            return ResourceHandle<Material>(materials.size() - 1);
        }


        enum class BlendMode
        {
            Replace,
            Add,
            Alpha
        };


        auto AddComponent(
            Entity const entity,
            ResourceHandle<Mesh> const mesh,
            ResourceHandle<Material> const material,
            BlendMode const blendMode = BlendMode::Replace
        ) -> void
        {
            RendererComponent component;
            component.mesh = mesh;
            component.material = material;
            component.inputLayout = CreateInputLayout(mesh, material);

            components.AddComponent(entity, component);
            auto const index = components.GetComponentIndex(entity);

            switch(blendMode)
            {
            case BlendMode::Replace:
                replaceComponents.push_back(index);
                break;
            case BlendMode::Add:
                addComponents.push_back(index);
                break;
            case BlendMode::Alpha:
                alphaComponents.push_back(index);
                break;
            }

            /*if(blendMode == BlendMode::Add)
            {
                if(index > firstAlphaComponent)
                {
                    components.SwapComponents(firstAlphaComponent, index);
                }
                firstAlphaComponent++;
            }
            else if(blendMode == BlendMode::Replace)
            {
                if(index > firstAlphaComponent)
                {
                    components.SwapComponents(index, firstAlphaComponent);
                }

                if(index > firstAddComponent)
                {
                    components.SwapComponents(firstAddComponent, index);
                    components.SwapComponents(index, firstAlphaComponent);
                }

                firstAlphaComponent++;
                firstAddComponent++;
            }*/
        }


    private:

        std::vector<size_t> alphaComponents;
        std::vector<size_t> addComponents;
        std::vector<size_t> replaceComponents;
        //size_t firstAlphaComponent = 0;
        //size_t firstAddComponent = 0;

        auto CreateInputLayout(ResourceHandle<Mesh> const mesh, ResourceHandle<Material> const material) -> ResourceHandle<InputLayout>
        {
            auto& rMesh = GetMesh(mesh);
            auto& rMat = GetMaterial(material);

            std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;

            for(auto& inputParameter : rMat.vertexShaderReflection.inputParameters)
            {
                auto isSupplied = false;

                for(auto j = 0; j < rMesh.mesh.vertexBuffers.size(); ++j)
                {
                    auto& vertexBuffer = rMesh.mesh.vertexBuffers[j];

                    for(auto& vertexElement : vertexBuffer.vertexElements)
                    {
                        if(vertexElement.format == inputParameter.format
                            && vertexElement.semanticName == inputParameter.semanticName)
                        {
                            D3D11_INPUT_ELEMENT_DESC desc;
                            desc.Format = vertexElement.format;
                            desc.SemanticName = vertexElement.semanticName.c_str();
                            desc.AlignedByteOffset = vertexElement.offset;
                            desc.SemanticIndex = 0;
                            desc.InputSlot = j;
                            desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                            desc.InstanceDataStepRate = 0;

                            inputElements.push_back(desc);
                            isSupplied = true;
                            break;
                        }
                    }

                    if(isSupplied) break;
                }

                if(!isSupplied)
                {
                    throw std::exception("Vertex shader input is not supplied by mesh vertex buffers");
                }
            }

            return graphicsSystem->CreateInputLayout(std::move(inputElements), rMat.material.vertexShader);
        }


        auto CreateInputLayout(ResourceHandle<Mesh> const mesh, ResourceHandle<VertexShader> const vertexShader) -> ResourceHandle<InputLayout>
        {
            auto& rMesh = GetMesh(mesh);

            auto const bytecode = graphicsSystem->GetVertexShaderBytecode(vertexShader);
            auto const& reflection = shaderReflectionSystem->Reflect(bytecode.first, bytecode.second);

            std::vector<D3D11_INPUT_ELEMENT_DESC> inputElements;

            for(auto& inputParameter : reflection.inputParameters)
            {
                auto isSupplied = false;

                for(auto j = 0; j < rMesh.mesh.vertexBuffers.size(); ++j)
                {
                    auto& vertexBuffer = rMesh.mesh.vertexBuffers[j];

                    for(auto& vertexElement : vertexBuffer.vertexElements)
                    {
                        if(vertexElement.format == inputParameter.format
                            && vertexElement.semanticName == inputParameter.semanticName)
                        {
                            D3D11_INPUT_ELEMENT_DESC desc;
                            desc.Format = vertexElement.format;
                            desc.SemanticName = vertexElement.semanticName.c_str();
                            desc.AlignedByteOffset = vertexElement.offset;
                            desc.SemanticIndex = 0;
                            desc.InputSlot = j;
                            desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
                            desc.InstanceDataStepRate = 0;

                            inputElements.push_back(desc);
                            isSupplied = true;
                            break;
                        }
                    }

                    if(isSupplied) break;
                }

                if(!isSupplied)
                {
                    throw std::exception("Vertex shader input is not supplied by mesh vertex buffers");
                }
            }

            return graphicsSystem->CreateInputLayout(std::move(inputElements), vertexShader);
        }

        WindowSystem* windowSystem = nullptr;
        GraphicsSystem* graphicsSystem = nullptr;
        WorldSystem* worldSystem = nullptr;
        CameraSystem* cameraSystem = nullptr;
        int width = 0;
        int height = 0;

        ShaderReflectionSystem* shaderReflectionSystem = nullptr;

        ResourceHandle<SwapChain> swapChain;
        ResourceHandle<Texture2D> swapChainBackbuffer;
        ResourceHandle<Texture2D> hdrRenderTargetMS;
        ResourceHandle<Texture2D> hdrRenderTarget;
        ResourceHandle<Texture2D> dsvMS;

        ResourceHandle<RenderTargetView> msRTV;
        ResourceHandle<DepthStencilView> msDSV;

        ResourceHandle<RenderTargetView> scRTV;


        // Postprocessing
        ResourceHandle<MeshProvider> fullscreenQuad;

        // Bloom
        BloomModule bloomModule;


        // Tonemapping
        ResourceHandle<MeshRenderer> tonemappingPass;




        ResourceHandle<SamplerState> pointSamplerWrap;
        ResourceHandle<SamplerState> linearSamplerWrap;
        ResourceHandle<SamplerState> pointSamplerClamp;
        ResourceHandle<SamplerState> linearSamplerClamp;

        ResourceHandle<RasterizerState> msRasterizer;

        ResourceHandle<Buffer> perObjectVertexCBuffer;
        ResourceHandle<Buffer> pixelBuffer;
        ResourceHandle<Buffer> perObjectPixelCBuffer;

        ResourceHandle<BlendState> alphaBlendState;
        ResourceHandle<BlendState> addBlendState;
        enum class DrawType
        {
            Vertex,
            Index
        };
        DrawType drawType = DrawType::Vertex;
        int vertexCount = 0;

        struct RMesh final
        {
            Mesh mesh;

            ResourceHandle<Buffer> vertexBuffers[4] = { };
            UINT vertexSizes[4] = { };
            ResourceHandle<Buffer> indexBuffer;
        };
        std::vector<RMesh> meshes;

        auto GetMesh(ResourceHandle<Mesh> const mesh) -> RMesh&
        {
            assert(mesh.GetValue() < meshes.size());
            return meshes[mesh.GetValue()];
        }



        struct RMaterial final
        {
            Material material;
            ShaderReflection vertexShaderReflection;
        };
        std::vector<RMaterial> materials;


        struct RMeshProvider final
        {
            ResourceHandle<Mesh> mesh;
            ResourceHandle<InputLayout> inputLayout;
            ResourceHandle<VertexShader> vertexShader;
        };
        std::vector<RMeshProvider> meshProviders;


        struct RMeshRenderer final
        {
            MeshRenderer meshRenderer;
        };
        std::vector<RMeshRenderer> meshRenderers;

        auto GetMaterial(ResourceHandle<Material> const material) -> RMaterial&
        {
            assert(material.GetValue() < materials.size());
            return materials[material.GetValue()];
        }

        struct RendererComponent final
        {
            ResourceHandle<Mesh> mesh;
            ResourceHandle<InputLayout> inputLayout;
            ResourceHandle<Material> material;
        };
        ComponentHolder<RendererComponent> components;


        RendererComponent toneMappingPostprocess;

        //RendererComponent bloomBrightPass;

        RendererComponent bloomGaussH;

        struct PerObjectVertexCBuffer final
        {
            DirectX::SimpleMath::Matrix wvp;
            DirectX::SimpleMath::Matrix world;
        };

        struct PerObjectPixelCBuffer final
        {
            DirectX::SimpleMath::Vector4 lightDir;
            DirectX::SimpleMath::Vector4 cameraPos;
        };

        struct PixelBuffer final
        {
            DirectX::SimpleMath::Vector2 texSize;
            DirectX::SimpleMath::Vector2 texelSize;
            DirectX::SimpleMath::Vector4 threshold;
        };

        auto DrawEntity(Entity const entity, RendererComponent const& component) -> void
        {
            auto& worldMatrix = worldSystem->GetComponent(entity);

            PerObjectVertexCBuffer vcb;
            vcb.wvp = worldMatrix.world * cameraSystem->GetViewMatrix() * cameraSystem->GetProjectionMatrix();
            vcb.world = worldMatrix.world;

            graphicsSystem->WriteBuffer(perObjectVertexCBuffer, &vcb, sizeof vcb);
            graphicsSystem->BindVertexConstantBuffers({ perObjectVertexCBuffer, { }, { }, { } });

            PerObjectPixelCBuffer pcv;
            auto lightDir = worldMatrix.world.Translation();
            lightDir.Normalize();
            pcv.lightDir = DirectX::SimpleMath::Vector4(lightDir.x, lightDir.y, lightDir.z, 0.0f);
            auto const& cameraPos = cameraSystem->GetPosition();
            pcv.cameraPos = DirectX::SimpleMath::Vector4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);

            graphicsSystem->WriteBuffer(perObjectPixelCBuffer, &pcv, sizeof pcv);
            graphicsSystem->BindPixelConstantBuffer(perObjectPixelCBuffer);

            DrawComponent(component);
        }



        auto DrawComponent(RendererComponent const& component) -> void
        {
            auto& mesh = GetMesh(component.mesh);
            auto& mat = GetMaterial(component.material);


            graphicsSystem->BindIndexBuffer(mesh.indexBuffer, mesh.mesh.indexBuffer.format);
            graphicsSystem->BindVertexBuffers(mesh.vertexBuffers, mesh.vertexSizes);
            graphicsSystem->BindInputLayout(component.inputLayout);
            graphicsSystem->BindPrimitiveTopology(mesh.mesh.topology);

            graphicsSystem->BindVertexShader(mat.material.vertexShader);

            graphicsSystem->BindPixelShader(mat.material.pixelShader);
            graphicsSystem->BindPixelShaderResourceViews(mat.material.pixelShaderResourceViews);
            graphicsSystem->BindPixelConstantBuffer(mat.material.pixelBuffer);

            if(mesh.mesh.indexBuffer.indexCount > 0)
            {
                graphicsSystem->DrawIndexed(mesh.mesh.indexBuffer.indexCount);
            }
            else
            {
                graphicsSystem->Draw(mesh.mesh.vertexBuffers[0].vertexCount);
            }
        }
    };
}
