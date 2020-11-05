#pragma once

#include "SolarSystem/Transform.hpp"
#include "SolarSystem/Orbit.hpp"
#include "SolarSystem/Window.hpp"
#include "SolarSystem/Graphics.hpp"
#include "SolarSystem/Renderer.hpp"

#include <fstream>
#include <iostream>
#include <chrono>

class App final
{
public:
    App(int const width, int const height)
    {
        ecs.AddSystem<SolarSystem::WindowSystem>(width, height, L"Solar System");
        ecs.AddSystem<SolarSystem::GraphicsSystem>();
        ecs.AddSystem<SolarSystem::ShaderReflectionSystem>();

        ecs.AddSystem<SolarSystem::OrbitSystem>();
        ecs.AddSystem<SolarSystem::RotationalAxisSystem>();
        
        ecs.AddSystem<SolarSystem::WorldSystem>();
        ecs.AddSystem<SolarSystem::ScalingSystem>();
        ecs.AddSystem<SolarSystem::RotationSystem>();
        ecs.AddSystem<SolarSystem::TranslationSystem>();
        
        ecs.AddSystem<SolarSystem::ParentSystem>();
        ecs.AddSystem<SolarSystem::CameraSystem>();
        ecs.AddSystem<SolarSystem::RendererSystem>();
        ecs.Initialize();

        IntializeResources();
    }


    auto Run() -> void
    {
        auto const ws = ecs.GetSystem<SolarSystem::WindowSystem>();
        ws->TrackKeyInput(SolarSystem::VirtualKey::Z);
        ws->TrackKeyInput(SolarSystem::VirtualKey::X);
        ws->Show();


        auto deltaTime = 0.0f;

        int timeMultiplier[] = { 0, 1, 10, 1'000, 10'000, 100'000, 1'000'000, 10'000'000 };
        int currentTimeMultiplier = 1;

        while(ws->IsOpen())
        {
            auto start = std::chrono::steady_clock::now();

            if(ws->GetKeyDown(SolarSystem::VirtualKey::Z))
            {
                if(currentTimeMultiplier > 0)
                {
                    currentTimeMultiplier -= 1;
                }
            }
            if(ws->GetKeyDown(SolarSystem::VirtualKey::X))
            {
                if(currentTimeMultiplier < std::size(timeMultiplier) - 1)
                {
                    currentTimeMultiplier += 1;
                }
            }


            ecs.Update(deltaTime, deltaTime * timeMultiplier[currentTimeMultiplier] / 86'400);

            auto end = std::chrono::steady_clock::now();
            using s = std::chrono::duration<float, std::chrono::seconds::period>;
            deltaTime = std::chrono::duration_cast<s>(end - start).count();
        }
    }


private:

    auto IntializeResources() -> void
    {
        auto const rs = ecs.GetSystem<SolarSystem::RendererSystem>();
        auto const gs = ecs.GetSystem<SolarSystem::GraphicsSystem>();

        sphere = rs->CreateMesh(SolarSystem::Procedural::CreateSphere(128, 64));
        vsDefault = gs->CreateVertexShader(LoadBytecode("Shaders/VertexShader.cso"));
        psDefault = gs->CreatePixelShader(LoadBytecode("Shaders/Planet_ps.cso"));
        
        unlit = rs->CreateMaterial({
            gs->CreateVertexShader(LoadBytecode("Shaders/Unlit_vs.cso")),
            gs->CreatePixelShader(LoadBytecode("Shaders/Unlit_ps.cso")),
            { { }, { }, { }, { } }
        });


        // SUN
        auto const sun = AddSun(10.0f);

        // MERCURY
        AddPlanet(
            sun,
            60.0f,
            88.0f,
            0.7f,
            58.0f,
            2.0f,
            L"Assets/mercury_albedo.dds",
            DirectX::SimpleMath::Color(0.9f, 0.6f, 0.3f, 0.1f),
            0.56f,
            vsDefault,
            psDefault,
            { }
        );

        // VENUS
        AddPlanet(
            sun,
            75.0f,
            225.0f,
            1.0f,
            116.0f,
            177.0f,
            L"Assets/venus_albedo.dds",
            DirectX::SimpleMath::Color(1.0f, 0.9f, 0.8f, 0.1f),
            0.87f,
            vsDefault,
            psDefault,
            { }
        );

        // EARTH
        auto const earthPoint = AddAtmoPlanet(
            sun, 
            100.0f, 
            365.0f, 
            1.0f, 
            1.0f, 
            23.5f, 
            L"Assets/earth_albedo.dds", 
            DirectX::SimpleMath::Color(0.0f, 0.65f, 0.85f, 0.1f), 
            0.23f,
            6360.0f,
            100.0f,
            L"Assets/earth_transmittance.bin",
            L"Assets/earth_rayleight.bin",
            L"Assets/earth_mie.bin",
            L"Assets/earth_irradiance.bin"
        );
        AddMoon(earthPoint);

        // MARS
        auto const marsPoint = AddAtmoPlanet(
            sun,
            115.0f,
            687.0f,
            0.75f,
            1.1f,
            25.0f,
            L"Assets/mars_albedo.dds",
            DirectX::SimpleMath::Color(0.9f, 0.25f, 0.12f, 0.1f),
            0.76f,
            6360.0f,
            50.0f,
            L"Assets/mars_transmittance.bin",
            L"Assets/mars_rayleight.bin",
            L"Assets/mars_mie.bin",
            L"Assets/mars_irradiance.bin"
        );
    
        // JUPITER
        AddPlanet(
            sun,
            200.0f,
            4330.0f,
            6.0f,
            0.4f,
            3.0f,
            L"Assets/jupiter_albedo.dds",
            DirectX::SimpleMath::Color(0.67f, 0.35f, 0.11f, 0.1f),
            0.2f,
            vsDefault,
            psDefault ,
            { }
        );

        // SATURN
        auto const vsSaturn = gs->CreateVertexShader(LoadBytecode("Shaders/Saturn_vs.cso"));
        auto const psSaturn = gs->CreatePixelShader(LoadBytecode("Shaders/Saturn_ps.cso"));
        auto const rings = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTexture2D(L"Assets/saturn_ring_albedo.dds");
        auto const saturnPoint = AddPlanet(
            sun,
            300.0f,
            10800.0f,
            5.0f,
            0.41f,
            26.0f,
            L"Assets/saturn_albedo.dds",
            DirectX::SimpleMath::Color(0.47f, 0.25f, 0.35f, 0.1f),
            0.8f,
            vsSaturn,
            psSaturn,
            rings
        );
        AddSaturnRings(saturnPoint, rings);

        // URANUS
        AddPlanet(sun,
            340.0f,
            30600.0f,
            2.0f,
            0.8f,
            97.0f,
            L"Assets/uranus_albedo.dds",
            DirectX::SimpleMath::Color(0.56f, 0.81f, 0.74f, 0.1f),
            0.3f,
            vsDefault,
            psDefault,
            { }
        );

        // NEPTUNE
        AddPlanet(
            sun,
            375.0f,
            65000.0f,
            2.0f, 0.75f,
            29.0f,
            L"Assets/neptune_albedo.dds",
            DirectX::SimpleMath::Color(0.11f, 0.36f, 0.63f, 0.1f),
            0.5f,
            vsDefault,
            psDefault,
            { }
        );
    }


    SolarSystem::ResourceHandle<SolarSystem::Mesh> sphere;
    
    SolarSystem::ResourceHandle<SolarSystem::VertexShader> vsDefault;
    SolarSystem::ResourceHandle<SolarSystem::PixelShader> psDefault;

    SolarSystem::ResourceHandle<SolarSystem::Material> unlit;


    auto AddSun(float const radius) -> SolarSystem::Entity
    {
        auto const albedo = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTexture2D(L"Assets/sun_albedo.dds");
        
        auto const sunMat = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial({
            vsDefault,
            ecs.GetSystem<SolarSystem::GraphicsSystem>()->CreatePixelShader(LoadBytecode("Shaders/Sun_ps.cso")),
            { albedo, { }, { }, { } }
        });

        auto const sunOrbitPoint = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(sunOrbitPoint);

        auto const sun = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(sun);
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(sun).scaling = DirectX::SimpleMath::Vector3(radius, radius, radius);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(sun);
        ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(sun).period = -25.0f;
        ecs.GetSystem<SolarSystem::CameraSystem>()->AddComponent(sun) = { 300.0f, 500.0f };
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(sun).parent = sunOrbitPoint;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(sun, sphere, sunMat);

        return sunOrbitPoint;
    }

    auto AddPlanet(
        SolarSystem::Entity const parent,
        float const orbitRadius,
        float const orbitPeriod,
        float const radius,
        float const period,
        float const axisAngle,
        wchar_t const* const texture,
        DirectX::SimpleMath::Color const& color,
        float const t,
        SolarSystem::ResourceHandle<SolarSystem::VertexShader> const vertex,
        SolarSystem::ResourceHandle<SolarSystem::PixelShader> const pixel,
        SolarSystem::ResourceHandle<SolarSystem::ShaderResouceView> const srv)
        -> SolarSystem::Entity
    {
        auto const albedo = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTexture2D(texture);

        auto const material = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial({
            vertex,
            pixel,
            { albedo, srv, { }, { } }
            });

        auto const circle = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMesh(SolarSystem::Procedural::Circle(
            orbitRadius, 128, color
        ));

        auto const orbit = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbit);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbit);
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbit).parent = parent;

        auto const orbitPoint = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbitPoint).rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
            DirectX::SimpleMath::Vector3::Right,
            DirectX::XMConvertToRadians(axisAngle)
        );
        ecs.GetSystem<SolarSystem::OrbitSystem>()->AddComponent(orbitPoint, orbitRadius, -orbitPeriod).t = t * orbitPeriod;
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbitPoint).parent = orbit;

        auto const planet = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(planet).scaling = DirectX::SimpleMath::Vector3(radius, radius, radius);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(planet).period = -period;
        ecs.GetSystem<SolarSystem::CameraSystem>()->AddComponent(planet) = { radius * 3.0f, radius * 5.0f };
        
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(planet).parent = orbitPoint;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(planet, sphere, material);

        auto const line = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(line);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(line);
        auto& z = ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(line);
        z.period = -orbitPeriod;
        z.t = t * orbitPeriod;
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(line).parent = orbit;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(line, circle, unlit, SolarSystem::RendererSystem::BlendMode::Alpha);

        return orbitPoint;
    }


    auto AddAtmoPlanet(
        SolarSystem::Entity const parent,
        float const orbitRadius,
        float const orbitPeriod,
        float const radius,
        float const period,
        float const axisAngle,
        wchar_t const* const texture,
        DirectX::SimpleMath::Color const& color,
        float const t,
        float const planetRadius,
        float const atmoHeight,
        wchar_t const* const transmittance,
        wchar_t const* const rScattering,
        wchar_t const* const mScattering,
        wchar_t const* const irradiance
    ) -> SolarSystem::Entity
    {

        auto mat = SolarSystem::Material();

        mat.vertexShader = vsDefault;
        mat.pixelShader = ecs.GetSystem<SolarSystem::GraphicsSystem>()->CreatePixelShader(LoadBytecode("Shaders/AtmoPlanet_ps.cso"));

        mat.pixelShaderResourceViews[0] = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTexture2D(texture);
        mat.pixelShaderResourceViews[1] = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTextureCustom(transmittance);
        mat.pixelShaderResourceViews[2] = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTextureCustom(irradiance);
        mat.pixelShaderResourceViews[3] = { };


        auto const material = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial(mat);

        auto const circle = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMesh(SolarSystem::Procedural::Circle(
            orbitRadius, 128, color
        ));

        auto const orbit = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbit);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbit);
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbit).parent = parent;

        auto const orbitPoint = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbitPoint).rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
            DirectX::SimpleMath::Vector3::Right,
            DirectX::XMConvertToRadians(axisAngle)
        );
        ecs.GetSystem<SolarSystem::OrbitSystem>()->AddComponent(orbitPoint, orbitRadius, -orbitPeriod).t = t * orbitPeriod;
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbitPoint).parent = orbit;

        auto const planet = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(planet).scaling = DirectX::SimpleMath::Vector3(radius, radius, radius);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(planet).period = -period;
        ecs.GetSystem<SolarSystem::CameraSystem>()->AddComponent(planet) = { radius * 3.0f, radius * 5.0f };

        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(planet).parent = orbitPoint;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(planet, sphere, material);

        auto const line = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(line);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(line);
        auto& z = ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(line);
        z.period = -orbitPeriod;
        z.t = t * orbitPeriod;
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(line).parent = orbit;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(line, circle, unlit, SolarSystem::RendererSystem::BlendMode::Alpha);



        
        auto const raySRV = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTextureCustom(rScattering);
        auto const mieSRV = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTextureCustom(mScattering);


        auto const scMaterial = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial({
            vsDefault,
            ecs.GetSystem<SolarSystem::GraphicsSystem>()->CreatePixelShader(LoadBytecode("Shaders/Atmos_ps.cso")),
            { raySRV, mieSRV, { }, { } }
        });

        auto const atmo = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(atmo);

        auto const atmoScale = (planetRadius + atmoHeight) / planetRadius;
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(atmo).scaling = DirectX::SimpleMath::Vector3(atmoScale, atmoScale, atmoScale);

        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(atmo).parent = planet;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(atmo, sphere, scMaterial, SolarSystem::RendererSystem::BlendMode::Add);
        
        return orbitPoint;
    }

    auto AddSaturnRings(
        SolarSystem::Entity const parent,
        SolarSystem::ResourceHandle<SolarSystem::ShaderResouceView> const srv
    ) -> void
    {
        
        auto const material = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial({
            ecs.GetSystem<SolarSystem::GraphicsSystem>()->CreateVertexShader(LoadBytecode("Shaders/Rings_vs.cso")),
            ecs.GetSystem<SolarSystem::GraphicsSystem>()->CreatePixelShader(LoadBytecode("Shaders/Rings_ps.cso")),
            { srv, { }, { }, { } }
        });

        auto const ring = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMesh(SolarSystem::Procedural::CreateRing(
            128, 1.2f, 2.5f
        ));

        auto const rings = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(rings);
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(rings).scaling = DirectX::SimpleMath::Vector3(5.0f, 5.0f, 5.0f);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(rings);
        ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(rings).period = -0.35f;

        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(rings).parent = parent;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(rings, ring, material, SolarSystem::RendererSystem::BlendMode::Alpha);

    }

    auto AddMoon(SolarSystem::Entity const parent) -> void
    {
        auto const albedo = ecs.GetSystem<SolarSystem::GraphicsSystem>()->LoadTexture2D(L"Assets/moon_albedo.dds");

        auto const material = ecs.GetSystem<SolarSystem::RendererSystem>()->CreateMaterial({
            vsDefault,
            psDefault,
            { albedo, { }, { }, { } }
            });

        auto const orbit = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbit);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbit).rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
            DirectX::SimpleMath::Vector3::Right,
            DirectX::XMConvertToRadians(-30.0f)
        );;
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbit).parent = parent;

        auto const orbitPoint = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(orbitPoint);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(orbitPoint).rotation = DirectX::SimpleMath::Quaternion::CreateFromAxisAngle(
            DirectX::SimpleMath::Vector3::Right,
            DirectX::XMConvertToRadians(6.0f)
        );
        ecs.GetSystem<SolarSystem::OrbitSystem>()->AddComponent(orbitPoint, 3.0f, -27.0f);
        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(orbitPoint).parent = orbit;

        auto const planet = ecs.CreateEntity();
        ecs.GetSystem<SolarSystem::WorldSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::TranslationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::ScalingSystem>()->AddComponent(planet).scaling = DirectX::SimpleMath::Vector3(0.1f, 0.1f, 0.1f);
        ecs.GetSystem<SolarSystem::RotationSystem>()->AddComponent(planet);
        ecs.GetSystem<SolarSystem::RotationalAxisSystem>()->AddComponent(planet).period = -27.0f;

        ecs.GetSystem<SolarSystem::ParentSystem>()->AddComponent(planet).parent = orbitPoint;
        ecs.GetSystem<SolarSystem::RendererSystem>()->AddComponent(planet, sphere, material);


    }

    static auto LoadBytecode(std::string const& path) -> std::vector<char>
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


    SolarSystem::ECS ecs;
};
