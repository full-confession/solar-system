#pragma once
#include "Transform.hpp"
#include <cmath>


namespace SolarSystem
{
    struct OrbitComponent final
    {
        float radius = 1.0f;
        float period = 1.0f;
        float t = 0.0f;

        OrbitComponent() = default;
        OrbitComponent(float const radius, float const period)
            :
            radius(radius),
            period(period)
        { }
    };


    class OrbitSystem final : public ECSSystem<OrbitSystem, OrbitComponent>
    {
        TranslationSystem* translationSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            translationSystem = context->GetSystem<TranslationSystem>();
        }

        auto Update(float const, float const deltaTime) -> void override
        {
            components.Each([this, deltaTime](Entity const entity, OrbitComponent & orbit) {
                auto& translation = translationSystem->GetComponent(entity);

                auto const angle = orbit.t * DirectX::XM_2PI / orbit.period;
                translation.translation.x = std::sin(angle) * orbit.radius;
                translation.translation.z = std::cos(angle) * orbit.radius;
                orbit.t += deltaTime;
            });
        }
    };


    struct RotationalAxisComponent final
    {
        float period = 1.0f;
        float t = 0.0f;
    };

    class RotationalAxisSystem final : public ECSSystem<RotationalAxisSystem, RotationalAxisComponent>
    {
        RotationSystem* rotationSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            rotationSystem = context->GetSystem<RotationSystem>();
        }

        auto Update(float const, float const deltaTime) -> void override
        {
            components.Each([this, deltaTime](Entity const entity, RotationalAxisComponent& axis) {
                auto& rotation = rotationSystem->GetComponent(entity);

                auto const angle = axis.t * DirectX::XM_2PI / axis.period;
                rotation.rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
                    DirectX::SimpleMath::Vector3::Up,
                    angle
                );
                axis.t += deltaTime;
            });
        }
    };
}