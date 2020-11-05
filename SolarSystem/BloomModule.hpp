#pragma once
#include "Graphics.hpp"



namespace SolarSystem
{
    class BloomModule final
    {
    public:
        auto Initialize(GraphicsSystem* graphicsSystem) -> void
        {
            this->graphicsSystem = graphicsSystem;
        }
        
        auto SetSource(ResourceHandle<ShaderResouceView> const source) -> void
        {
            this->source = source;		
            sourceDimensions = graphicsSystem->GetResourceDimensions(source);
            auto const maxMipLevels = FloorLog2((std::min)(sourceDimensions.width, sourceDimensions.height));

            mipLevels = maxMipLevels;
            
            D3D11_TEXTURE2D_DESC texDesc;
            texDesc.Width = sourceDimensions.width / 2;
            texDesc.Height = sourceDimensions.height / 2;
            texDesc.ArraySize = 1;
            texDesc.MipLevels = mipLevels;
            texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
            texDesc.SampleDesc = { 1, 0 };
            texDesc.Usage = D3D11_USAGE_DEFAULT;
            texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
            texDesc.CPUAccessFlags = 0;
            texDesc.MiscFlags = 0;

            bloomMin = graphicsSystem->CreateTexture2D(texDesc);
            
            texDesc.MipLevels = mipLevels - 1;
            bloomMag = graphicsSystem->CreateTexture2D(texDesc);


            D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
            rtvDesc.Format = DXGI_FORMAT_UNKNOWN;
            rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;
            
            for(auto i = 0; i < maxMipLevels; ++i)
            {
                rtvDesc.Texture2D.MipSlice = i;			
                bloomMinRTVs.push_back(graphicsSystem->CreateRenderTargetView(bloomMin, rtvDesc));

                srvDesc.Texture2D.MostDetailedMip = i;
                bloomMinSRVs.push_back(graphicsSystem->CreateShaderResourceView(bloomMin, &srvDesc));
            }

            for(auto  i = 0; i < maxMipLevels - 1; ++i)
            {
                rtvDesc.Texture2D.MipSlice = i;
                bloomMagRTVs.push_back(graphicsSystem->CreateRenderTargetView(bloomMag, rtvDesc));

                srvDesc.Texture2D.MostDetailedMip = i;
                bloomMagSRVs.push_back(graphicsSystem->CreateShaderResourceView(bloomMag, &srvDesc));
            }

            D3D11_BUFFER_DESC bDesc;
            bDesc.ByteWidth = sizeof(PixelCBuffer);
            bDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            bDesc.MiscFlags = 0;
            bDesc.StructureByteStride = 0;
            bDesc.Usage = D3D11_USAGE_DYNAMIC;

            pixelCBuffer = graphicsSystem->CreateBuffer(bDesc, nullptr);

            bloomDownAndFilter = graphicsSystem->CreatePixelShader(LoadBytecode("Shaders/BloomFilterPass_ps.cso"));
            bloomDown = graphicsSystem->CreatePixelShader(LoadBytecode("Shaders/BloomMinPass_ps.cso"));
            bloomUp = graphicsSystem->CreatePixelShader(LoadBytecode("Shaders/BloomMagPass_ps.cso"));
        }

        auto GetDestination() const -> ResourceHandle<ShaderResouceView>
        {
            return bloomMagSRVs[0];
        }

        auto Draw() -> void
        {
            graphicsSystem->SetRenderTargets(bloomMinRTVs[0]);
            graphicsSystem->BindPixelShaderResourceViews({ source, { }, { }, { } });

    
            UpdateCBuffer(0);
            
            graphicsSystem->BindPixelShader(bloomDownAndFilter);
            graphicsSystem->BindPixelConstantBuffer(pixelCBuffer);
            graphicsSystem->Draw(3);

            graphicsSystem->BindPixelShader(bloomDown);
            for(auto i = 1; i < bloomMinRTVs.size(); ++i)
            {
                graphicsSystem->SetRenderTargets(bloomMinRTVs[i]);
                graphicsSystem->BindPixelShaderResourceViews({ bloomMinSRVs[i - 1], { }, { }, { } });
                UpdateCBuffer(i);
                graphicsSystem->Draw(3);
            }

            
            UpdateCBuffer(mipLevels - 2, 0.64f);
            graphicsSystem->SetRenderTargets(bloomMagRTVs[mipLevels - 2]);
            graphicsSystem->BindPixelShaderResourceViews({ 
                bloomMinSRVs[mipLevels - 1],
                bloomMinSRVs[mipLevels - 2],
                { },
                { }
            });
            graphicsSystem->BindPixelShader(bloomUp);
            graphicsSystem->Draw(3);
            

            for(auto i = mipLevels - 3; i >= 0; --i)
            {
                graphicsSystem->SetRenderTargets(bloomMagRTVs[i]);
                
                graphicsSystem->BindPixelShaderResourceViews({ 
                    bloomMagSRVs[i + 1],
                    bloomMinSRVs[i],
                    { },
                    { }
                });
                UpdateCBuffer(i);
                graphicsSystem->Draw(3);
            }

            graphicsSystem->SetRenderTargets({ });
        }
        

    private:
        GraphicsSystem* graphicsSystem = nullptr;

        ResourceHandle<ShaderResouceView> source;
        GraphicsSystem::ResourceDimensions sourceDimensions;
        int mipLevels = 0;

        struct PixelCBuffer final
        {
            float textureSize[2];
            float texelSize[2];
            float threshold[4];
            float sampleSize[4];
        };
        ResourceHandle<Buffer> pixelCBuffer;

        float threshold = 2.5f;
        float thresholdKnee = 0.00001f;
        
        ResourceHandle<Texture2D> bloomMin;
        ResourceHandle<Texture2D> bloomMag;

        std::vector<ResourceHandle<ShaderResouceView>> bloomMinSRVs;
        std::vector<ResourceHandle<RenderTargetView>> bloomMinRTVs;

        std::vector<ResourceHandle<ShaderResouceView>> bloomMagSRVs;
        std::vector<ResourceHandle<RenderTargetView>> bloomMagRTVs;

        ResourceHandle<PixelShader> bloomDownAndFilter;
        ResourceHandle<PixelShader> bloomDown;
        ResourceHandle<PixelShader> bloomUp;

        static auto FloorLog2(int value) -> int
        {
            auto val = 0;
            while(value >>= 1)
            {
                val++;
            }
            return val;
        }

        auto UpdateCBuffer(int const mipLevel, float const sampleSize = 0.0f) const -> void
        {
            PixelCBuffer cBuffer;
            
            cBuffer.textureSize[0] = static_cast<float>(sourceDimensions.width / (1 << (mipLevel + 1)));
            cBuffer.textureSize[1] = static_cast<float>(sourceDimensions.height / (1 << (mipLevel + 1)));
            cBuffer.texelSize[0] = 1.0f / cBuffer.textureSize[0];
            cBuffer.texelSize[1] = 1.0f / cBuffer.textureSize[1];
            cBuffer.threshold[0] = threshold;
            cBuffer.threshold[1] = threshold - thresholdKnee;
            cBuffer.threshold[2] = thresholdKnee * 2.0f;
            cBuffer.threshold[3] = 0.25f / thresholdKnee;
            cBuffer.sampleSize[0] = sampleSize;
            
            graphicsSystem->WriteBuffer(pixelCBuffer, &cBuffer, sizeof cBuffer);
        }
    };

}