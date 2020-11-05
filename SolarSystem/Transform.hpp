#pragma once
#include "ECS.hpp"

#include <d3d11.h>
#include <SimpleMath.h>

namespace SolarSystem
{
    struct TransformComponent final
    {
        DirectX::SimpleMath::Vector3 position = DirectX::SimpleMath::Vector3::Zero;
        DirectX::SimpleMath::Quaternion rotation = DirectX::SimpleMath::Quaternion::Identity;
        DirectX::SimpleMath::Vector3 scale = DirectX::SimpleMath::Vector3::One;

        TransformComponent() = default;

        explicit TransformComponent(
            DirectX::SimpleMath::Vector3 const& position
        ): position(position)
        { }

        TransformComponent(
            DirectX::SimpleMath::Vector3 const& position,
            DirectX::SimpleMath::Quaternion const& rotation
        ): position(position), rotation(rotation)
        { }

        TransformComponent(
            DirectX::SimpleMath::Vector3 const& position,
            DirectX::SimpleMath::Quaternion const& rotation,
            DirectX::SimpleMath::Vector3 const& scale
        ): position(position), rotation(rotation), scale(scale)
        { }
    };

    class TransformSystem final : public ECSSystem<TransformSystem, TransformComponent>
    {

    };


    struct TranslationMatrixComponent final
    {
        DirectX::SimpleMath::Matrix translation;
    };

    class TranslationMatrixFromTransformSystem final : public ECSSystem<TranslationMatrixFromTransformSystem, TranslationMatrixComponent>
    {
        TransformSystem* transformSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            transformSystem = context->GetSystem<TransformSystem>();
        }
        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, TranslationMatrixComponent & translationMatrix) {
                translationMatrix.translation = DirectX::SimpleMath::Matrix::CreateTranslation(transformSystem->GetComponent(entity).position);
                });
        }
    };


    struct RotationMatrixComponent final
    {
        DirectX::SimpleMath::Matrix rotation;
    };

    class RotationMatrixFromTransformSystem final : public ECSSystem<RotationMatrixFromTransformSystem, RotationMatrixComponent>
    {
        TransformSystem* transformSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            transformSystem = context->GetSystem<TransformSystem>();
        }
        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, RotationMatrixComponent & rotationMatrix) {
                rotationMatrix.rotation = DirectX::SimpleMath::Matrix::CreateFromQuaternion(transformSystem->GetComponent(entity).rotation);
                });
        }
    };


    struct ScalingMatrixComponent final
    {
        DirectX::SimpleMath::Matrix scaling;
    };

    class ScalingMatrixFromTransformSystem final : public ECSSystem<ScalingMatrixFromTransformSystem, ScalingMatrixComponent>
    {
        TransformSystem* transformSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            transformSystem = context->GetSystem<TransformSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, ScalingMatrixComponent & scalingMatrix) {
                scalingMatrix.scaling = DirectX::SimpleMath::Matrix::CreateScale(transformSystem->GetComponent(entity).scale);
            });
        }
    };


    struct WorldMatrixComponent final
    {
        DirectX::SimpleMath::Matrix world;
    };

    class WorldMatrixFromTranslationRotationScalingSystem final : public ECSSystem<WorldMatrixFromTranslationRotationScalingSystem, WorldMatrixComponent>
    {
        TranslationMatrixFromTransformSystem* translationSystem = nullptr;
        RotationMatrixFromTransformSystem* rotationSystem = nullptr;
        ScalingMatrixFromTransformSystem* scalingSystem = nullptr;


    public:
        auto Initialize() -> void override
        {
            translationSystem = context->GetSystem<TranslationMatrixFromTransformSystem>();
            rotationSystem = context->GetSystem<RotationMatrixFromTransformSystem>();
            scalingSystem = context->GetSystem<ScalingMatrixFromTransformSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, WorldMatrixComponent & worldMatrix) {
                auto& t = translationSystem->GetComponent(entity);
                auto& r = rotationSystem->GetComponent(entity);
                auto& s = scalingSystem->GetComponent(entity);

                worldMatrix.world = s.scaling * r.rotation * t.translation;
                });
        }
    };


    class WorldSystem final : public ECSSystem<WorldSystem, WorldMatrixComponent>
    {
        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, WorldMatrixComponent& worldComponent) {

                worldComponent.world = DirectX::SimpleMath::Matrix::Identity;
            });
        }
    };

    struct TranslationComponent final
    {
        DirectX::SimpleMath::Vector3 translation;
    };

    class TranslationSystem final : public ECSSystem<TranslationSystem, TranslationComponent>
    {
        WorldSystem* worldMatrixSystem = nullptr;
        
    public:
        auto Initialize() -> void override
        {
            worldMatrixSystem = context->GetSystem<WorldSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, TranslationComponent& translationComponent) {
                
                auto& worldMatrix = worldMatrixSystem->GetComponent(entity).world;
                worldMatrix *= DirectX::SimpleMath::Matrix::CreateTranslation(translationComponent.translation);
            });
        }
    };

    struct RotationComponent final
    {
        DirectX::SimpleMath::Quaternion rotation = DirectX::SimpleMath::Quaternion::Identity;
    };

    class RotationSystem final : public ECSSystem<RotationSystem, RotationComponent>
    {
        WorldSystem* worldMatrixSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            worldMatrixSystem = context->GetSystem<WorldSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, RotationComponent& rotationComponent) {

                auto& worldMatrix = worldMatrixSystem->GetComponent(entity).world;

                worldMatrix *= DirectX::SimpleMath::Matrix::CreateFromQuaternion(rotationComponent.rotation);
            });
        }
    };

    struct ScalingComponent final
    {
        DirectX::SimpleMath::Vector3 scaling = DirectX::SimpleMath::Vector3::One;
    };

    class ScalingSystem final : public ECSSystem<ScalingSystem, ScalingComponent>
    {
        WorldSystem* worldMatrixSystem = nullptr;

    public:
        auto Initialize() -> void override
        {
            worldMatrixSystem = context->GetSystem<WorldSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, ScalingComponent& scalingComponent) {

                auto& worldMatrix = worldMatrixSystem->GetComponent(entity).world;
                worldMatrix *= DirectX::SimpleMath::Matrix::CreateScale(scalingComponent.scaling);
                });
        }
    };


    struct ParentComponent final
    {
        Entity parent;
    };

    class ParentSystem final : public ECSSystem<ParentSystem, ParentComponent>
    {
        WorldSystem* worldSystem = nullptr;
        
    public:
        auto Initialize() -> void override
        {
            worldSystem = context->GetSystem<WorldSystem>();
        }

        auto Update(float, float) -> void override
        {
            components.Each([this](Entity const entity, ParentComponent const& component) {
                auto& parent = worldSystem->GetComponent(component.parent);
                auto& child = worldSystem->GetComponent(entity);

                child.world = child.world * parent.world;
            });
        }
    };
}
