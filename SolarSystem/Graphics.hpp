#pragma once
#include "ECS.hpp"
#include "IUnknownUniquePtr.hpp"
#include "ResourceHandle.hpp"
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <fstream>
#include <DDSTextureLoader.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


namespace std
{
    auto inline operator==(D3D11_INPUT_ELEMENT_DESC const& left, D3D11_INPUT_ELEMENT_DESC const& right) -> bool
    {
        return left.Format == right.Format
            && left.InputSlot == right.InputSlot
            && left.AlignedByteOffset == right.AlignedByteOffset
            && std::strcmp(left.SemanticName, right.SemanticName) == 0
            && left.SemanticIndex == right.SemanticIndex
            && left.InputSlotClass == right.InputSlotClass
            && left.InstanceDataStepRate == right.InstanceDataStepRate;
    }
}


namespace SolarSystem
{

    struct Texture2D final { };
    struct Texture1D final { };
    struct Texture3D final { };
    struct SwapChain final { };
    struct RenderTargetView final { };
    struct DepthStencilView final { };
    struct Buffer final { };
    struct VertexShader final { };
    struct PixelShader final { };
    struct InputLayout final { };
    struct ShaderResouceView final { };
    struct SamplerState final { };
    struct RasterizerState final { };
    struct BlendState final { };

    class GraphicsSystem final : public ECSSystem<GraphicsSystem>
    {
    public:
        auto Initialize() -> void override
        {
            UINT flags = 0;

#if defined(_DEBUG)
            flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

            auto device = IUnknownUniquePtr<ID3D11Device>();
            auto deviceContext = IUnknownUniquePtr<ID3D11DeviceContext>();
            auto featureLevel = D3D_FEATURE_LEVEL{};

            ThrowIfFailed(D3D11CreateDevice(
                nullptr,
                D3D_DRIVER_TYPE_HARDWARE,
                nullptr,
                flags,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                device.ResetAndGetAddress(),
                &featureLevel,
                deviceContext.ResetAndGetAddress()
            ), "Failed to create device");

            ThrowIfFailed(device->QueryInterface(IID_PPV_ARGS(this->device.ResetAndGetAddress())),
                "Failed to query required device interface");

            ThrowIfFailed(deviceContext->QueryInterface(IID_PPV_ARGS(this->deviceContext.ResetAndGetAddress())),
                "Failed to query required device context interface");

            ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(factory.ResetAndGetAddress())),
                "Failed to create factory");
        }



        auto CreateTexture2D(D3D11_TEXTURE2D_DESC const& desc, D3D11_SUBRESOURCE_DATA const* const initialData = nullptr) -> ResourceHandle<Texture2D>
        {
            auto& rt2 = texture2Ds.emplace_back();

            ThrowIfFailed(device->CreateTexture2D(&desc, initialData, rt2.texture2D.ResetAndGetAddress()),
                "Failed to create texture 2D");

            return ResourceHandle<Texture2D>(texture2Ds.size() - 1);
        }

        auto CreateTexture1D(D3D11_TEXTURE1D_DESC const& desc, D3D11_SUBRESOURCE_DATA const* const initialData = nullptr) -> ResourceHandle<Texture1D>
        {
            auto& rt1 = texture1Ds.emplace_back();

            ThrowIfFailed(device->CreateTexture1D(&desc, initialData, rt1.texture1D.ResetAndGetAddress()),
                "Failed to create texture 1D");

            return ResourceHandle<Texture1D>(texture1Ds.size() - 1);
        }

        auto CreateTexture3D(D3D11_TEXTURE3D_DESC const& desc, D3D11_SUBRESOURCE_DATA const* const initialData = nullptr) -> ResourceHandle<Texture3D>
        {
            auto& rt3 = texture3Ds.emplace_back();

            ThrowIfFailed(device->CreateTexture3D(&desc, initialData, rt3.texture3D.ResetAndGetAddress()),
                "Failed to create texture 1D");

            return ResourceHandle<Texture3D>(texture3Ds.size() - 1);
        }

        auto CreateSwapChain(DXGI_SWAP_CHAIN_DESC1 const& desc, HWND const hWnd) -> ResourceHandle<SwapChain>
        {	
            auto sc1 = IUnknownUniquePtr<IDXGISwapChain1>();
            ThrowIfFailed(factory->CreateSwapChainForHwnd(
                device.Get(),
                hWnd,
                &desc,
                nullptr,
                nullptr,
                sc1.ResetAndGetAddress()
            ), "Failed to create swap chain");

            auto& rsc = swapChains.emplace_back();
            ThrowIfFailed(sc1->QueryInterface(IID_PPV_ARGS(rsc.swapChain.ResetAndGetAddress())), "Failed to query interface");

            rsc.waitable = rsc.swapChain->GetFrameLatencyWaitableObject();
            if(!rsc.waitable)
            {
                throw std::exception("Failed to get swap chain waitable object");
            }
            ThrowIfFailed(rsc.swapChain->SetMaximumFrameLatency(2), "Failed set max frame latency");


            /*auto& rt2 = texture2Ds.emplace_back();
            ThrowIfFailed(rsc.swapChain->GetBuffer(0, IID_PPV_ARGS(rt2.texture2D.ResetAndGetAddress())),
                "Failed to get swap chain back buffer");

            rsc.backBuffer = ResourceHandle<Texture2D>(texture2Ds.size() - 1);*/

            return ResourceHandle<SwapChain>(swapChains.size() - 1);
        }



        auto ResizeSwapChain(ResourceHandle<SwapChain> const swapChain, UINT const width = 0, UINT const height = 0) -> void
        {
            auto& rsc = GetSwapChain(swapChain);




            ThrowIfFailed(rsc.swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0),
                "Failed to resize swap chain");
        }
 
        auto CreateRenderTargetView(ResourceHandle<SwapChain> const swapChain, D3D11_RENDER_TARGET_VIEW_DESC const* desc = nullptr) -> ResourceHandle<RenderTargetView>
        {
            auto& rsc = GetSwapChain(swapChain);
            auto backbuffer = IUnknownUniquePtr<ID3D11Texture2D>();

            ThrowIfFailed(rsc.swapChain->GetBuffer(0, IID_PPV_ARGS(backbuffer.ResetAndGetAddress())),
                "Failed to get swap chain back buffer");

            auto& rrtv = renderTargetViews.emplace_back();
            rrtv.resourceType = RTVResourceType::SwapChain;
            rrtv.swapChain = swapChain;

            ThrowIfFailed(device->CreateRenderTargetView(backbuffer.Get(), desc, rrtv.renderTargetView.ResetAndGetAddress()),
                "Failed to create render target view");

            return ResourceHandle<RenderTargetView>(renderTargetViews.size() - 1);
        }



        auto GetSwapChainDescription(ResourceHandle<SwapChain> const swapChain) -> DXGI_SWAP_CHAIN_DESC1
        {
            auto& rsc = GetSwapChain(swapChain);

            DXGI_SWAP_CHAIN_DESC1 desc;
            ThrowIfFailed(rsc.swapChain->GetDesc1(&desc),
                "Failed to get swap chain description");

            return desc;
        }


        auto WaitSwapChain(ResourceHandle<SwapChain> const swapChain) -> void
        {
            auto& rsc = GetSwapChain(swapChain);

            WaitForSingleObjectEx(
                rsc.waitable,
                1000,
                true
            );
        }



        auto PresentSwapChain(ResourceHandle<SwapChain> const swapChain) -> void
        {
            auto& rsc = GetSwapChain(swapChain);
            ThrowIfFailed(rsc.swapChain->Present(1, 0), "Failed to present swap chain");
        }



        auto CreateRenderTargetView(
            ResourceHandle<Texture2D> const texture2D, 
            D3D11_RENDER_TARGET_VIEW_DESC const& desc
        ) -> ResourceHandle<RenderTargetView>
        {
            auto& rt2 = GetTexture2D(texture2D);

            auto& rrtv = renderTargetViews.emplace_back();

            ThrowIfFailed(device->CreateRenderTargetView(rt2.texture2D.Get(), &desc, rrtv.renderTargetView.ResetAndGetAddress()),
                "Failed to create render target view");

            return ResourceHandle<RenderTargetView>(renderTargetViews.size() - 1);
        }



        auto ClearRenderTargetView(ResourceHandle<RenderTargetView> const renderTargetView, float const(&color)[4]) -> void
        {
            auto& rrtv = GetRenderTargetView(renderTargetView);
            deviceContext->ClearRenderTargetView(rrtv.renderTargetView.Get(), color);
        }


        auto CreateDepthStencilView(
            ResourceHandle<Texture2D> const texture2D,
            D3D11_DEPTH_STENCIL_VIEW_DESC const& desc
        ) -> ResourceHandle<DepthStencilView>
        {
            auto& rt2 = GetTexture2D(texture2D);

            auto& rdsv = depthStencilViews.emplace_back();
            ThrowIfFailed(device->CreateDepthStencilView(rt2.texture2D.Get(), &desc, rdsv.depthStencilView.ResetAndGetAddress()),
                "Failed to create depth stencil view");

            return ResourceHandle<DepthStencilView>(depthStencilViews.size() - 1);
        }


        auto ClearDepthStencilView(
            ResourceHandle<DepthStencilView> const depthStencilView,
            float const value
        ) -> void
        {
            auto& rdsv = GetDepthStencilView(depthStencilView);
            deviceContext->ClearDepthStencilView(rdsv.depthStencilView.Get(), D3D11_CLEAR_DEPTH, value, 0);
        }

        auto SetRenderTargets(ResourceHandle<RenderTargetView> const renderTarget, ResourceHandle<DepthStencilView> const depthStencilView = { }) -> void
        {
            ID3D11RenderTargetView* rtv = nullptr;
            ID3D11DepthStencilView* dsv = nullptr;
            if(!renderTarget.IsNull())
            {
                rtv = GetRenderTargetView(renderTarget).renderTargetView.Get();
                //deviceContext->OMSetRenderTargets(1, &rtv, nullptr);
            }
            /*else
            {
                auto& rrtv = GetRenderTargetView(renderTarget);
                deviceContext->OMSetRenderTargets(1, rrtv.renderTargetView.GetAddress(), nullptr);
            }*/

            if(!depthStencilView.IsNull())
            {
                dsv = GetDepthStencilView(depthStencilView).depthStencilView.Get();
            }

            deviceContext->OMSetRenderTargets(1, &rtv, dsv);
        }



        auto SetViewport(D3D11_VIEWPORT const& viewport) -> void
        {
            deviceContext->RSSetViewports(1, &viewport);
        }



        auto ResolveMultisampling(ResourceHandle<Texture2D> const src, ResourceHandle<Texture2D> const dst, DXGI_FORMAT const format) -> void
        {
            auto& srcTex = GetTexture2D(src);
            auto& dstTex = GetTexture2D(dst);

            deviceContext->ResolveSubresource(dstTex.texture2D.Get(), 0, srcTex.texture2D.Get(), 0, format);
        }


        auto GetMSAAQuality(DXGI_FORMAT const format, int const sampleCount) -> UINT
        {
            UINT value = 0;
            ThrowIfFailed(device->CheckMultisampleQualityLevels(format, sampleCount, &value), 
                "Failed to check msaa quality");
            return value;
        }



        auto GenerateMips(ResourceHandle<ShaderResouceView> const shaderResourceView) -> void
        {
            auto& srv = GetShaderResourceView(shaderResourceView);
            deviceContext->GenerateMips(srv.shaderResourceView.Get());
        }
        

        auto CreateBuffer(D3D11_BUFFER_DESC const& bufferDesc, D3D11_SUBRESOURCE_DATA const* const data) -> ResourceHandle<Buffer>
        {
            auto& rb = buffers.emplace_back();

            ThrowIfFailed(device->CreateBuffer(&bufferDesc, data, rb.buffer.ResetAndGetAddress()),
                "Failed create buffer");

            return ResourceHandle<Buffer>(buffers.size() - 1);
        }



        auto BindVertexBuffers(
            ResourceHandle<Buffer> const (&vertexBuffers)[4],
            UINT const (&vertexSizes)[4]
        ) -> void
        {
            auto i = 0;
            for(; i < 4; ++i)
            {
                if(boundVertexBuffers[i] != vertexBuffers[i])
                {
                    break;
                }
            }

            if(i == 4) return;


            ID3D11Buffer* buffers[4];
            UINT strides[4];
            UINT offsets[4];

            for(auto j = 0; j < 4; ++j)
            {
                if(vertexBuffers[j].IsNull())
                {
                    buffers[j] = nullptr;
                    strides[j] = 0;
                    offsets[j] = 0;
                }
                else
                {
                    buffers[j] = GetBuffer(vertexBuffers[j]).buffer.Get();
                    strides[j] = vertexSizes[j];
                    offsets[j] = 0;
                }

                boundVertexBuffers[j] = vertexBuffers[j];
            }

            deviceContext->IASetVertexBuffers(0, 4, buffers, strides, offsets);
        }



        auto BindIndexBuffer(ResourceHandle<Buffer> const indexBuffer, DXGI_FORMAT const format) -> void
        {
            if(boundIndexBuffer == indexBuffer)
            {
                return;
            }

            ID3D11Buffer* buffer = nullptr;
            UINT offset = 0;

            if(!indexBuffer.IsNull())
            {
                auto& rb = GetBuffer(indexBuffer);
                buffer = rb.buffer.Get();
            }

            boundIndexBuffer = indexBuffer;
            deviceContext->IASetIndexBuffer(buffer, format, offset);
        }



        auto CreateVertexShader(std::vector<char> bytecode) -> ResourceHandle<VertexShader>
        {
            auto& rvs = vertexShaders.emplace_back();
            rvs.bytecode = std::move(bytecode);

            ThrowIfFailed(device->CreateVertexShader(rvs.bytecode.data(), rvs.bytecode.size(), nullptr, rvs.vertexShader.ResetAndGetAddress()),
                "Failed to create vertex shader");

            return ResourceHandle<VertexShader>(vertexShaders.size() - 1);
        }



        auto BindVertexShader(ResourceHandle<VertexShader> const vertexShader) -> void
        {
            if(boundVertexShader == vertexShader)
            {
                return;
            }

            if(vertexShader.IsNull())
            {
                deviceContext->VSSetShader(nullptr, nullptr, 0);
            }
            else
            {
                auto& rvs = GetVertexShader(vertexShader);
                deviceContext->VSSetShader(rvs.vertexShader.Get(), nullptr, 0);
            }

            boundVertexShader = vertexShader;
        }



        auto GetVertexShaderBytecode(ResourceHandle<VertexShader> const vertexShader) -> std::pair<void const*, size_t>
        {
            auto& rvs = GetVertexShader(vertexShader);

            return { rvs.bytecode.data(), rvs.bytecode.size() };
        }


        struct ResourceDimensions final
        {
            int width = 0;
            int height = 0;
        };

        auto GetResourceDimensions(ResourceHandle<ShaderResouceView> const srv) -> ResourceDimensions
        {
            auto& rsrv = GetShaderResourceView(srv);

            if(rsrv.resourceType == SRVResourceType::Texture2D)
            {
                auto& rt2 = GetTexture2D(rsrv.texture2D);

                D3D11_TEXTURE2D_DESC desc;
                rt2.texture2D->GetDesc(&desc);

                return { static_cast<int>(desc.Width), static_cast<int>(desc.Height) };
            }

            return { 0, 0 };
        }



        auto CreatePixelShader(std::vector<char> bytecode) -> ResourceHandle<PixelShader>
        {
            auto& rps = pixelShaders.emplace_back();
            rps.bytecode = std::move(bytecode);

            ThrowIfFailed(device->CreatePixelShader(rps.bytecode.data(), rps.bytecode.size(), nullptr, rps.pixelShader.ResetAndGetAddress()),
                "Failed to create vertex shader");

            return ResourceHandle<PixelShader>(pixelShaders.size() - 1);
        }



        auto BindPixelShader(ResourceHandle<PixelShader> const pixelShader) -> void
        {
            if(boundPixelShader == pixelShader)
            {
                return;
            }

            if(pixelShader.IsNull())
            {
                deviceContext->PSSetShader(nullptr, nullptr, 0);
            }
            else
            {
                auto& rps = GetPixelShader(pixelShader);
                deviceContext->PSSetShader(rps.pixelShader.Get(), nullptr, 0);
            }

            boundPixelShader = pixelShader;
        }


        auto BindPixelConstantBuffer(ResourceHandle<Buffer> const pixelBuffer) -> void
        {
            if(boundPixelBuffer == pixelBuffer)
            {
                return;
            }
            
            if(pixelBuffer.IsNull())
            {
                deviceContext->PSSetConstantBuffers(0, 0, nullptr);
            }
            else
            {
                auto& buffer = GetBuffer(pixelBuffer);
                deviceContext->PSSetConstantBuffers(0, 1, buffer.buffer.GetAddress());
            }

            boundPixelBuffer = pixelBuffer;
        }



        auto CreateInputLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> inputElemets, ResourceHandle<VertexShader> const vertexShader) -> ResourceHandle<InputLayout>
        {
            for(size_t i = 0; i < inputLayouts.size(); ++i)
            {
                if(inputLayouts[i].inputElemets == inputElemets)
                {
                    return ResourceHandle<InputLayout>(i);
                }
            }

            auto& rvs = GetVertexShader(vertexShader);
            auto& ril = inputLayouts.emplace_back();
            ril.inputElemets = std::move(inputElemets);


            ThrowIfFailed(device->CreateInputLayout(
                ril.inputElemets.data(),
                static_cast<UINT>(ril.inputElemets.size()),
                rvs.bytecode.data(),
                rvs.bytecode.size(),
                ril.inputLayout.ResetAndGetAddress()
            ), "Failed to create input layout");

            return ResourceHandle<InputLayout>(inputLayouts.size() - 1);
        }



        auto BindInputLayout(ResourceHandle<InputLayout> const inputLayout) -> void
        {
            if(boundInputLayout == inputLayout)
            {
                return;
            }

            if(inputLayout.IsNull())
            {
                deviceContext->IASetInputLayout(nullptr);
            }
            else
            {
                auto& ril = GetInputLayout(inputLayout);
                deviceContext->IASetInputLayout(ril.inputLayout.Get());
            }

            boundInputLayout = inputLayout;
        }



        auto BindPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY const primitiveTopology) -> void
        {
            if(boundPrimitiveTopology == primitiveTopology)
            {
                return;
            }

            deviceContext->IASetPrimitiveTopology(primitiveTopology);
            boundPrimitiveTopology = primitiveTopology;
        }



        auto CreateShaderResourceView(ResourceHandle<Texture2D> const texture2D, D3D11_SHADER_RESOURCE_VIEW_DESC const* const desc = nullptr)
            -> ResourceHandle<ShaderResouceView>
        {
            auto& rt2 = GetTexture2D(texture2D);
            auto& rsrv = shaderResourceViews.emplace_back();

            rsrv.resourceType = SRVResourceType::Texture2D;
            rsrv.texture2D = texture2D;
            

            ThrowIfFailed(device->CreateShaderResourceView(rt2.texture2D.Get(), desc, rsrv.shaderResourceView.ResetAndGetAddress()),
                "Failed to create shader resource view");

            return ResourceHandle<ShaderResouceView>(shaderResourceViews.size() - 1);
        }


        auto CreateShaderResourceView(ResourceHandle<Texture1D> const texture1D, D3D11_SHADER_RESOURCE_VIEW_DESC const* const desc = nullptr)
            -> ResourceHandle<ShaderResouceView>
        {
            auto& rt1 = GetTexture1D(texture1D);
            auto& rsrv = shaderResourceViews.emplace_back();

            rsrv.resourceType = SRVResourceType::Texture1D;
            rsrv.texture1D = texture1D;


            ThrowIfFailed(device->CreateShaderResourceView(rt1.texture1D.Get(), desc, rsrv.shaderResourceView.ResetAndGetAddress()),
                "Failed to create shader resource view");

            return ResourceHandle<ShaderResouceView>(shaderResourceViews.size() - 1);
        }


        auto CreateShaderResourceView(ResourceHandle<Texture3D> const texture3D, D3D11_SHADER_RESOURCE_VIEW_DESC const* const desc = nullptr)
            -> ResourceHandle<ShaderResouceView>
        {
            auto& rt3 = GetTexture3D(texture3D);
            auto& rsrv = shaderResourceViews.emplace_back();

            rsrv.resourceType = SRVResourceType::Texture3D;
            rsrv.texture3D = texture3D;


            ThrowIfFailed(device->CreateShaderResourceView(rt3.texture3D.Get(), desc, rsrv.shaderResourceView.ResetAndGetAddress()),
                "Failed to create shader resource view");

            return ResourceHandle<ShaderResouceView>(shaderResourceViews.size() - 1);
        }


        auto BindPixelShaderResourceViews(ResourceHandle<ShaderResouceView> const (&shaderResourceViews)[4]) -> void
        {
            auto i = 0;
            for(; i < 4; ++i)
            {
                if(boundPixelShaderResourceViews[i] != shaderResourceViews[i])
                {
                    break;
                }
            }

            if(i == 4) return;


            ID3D11ShaderResourceView* views[4];

            for(auto j = 0; j < 4; ++j)
            {
                if(shaderResourceViews[j].IsNull())
                {
                    views[j] = nullptr;
                }
                else
                {
                    views[j] = GetShaderResourceView(shaderResourceViews[j]).shaderResourceView.Get();
                }

                boundPixelShaderResourceViews[j] = shaderResourceViews[j];
            }

            deviceContext->PSSetShaderResources(0, 4, views);
        }



        auto CreateSamplerState(D3D11_SAMPLER_DESC const& description) -> ResourceHandle<SamplerState>
        {
            auto& rss = samplerStates.emplace_back();

            ThrowIfFailed(device->CreateSamplerState(&description, rss.samplerState.ResetAndGetAddress()),
                "Failed to create sampler state");

            return ResourceHandle<SamplerState>(samplerStates.size() - 1);
        }



        auto SetPixelSamplerStates(ResourceHandle<SamplerState> const (&samplerSatates)[4]) -> void
        {
            ID3D11SamplerState* states[4];
            for(auto i = 0; i < 4; ++i)
            {
                if(samplerSatates[i].IsNull())
                {
                    states[i] = nullptr;
                }
                else
                {
                    states[i] = GetSamplerState(samplerSatates[i]).samplerState.Get();
                }
            }

            deviceContext->PSSetSamplers(0, 4, states);
        }


        auto CreateBlendState(D3D11_BLEND_DESC const& desc) -> ResourceHandle<BlendState>
        {
            auto& rbs = blendStates.emplace_back();

            ThrowIfFailed(device->CreateBlendState(&desc, rbs.blendState.ResetAndGetAddress()),
                "Failed to create blend state");

            return ResourceHandle<BlendState>(blendStates.size() - 1);
        }

        auto SetBlendState(ResourceHandle<BlendState> const blendState) -> void
        {
            if(!blendState.IsNull())
            {
                auto& rbs = GetBlendState(blendState);
                deviceContext->OMSetBlendState(rbs.blendState.Get(), nullptr, 0xffffffff);
            }
            else
            {
                deviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
            }
        }

        auto BindVertexConstantBuffers(ResourceHandle<Buffer> const (&constantBuffers)[4]) -> void
        {
            auto i = 0;
            for(; i < 4; ++i)
            {
                if(boundVertexConstantBuffers[i] != constantBuffers[i])
                {
                    break;
                }
            }

            if(i == 4) return;


            ID3D11Buffer* cbuffers[4];

            for(auto j = 0; j < 4; ++j)
            {
                if(constantBuffers[j].IsNull())
                {
                    cbuffers[j] = nullptr;
                }
                else
                {
                    cbuffers[j] = GetBuffer(constantBuffers[j]).buffer.Get();
                }

                boundVertexConstantBuffers[j] = constantBuffers[j];
            }

            deviceContext->VSSetConstantBuffers(0, 4, cbuffers);
        }



        auto WriteBuffer(ResourceHandle<Buffer> const buffer, void const* data, size_t const dataSize) -> void
        {
            auto& rb = GetBuffer(buffer);

            D3D11_MAPPED_SUBRESOURCE map;
            ThrowIfFailed(deviceContext->Map(rb.buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map),
                "Failed to map buffer");
            std::memcpy(map.pData, data, dataSize);
            deviceContext->Unmap(rb.buffer.Get(), 0);
        }



        auto CreateRasterizerState(D3D11_RASTERIZER_DESC const& description) -> ResourceHandle<RasterizerState>
        {
            auto& rrs = rasterizerStates.emplace_back();

            ThrowIfFailed(device->CreateRasterizerState(&description, rrs.rasterizerState.ResetAndGetAddress()),
                "Failed to create rasterizer state");

            return ResourceHandle<RasterizerState>(rasterizerStates.size() - 1);
        }



        auto SetRasterizerState(ResourceHandle<RasterizerState> const rasterizerState) -> void
        {
            if(rasterizerState.IsNull())
            { 
                deviceContext->RSSetState(nullptr);
            }
            else
            {
                auto& rrs = GetRasterizerState(rasterizerState);
                deviceContext->RSSetState(rrs.rasterizerState.Get());
            }
        }



        auto LoadTexture2D(wchar_t const* const fileName) -> ResourceHandle<ShaderResouceView>
        {
            
            ID3D11Resource* resource;
            ID3D11ShaderResourceView* shaderResourceView;

            ThrowIfFailed(DirectX::CreateDDSTextureFromFile(
                device.Get(),
                fileName,
                &resource,
                &shaderResourceView), 
                "Failed to load texture"
            );

            auto resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            resource->GetType(&resType);

            if(resType != D3D11_RESOURCE_DIMENSION_TEXTURE2D)
            {
                throw std::exception("resource in not texture2D");
            }

            auto& rt2 = texture2Ds.emplace_back();
            ThrowIfFailed(resource->QueryInterface(IID_PPV_ARGS(rt2.texture2D.ResetAndGetAddress())),
                "Failed to get texture 2d interface");

            auto& rsrv = shaderResourceViews.emplace_back();
            *rsrv.shaderResourceView.ResetAndGetAddress() = shaderResourceView;


            return ResourceHandle<ShaderResouceView>(shaderResourceViews.size() - 1);
        }

        auto LoadTextureCustom(wchar_t const* const fileName) -> ResourceHandle<ShaderResouceView>
        {
            struct Header final
            {
                uint16_t width = 0;
                uint16_t height = 0;
                uint16_t depth = 0;
            } header;

            auto fin = std::ifstream(fileName, std::ios::in | std::ios::binary);
            if(!fin)
            {
                throw std::exception("Failed to open resource");
            }

            fin.read(reinterpret_cast<char*>(&header), sizeof header);

            
            auto const byteSize = static_cast<size_t>(header.width) * header.height * header.depth * 8;
            auto data = std::vector<char>(byteSize);
            fin.read(data.data(), byteSize);


            if(header.depth > 1)
            {
                D3D11_SUBRESOURCE_DATA subresourceData;
                subresourceData.pSysMem = data.data();
                subresourceData.SysMemPitch = header.width * 8;
                subresourceData.SysMemSlicePitch = header.width * header.height * 8;


                D3D11_TEXTURE3D_DESC desc;
                desc.Width = header.width;
                desc.Height = header.height;
                desc.Depth = header.depth;

                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                desc.MipLevels = 1;
                desc.MiscFlags = 0;
                desc.Usage = D3D11_USAGE_IMMUTABLE;

                auto const texture = CreateTexture3D(desc, &subresourceData);
                return CreateShaderResourceView(texture);
            }

            if(header.height == 1)
            {
                D3D11_SUBRESOURCE_DATA subresourceData;
                subresourceData.pSysMem = data.data();
                subresourceData.SysMemPitch = 0;
                subresourceData.SysMemSlicePitch = 0;


                D3D11_TEXTURE1D_DESC desc;
                desc.Width = header.width;
                desc.ArraySize = 1;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                desc.MipLevels = 1;
                desc.MiscFlags = 0;
                desc.Usage = D3D11_USAGE_IMMUTABLE;
    
                auto const texture = CreateTexture1D(desc, &subresourceData);
                return CreateShaderResourceView(texture);
            }

            D3D11_SUBRESOURCE_DATA subresourceData;
            subresourceData.pSysMem = data.data();
            subresourceData.SysMemPitch = header.width * 8;
            subresourceData.SysMemSlicePitch = 0;


            D3D11_TEXTURE2D_DESC desc;
            desc.Width = header.width;
            desc.Height = header.height;
            desc.ArraySize = 1;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.MipLevels = 1;
            desc.MiscFlags = 0;
            desc.SampleDesc = { 1,0 };
            desc.Usage = D3D11_USAGE_IMMUTABLE;

            auto const texture = CreateTexture2D(desc, &subresourceData);
            return CreateShaderResourceView(texture);
        }

        auto DrawIndexed(UINT const indexCount) -> void
        {
            deviceContext->DrawIndexed(indexCount, 0, 0);
        }


        auto Draw(UINT const vertexCount) -> void
        {
            deviceContext->Draw(vertexCount, 0);
        }

    private:

        static auto ThrowIfFailed(HRESULT const hr, char const* const message) -> void
        {
            if(FAILED(hr))
            {
                throw std::exception(message);
            }
        }


        IUnknownUniquePtr<ID3D11Device> device;
        IUnknownUniquePtr<ID3D11DeviceContext> deviceContext;
        IUnknownUniquePtr<IDXGIFactory2> factory;


        ResourceHandle<Buffer> boundVertexBuffers[4] = { };
        ResourceHandle<Buffer> boundIndexBuffer;
        ResourceHandle<InputLayout> boundInputLayout;
        D3D11_PRIMITIVE_TOPOLOGY boundPrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;

        ResourceHandle<VertexShader> boundVertexShader;
        ResourceHandle<Buffer> boundVertexConstantBuffers[4] = { };

        ResourceHandle<PixelShader> boundPixelShader;
        ResourceHandle<Buffer> boundPixelBuffer;
        ResourceHandle<ShaderResouceView> boundPixelShaderResourceViews[4] = { };

        struct RTexture2D final
        {
            IUnknownUniquePtr<ID3D11Texture2D> texture2D;
        };
        std::vector<RTexture2D> texture2Ds;

        auto GetTexture2D(ResourceHandle<Texture2D> const texture2D) -> RTexture2D&
        {
            assert(texture2D.GetValue() < texture2Ds.size());
            return texture2Ds[texture2D.GetValue()];
        }


        struct RTexture1D final
        {
            IUnknownUniquePtr<ID3D11Texture1D> texture1D;
        };
        std::vector<RTexture1D> texture1Ds;


        auto GetTexture1D(ResourceHandle<Texture1D> const texture1D) -> RTexture1D&
        {
            assert(texture1D.GetValue() < texture1Ds.size());
            return texture1Ds[texture1D.GetValue()];
        }


        struct RTexture3D final
        {
            IUnknownUniquePtr<ID3D11Texture3D> texture3D;
        };
        std::vector<RTexture3D> texture3Ds;


        auto GetTexture3D(ResourceHandle<Texture3D> const texture3D) -> RTexture3D &
        {
            assert(texture3D.GetValue() < texture3Ds.size());
            return texture3Ds[texture3D.GetValue()];
        }


        struct RSwapChain final
        {
            HANDLE waitable = nullptr;
            //ResourceHandle<Texture2D> backBuffer;
            IUnknownUniquePtr<IDXGISwapChain2> swapChain;
        };
        std::vector<RSwapChain> swapChains;

        auto GetSwapChain(ResourceHandle<SwapChain> const swapChain) -> RSwapChain&
        {
            assert(swapChain.GetValue() < swapChains.size());
            return swapChains[swapChain.GetValue()];
        }

        enum class RTVResourceType
        {
            Unknown,
            SwapChain,
            Texture2D
        };

        struct RRenderTargetView final
        {
            IUnknownUniquePtr<ID3D11RenderTargetView> renderTargetView;

            RTVResourceType resourceType = RTVResourceType::Unknown;
            union
            {
                ResourceHandle<SwapChain> swapChain;
                ResourceHandle<Texture2D> texture2D;
            };

            RRenderTargetView()
                : texture2D()
            {

            }
        };
        std::vector<RRenderTargetView> renderTargetViews;

        auto GetRenderTargetView(ResourceHandle<RenderTargetView> const renderTargetView) -> RRenderTargetView&
        {
            assert(renderTargetView.GetValue() < renderTargetViews.size());
            return renderTargetViews[renderTargetView.GetValue()];
        }


        struct RDepthStencilView final
        {
            IUnknownUniquePtr<ID3D11DepthStencilView> depthStencilView;
        };
        std::vector<RDepthStencilView> depthStencilViews;

        auto GetDepthStencilView(ResourceHandle<DepthStencilView> const depthStencilView) -> RDepthStencilView&
        {
            assert(depthStencilView.GetValue() < depthStencilViews.size());
            return depthStencilViews[depthStencilView.GetValue()];
        }

        struct RBuffer final
        {
            IUnknownUniquePtr<ID3D11Buffer> buffer;
        };
        std::vector<RBuffer> buffers;

        auto GetBuffer(ResourceHandle<Buffer> const buffer) -> RBuffer&
        {
            assert(buffer.GetValue() < buffers.size());
            return buffers[buffer.GetValue()];
        }



        struct RVertexShader final
        {
            std::vector<char> bytecode;
            IUnknownUniquePtr<ID3D11VertexShader> vertexShader;
        };
        std::vector<RVertexShader> vertexShaders;

        auto GetVertexShader(ResourceHandle<VertexShader> const vertexShader) -> RVertexShader&
        {
            assert(vertexShader.GetValue() < vertexShaders.size());
            return vertexShaders[vertexShader.GetValue()];
        }



        struct RPixelShader final
        {
            std::vector<char> bytecode;
            IUnknownUniquePtr<ID3D11PixelShader> pixelShader;
        };
        std::vector<RPixelShader> pixelShaders;

        auto GetPixelShader(ResourceHandle<PixelShader> const pixelShader) -> RPixelShader&
        {
            assert(pixelShader.GetValue() < pixelShaders.size());
            return pixelShaders[pixelShader.GetValue()];
        }



        struct RInputLayout final
        {
            std::vector<D3D11_INPUT_ELEMENT_DESC> inputElemets;
            IUnknownUniquePtr<ID3D11InputLayout> inputLayout;
        };
        std::vector<RInputLayout> inputLayouts;

        auto GetInputLayout(ResourceHandle<InputLayout> inputLayout) -> RInputLayout&
        {
            assert(inputLayout.GetValue() < inputLayouts.size());
            return inputLayouts[inputLayout.GetValue()];
        }


        enum class SRVResourceType
        {
            Unknown,
            Texture1D,
            Texture2D,
            Texture3D
        };

        struct RShaderResourceView final
        {
            IUnknownUniquePtr<ID3D11ShaderResourceView> shaderResourceView;
            SRVResourceType resourceType = SRVResourceType::Unknown;
            union
            {
                ResourceHandle<Texture1D> texture1D;
                ResourceHandle<Texture2D> texture2D;
                ResourceHandle<Texture3D> texture3D;
            };

            RShaderResourceView()
                : texture2D()
            {
                
            }
        }; 
        std::vector<RShaderResourceView> shaderResourceViews;

        auto GetShaderResourceView(ResourceHandle<ShaderResouceView> shaderResourceView) -> RShaderResourceView&
        {
            assert(shaderResourceView.GetValue() < shaderResourceViews.size());
            return shaderResourceViews[shaderResourceView.GetValue()];
        }



        struct RSamplerState final
        {
            IUnknownUniquePtr<ID3D11SamplerState> samplerState;
        };
        std::vector<RSamplerState> samplerStates;

        auto GetSamplerState(ResourceHandle<SamplerState> samplerState) -> RSamplerState&
        {
            assert(samplerState.GetValue() < samplerStates.size());
            return samplerStates[samplerState.GetValue()];
        }



        struct RRasterizerState final
        {
            IUnknownUniquePtr<ID3D11RasterizerState> rasterizerState;
        };
        std::vector<RRasterizerState> rasterizerStates;

        auto GetRasterizerState(ResourceHandle<RasterizerState> rasterizerState) -> RRasterizerState&
        {
            assert(rasterizerState.GetValue() < rasterizerStates.size());
            return rasterizerStates[rasterizerState.GetValue()];
        }


        struct RBlendState final
        {
            IUnknownUniquePtr<ID3D11BlendState> blendState;
        };
        std::vector<RBlendState> blendStates;

        auto GetBlendState(ResourceHandle<BlendState> const blendState) -> RBlendState&
        {
            assert(blendState.GetValue() < blendStates.size());
            return blendStates[blendState.GetValue()];
        }
    };


    auto inline GetDXGIFormatSize(DXGI_FORMAT const format) -> int
    {
        switch(format)
        {
        case DXGI_FORMAT_R32_UINT:
            return 4;
        case DXGI_FORMAT_R16_UINT:
            return 2;
        default:
            return 0;
        }
    }

    auto LoadBytecode(std::string const& path) -> std::vector<char>
    {
        auto fin = std::fstream(path, std::ios::in | std::ios::binary | std::ios::ate);
        if(!fin)
        {
            throw std::exception("Failed to open file with given path");
        }

        size_t const size = fin.tellg();
        auto bytecode = std::vector<char>(size);
        fin.seekg(std::ios::beg);
        fin.read(bytecode.data(), size);

        return bytecode;
    }
}
