#pragma once
#include <vector>
#include <memory>
#include <cassert>
#include <algorithm>

namespace SolarSystem
{
    struct Entity final
    {
        size_t id;
    };


    class ECS;

    class ECSContext final
    {
    public:
        explicit ECSContext(ECS& ecs): ecs(ecs)
        { }

        template <typename T>
        auto GetSystem()->T*;

    private:
        ECS& ecs;
    };

    class SystemBase
    {
    public:
        using system_index_type = size_t;

        explicit SystemBase(system_index_type const systemIndex): systemIndex(systemIndex)
        { }

        virtual ~SystemBase() = default;

        SystemBase(SystemBase const&) = delete;
        SystemBase(SystemBase&&) = delete;

        auto operator=(SystemBase const&)->SystemBase & = delete;
        auto operator=(SystemBase&&)->SystemBase & = delete;

        auto SetContext(ECSContext* context) -> void
        {
            this->context = context;
        }

        virtual auto Initialize() -> void
        { }

        virtual auto Update(float deltaTime, float deltaTime2) -> void
        { }

        virtual auto Terminate() -> void
        { }


        auto GetSystemIndex() const -> system_index_type
        {
            return systemIndex;
        }

    private:
        system_index_type systemIndex;
        static inline system_index_type nextSystemIndex = 0;

    protected:
        ECSContext* context = nullptr;

        static auto GetNextSystemIndex() -> system_index_type
        {
            return nextSystemIndex++;
        }
    };


    template <typename System, typename... Component>
    class ECSSystem;


    template <typename System>
    class ECSSystem<System> : public SystemBase
    {
    public:
        explicit ECSSystem(): SystemBase(systemIndex)
        { }

        static auto GetSystemIndex() -> system_index_type
        {
            return systemIndex;
        }

    private:
        static inline system_index_type systemIndex = GetNextSystemIndex();
    };


    template <typename Component>
    class ComponentHolder
    {
    public:
        ComponentHolder(): entityComponents(), entityToComponent(256, NO_MAPPING)
        { }

        template <typename... CtorArgs>
        auto AddComponent(Entity entity, CtorArgs ... args) -> Component &
        {
            assert(entityToComponent[entity.id] == NO_MAPPING);

            auto & component = entityComponents.emplace_back(entity, std::forward<CtorArgs>(args)...).component;
            entityToComponent[entity.id] = entityComponents.size() - 1;

            return component;
        }

        auto GetComponent(Entity entity) -> Component &
        {
            assert(entityToComponent[entity.id] != NO_MAPPING);
            return entityComponents[entityToComponent[entity.id]].component;
        }

        template <typename Func>
        auto Each(Func && function) -> void
        {
            for(auto& entityComponent : entityComponents)
            {
                function(entityComponent.entity, entityComponent.component);
            }
        }

        auto operator[](size_t index) -> Component&
        {
            return entityComponents[index].component;
        }

        auto GetComponentCount() -> size_t
        {
            return entityComponents.size();
        }

        auto GetEntityFromComponent(size_t index) -> Entity
        {
            return entityComponents[index].entity;
        }

        auto GetComponentIndex(Entity entity) -> size_t
        {
            assert(entityToComponent[entity.id] != NO_MAPPING);
            return entityToComponent[entity.id];
        }

        auto SwapComponents(size_t a, size_t b) -> void
        {
            std::swap(entityComponents[a], entityComponents[b]);
        }

    private:
        struct EntityComponent final
        {
            Entity entity;
            Component component;

            template <typename ... CtorArgs>
            explicit EntityComponent(Entity const entity, CtorArgs ... args)
                : entity(entity), component(std::forward<CtorArgs>(args)...)
            { }
        };

        std::vector<EntityComponent> entityComponents;
        using size_type = decltype(entityComponents)::template size_type;

        static constexpr size_type NO_MAPPING = (std::numeric_limits<size_type>::max)();
        std::vector<size_type> entityToComponent;
    };

    template <typename System, typename Component>
    class ECSSystem<System, Component> : public ECSSystem<System>
    {
    public:
        template <typename... CtorArgs>
        auto AddComponent(Entity entity, CtorArgs ... args) -> Component &
        {
            return components.AddComponent(entity, std::forward<CtorArgs>(args)...);
        }

        auto GetComponent(Entity entity) -> Component &
        {
            return components.GetComponent(entity);
        }

        template <typename Func>
        auto Each(Func&& function) -> void
        {
            components.Each(std::forward<Func>(function));
        }

    protected:
        ComponentHolder<Component> components;
    };


    class ECS final
    {
    public:
        template <typename T, typename ... CtorArgs>
        auto AddSystem(CtorArgs ... args) -> T*
        {
            auto systemIndex = T::GetSystemIndex();
            if(systems.size() < systemIndex + 1)
            {
                systems.resize(systemIndex + 1);
            }
            systems[systemIndex] = std::make_unique<T>(std::forward<CtorArgs>(args)...);
            systems[systemIndex]->SetContext(&context);

            systemsUpdateOrder.push_back(systemIndex);
            return reinterpret_cast<T*>(systems[systemIndex].get());
        }


        template <typename T>
        auto GetSystem() -> T*
        {
            auto systemIndex = T::GetSystemIndex();
            assert(systemIndex < systems.size());

            auto system = systems[systemIndex].get();
            assert(system->GetSystemIndex() == systemIndex);

            return reinterpret_cast<T*>(system);
        }


        auto Initialize() -> void
        {
            for(auto systemIndex : systemsUpdateOrder)
            {
                systems[systemIndex]->Initialize();
            }
        }

        auto Update(float const deltaTime, float const deltaTime2) -> void
        {
            for(auto systemIndex : systemsUpdateOrder)
            {
                systems[systemIndex]->Update(deltaTime, deltaTime2);
            }
        }

        auto Terminate() -> void
        {
            for(auto i = systemsUpdateOrder.rbegin(); i != systemsUpdateOrder.rend(); ++i)
            {
                auto const systemIndex = *i;
                systems[systemIndex]->Terminate();
            }
        }


        auto CreateEntity() -> Entity
        {
            return Entity{ nextEntityIndex++ };
        }

    private:
        std::vector<std::unique_ptr<SystemBase>> systems;
        using size_type = decltype(systems)::size_type;
        std::vector<size_type> systemsUpdateOrder;
        ECSContext context{ *this };
        size_t nextEntityIndex = 0;
    };


    template <typename T>
    auto ECSContext::GetSystem() -> T*
    {
        return ecs.GetSystem<T>();
    }
}
