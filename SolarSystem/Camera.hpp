#pragma once
#include "ECS.hpp"
#include "Window.hpp"
#include "Transform.hpp"
#include <d3d11.h>
#include <SimpleMath.h>

namespace SolarSystem
{
    struct CameraTargetComponent
    {
        float minDistance = 1.0f;
        float maxDistance = 10.0f;
    };

    class CameraSystem final : public ECSSystem<CameraSystem, CameraTargetComponent>
    {
    public:
        auto Initialize() -> void override
        {
            windowSystem = context->GetSystem<WindowSystem>();
            worldSystem = context->GetSystem<WorldSystem>();
            
            zoomAxis = windowSystem->CreateAxis(VirtualKey::V, VirtualKey::C, 0.5f, 3.0f);
            horizontalAxis = windowSystem->CreateAxis(VirtualKey::A, VirtualKey::D, 0.5f, 3.0f);
            verticalAxis = windowSystem->CreateAxis(VirtualKey::W, VirtualKey::S, 0.5f, 3.0f);

            windowSystem->TrackKeyInput(VirtualKey::Q);
            windowSystem->TrackKeyInput(VirtualKey::E);
            
            SetPerspectiveProjection(fov, nearZ, farZ);
        }

        auto Update(float const deltaTime, float const) -> void override
        {
            if(windowSystem->IsSizeChanged())
            {
                SetPerspectiveProjection(fov, nearZ, farZ);
            }

            distance += windowSystem->GetAxis(zoomAxis) * zoomSpeed * deltaTime;
            horizontalAngle += windowSystem->GetAxis(horizontalAxis) * horizontalSpeed * deltaTime;
            verticalAngle += windowSystem->GetAxis(verticalAxis) * verticalSpeed * deltaTime;
            verticalAngle = std::clamp(verticalAngle, -DirectX::XM_PIDIV2 * 0.9f, DirectX::XM_PIDIV2 * 0.9f);

            if(windowSystem->GetKeyDown(VirtualKey::Q))
            {
                if(currentComponentFocus == 0)
                {
                    currentComponentFocus = components.GetComponentCount() - 1;
                }
                else
                {
                    currentComponentFocus--;
                }
                isLerping = true;
            }
            
            if(windowSystem->GetKeyDown(VirtualKey::E))
            {
                currentComponentFocus++;
                currentComponentFocus %= components.GetComponentCount();
                isLerping = true;
            }

            if(windowSystem->GetAxis(zoomAxis) != 0.0f)
            {
                isLerping = false;
            }

            auto const focusEntity = components.GetEntityFromComponent(currentComponentFocus);
            auto const focusPosition = worldSystem->GetComponent(focusEntity).world.Translation();


            if(windowSystem->GetKeyDown(VirtualKey::E) || windowSystem->GetKeyDown(VirtualKey::Q))
            {
                distance = components.GetComponent(focusEntity).maxDistance;
                verticalAngle = 0.3f;
            }

            if(isLerping)
            {
                auto const newDistance = components.GetComponent(focusEntity).minDistance;
                auto const t = std::clamp(2.0f * deltaTime, 0.0f, 1.0f);
                distance = Lerp(distance, newDistance, t);
            }
            
            position = DirectX::SimpleMath::Vector3(
                std::sin(horizontalAngle) * std::cos(verticalAngle) * distance,
                std::sin(verticalAngle) * distance,
                std::cos(horizontalAngle) * std::cos(verticalAngle) * distance
            ) + focusPosition;

            viewMatrix = DirectX::SimpleMath::Matrix(DirectX::XMMatrixLookAtLH(
                position,
                focusPosition,
                DirectX::SimpleMath::Vector3::Up
            ));

        }

        auto GetPosition() const -> DirectX::SimpleMath::Vector3 const&
        {
            return position;
        }

        auto SetPerspectiveProjection(float const fov, float const nearZ, float const farZ) -> void
        {
            this->fov = fov;
            this->nearZ = nearZ;
            this->farZ = farZ;


            DirectX::XMMatrixPerspectiveFovLH(fov, windowSystem->GetAspectRatio(), nearZ, farZ);
            
            //projectionMatrix = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(
            //	fov, windowSystem->GetAspectRatio(), nearZ, farZ
            //);
            //

            projectionMatrix = DirectX::SimpleMath::Matrix(DirectX::XMMatrixPerspectiveFovLH(fov, windowSystem->GetAspectRatio(), nearZ, farZ));
        }

        auto GetProjectionMatrix() const -> DirectX::SimpleMath::Matrix const&
        {
            return projectionMatrix;
        }

        auto GetViewMatrix() const -> DirectX::SimpleMath::Matrix const&
        {
            return viewMatrix;
        }

    private:
        
        WindowSystem* windowSystem = nullptr;
        WorldSystem* worldSystem = nullptr;

        float fov = DirectX::XM_PI / 3.0f;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
    
        DirectX::SimpleMath::Matrix viewMatrix;
        DirectX::SimpleMath::Matrix projectionMatrix;
        DirectX::SimpleMath::Vector3 position;

        size_t zoomAxis = (std::numeric_limits<size_t>::max)();
        float zoomSpeed = 10.0f;
        float distance = 100.0f;

        size_t horizontalAxis = (std::numeric_limits<size_t>::max)();
        float horizontalSpeed = 2.0f;
        float horizontalAngle = 0.0f;

        size_t verticalAxis = (std::numeric_limits<size_t>::max)();
        float verticalSpeed = 1.0f;
        float verticalAngle = 0.3f;

        size_t currentComponentFocus = 0;
        
        bool isLerping = true;

        static auto Lerp(float const a, float const b, float const t) -> float
        {
            return a + t * (b - a);
        }
        
    };
}
