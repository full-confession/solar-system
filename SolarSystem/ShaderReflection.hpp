#pragma once
#include "ECS.hpp"

#include <d3d11shader.h>
#include <d3dcompiler.h>
#include "IUnknownUniquePtr.hpp"
#include <string>
#include <dxgiformat.h>
#include <bitset>

#pragma comment(lib, "d3dcompiler.lib")

namespace SolarSystem
{


    struct ShaderReflection final
    {
        struct SignatureParameter final
        {
            std::string semanticName;
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
            int registerIndex = 0;
            std::bitset<8> registerMask = 0;
        };

        std::vector<SignatureParameter> inputParameters;
        std::vector<SignatureParameter> outputParameters;
    };


    class ShaderReflectionSystem final : public ECSSystem<ShaderReflectionSystem>
    {
        


    public:
        auto Reflect(void const* const bytecode, size_t const size) -> ShaderReflection
        {
            auto shaderReflection = IUnknownUniquePtr<ID3D11ShaderReflection>();
            ThrowIfFailed(D3DReflect(bytecode, size, IID_PPV_ARGS(shaderReflection.ResetAndGetAddress())),
                "Failed to reflect shader");

            D3D11_SHADER_DESC shaderDesc;
            ThrowIfFailed(shaderReflection->GetDesc(&shaderDesc),
                "Failed to get shader description");


            ShaderReflection reflection;

            for(UINT i = 0; i < shaderDesc.InputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC signatureParameterDesc;
                ThrowIfFailed(shaderReflection->GetInputParameterDesc(i, &signatureParameterDesc),
                    "Failed to get input parameters description");

                reflection.inputParameters.push_back(GetSignatureParameter(signatureParameterDesc));
            }

            for(UINT i = 0; i < shaderDesc.OutputParameters; ++i)
            {
                D3D11_SIGNATURE_PARAMETER_DESC signatureParameterDesc;
                ThrowIfFailed(shaderReflection->GetOutputParameterDesc(i, &signatureParameterDesc),
                    "Failed to get input parameters description");

                reflection.outputParameters.push_back(GetSignatureParameter(signatureParameterDesc));
            }

            return reflection;
        }
    private:


        static auto GetSignatureParameter(D3D11_SIGNATURE_PARAMETER_DESC const& desc) -> ShaderReflection::SignatureParameter
        {
            ShaderReflection::SignatureParameter parameter;
            parameter.registerIndex = desc.Register;
            parameter.registerMask = desc.Mask;
            parameter.semanticName = desc.SemanticName;
            
            if(desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
            {
                switch(parameter.registerMask.count())
                {
                case 1:
                    parameter.format = DXGI_FORMAT_R32_FLOAT;
                    break;
                case 2:
                    parameter.format = DXGI_FORMAT_R32G32_FLOAT;
                    break;
                case 3:
                    parameter.format = DXGI_FORMAT_R32G32B32_FLOAT;
                    break;
                case 4:
                    parameter.format = DXGI_FORMAT_R32G32B32A32_FLOAT;
                    break;
                }
            }

            return parameter;
        }

        static auto ThrowIfFailed(HRESULT const hr, char const* const message) -> void
        {
            if(FAILED(hr))
            {
                throw std::exception(message);
            }
        }
    };
}
