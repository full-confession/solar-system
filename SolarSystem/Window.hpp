#pragma once
#include "ECS.hpp"
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef CreateWindow

namespace SolarSystem
{
    enum class VirtualKey : unsigned char
    {
        Ecs = 0x1B,
        Tab = 0x09,
        Space = 0x20,
        A = 0x41,
        D = 0x44,
        S = 0x53,
        W = 0x57,
        C = 0x43,
        V = 0x56,
        Q = 0x51,
        E = 0x45,
        Z = 0x5A,
        X = 0x58,
        LShift = VK_SHIFT,
        LCtrl = VK_CONTROL
    };
    class WindowSystem final : public ECSSystem<WindowSystem>
    {
    public:
        WindowSystem(int const width, int const height, LPCWSTR const windowName)
            : width(width), height(height), windowName(windowName)
        {
            std::fill(keysDataMapping.begin(), keysDataMapping.end(), NO_KEY_MAPPING);
        }


        auto Initialize() -> void override
        {
            RegisterWindowClass();
            CreateWindow();
        }


        auto Update(float const deltaTime, float) -> void override
        {
            sizeChanged = false;

            MSG msg;
            while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            UpdateKeys();
            UpdateAxes(deltaTime);
        }


        auto Terminate() -> void override
        {
            Close();
            UnregisterWindowClass();
        }


        auto Show() -> void
        {
            ShowWindow(hWnd, SW_SHOW);
            isOpen = true;
        }


        auto IsOpen() const -> bool
        {
            return isOpen;
        }


        auto Close() -> void
        {
            if(!hWnd) return;
            
            ::DestroyWindow(hWnd);
            isOpen = false;
            hWnd = nullptr;
        }


        auto GetHandle() const -> HWND
        {
            return hWnd;
        }


        auto TrackKeyInput(VirtualKey const virtualKey) -> void
        {
            if(keysDataMapping[static_cast<std::underlying_type_t<VirtualKey>>(virtualKey)] != NO_KEY_MAPPING)
            {
                return;
            }

            auto& vki = keysData.emplace_back();
            vki.virtualKey = virtualKey;
            keysDataMapping[static_cast<std::underlying_type_t<VirtualKey>>(virtualKey)] = keysData.size() - 1;
        }


        auto GetKey(VirtualKey const virtualKey) -> bool
        {

            return GetKeyDataOrThrow(virtualKey).wasHold;
        }


        auto GetKeyDown(VirtualKey const virtualKey) -> bool
        {
            return GetKeyDataOrThrow(virtualKey).wasPressed;
        }


        auto GetKeyUp(VirtualKey const virtualKey) -> bool
        {
            return GetKeyDataOrThrow(virtualKey).wasReleased;
        }


        auto CreateAxis(VirtualKey const positive, VirtualKey const negative, float const force, float const friction) -> size_t
        {
            TrackKeyInput(positive);
            TrackKeyInput(negative);

            axes.emplace_back(AxisData{ positive, negative, force, friction });
            return axes.size() - 1;
        }


        auto GetAxis(size_t const axis) -> float
        {
            return axes[axis].value;
        }


        auto GetWidth() const -> int
        {
            return width;
        }

        auto GetHeight() const -> int
        {
            return height;
        }

        auto GetAspectRatio() const -> float
        {
            return width / static_cast<float>(height);
        }

        auto IsSizeChanged() const -> bool
        {
            return sizeChanged;
        }

    private:
        auto RegisterWindowClass() const -> void
        {
            WNDCLASSEX wndclassex = {
            sizeof wndclassex,
                CS_VREDRAW | CS_HREDRAW | CS_OWNDC,
                WindowProc,
                0,
                0,
                GetModuleHandle(nullptr),
                nullptr,
                LoadCursor(nullptr, IDC_ARROW),
                nullptr,
                nullptr,
                className,
                nullptr
            };

            if(::RegisterClassEx(&wndclassex) == 0)
            {
                throw std::exception("Failed to register window class");
            }
        }


        auto CreateWindow() -> void
        {

            auto windowRect = RECT{ 0, 0, width, height };
            AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);


            hWnd = ::CreateWindowEx(
                0,
                className,
                windowName,
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                windowRect.right - windowRect.left,
                windowRect.bottom - windowRect.top,
                nullptr,
                nullptr,
                GetModuleHandle(nullptr),
                nullptr
            );

            if(!hWnd)
            {
                throw std::exception("Failed to create window");
            }

            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        }


        auto UnregisterWindowClass() const -> void
        {
            ::UnregisterClass(className, GetModuleHandle(nullptr));
        }


        auto UpdateKeys() -> void
        {
            for(auto& keyData : keysData)
            {
                auto& messages = keysReceivedMessages[static_cast<std::underlying_type_t<VirtualKey>>(keyData.virtualKey)];

                keyData.wasHold = messages.isDown || messages.wasDown;
                keyData.wasPressed = messages.wasDown && (messages.wasUp || !keyData.isDown);
                keyData.wasReleased = messages.wasUp;
                keyData.isDown = messages.isDown;

                messages.wasDown = false;
                messages.wasUp = false;
            }
        }

        auto UpdateAxes(float const deltaTime) -> void
        {
            for(auto& axisData : axes)
            {
                auto const active = GetKey(axisData.positive) || GetKey(axisData.negative);

                if(active)
                {
                    if(GetKey(axisData.positive))
                    {
                        if(axisData.value < 0.0f)
                        {
                            axisData.value = 0.0f;
                        }
                        
                        axisData.value += axisData.force * deltaTime;
                        
                        if(axisData.value > 1.0f)
                        {
                            axisData.value = 1.0f;
                        }
                    }
                    if(GetKey(axisData.negative))
                    {
                        if(axisData.value > 0.0f)
                        {
                            axisData.value = 0.0f;
                        }
                        
                        axisData.value -= axisData.force * deltaTime;
                        
                        if(axisData.value < -1.0f)
                        {
                            axisData.value = -1.0f;
                        }
                    }
                }
                else
                {
                    if(axisData.value > 0.0f)
                    {
                        axisData.value -= axisData.friction * deltaTime;
                        if(axisData.value < 0.0f)
                        {
                            axisData.value = 0.0f;
                        }
                    }
                    else if(axisData.value < 0.0f)
                    {
                        axisData.value += axisData.friction * deltaTime;
                        if(axisData.value > 0.0f)
                        {
                            axisData.value = 0.0f;
                        }
                    }
                }
            }
        }


        struct KeyData final
        {
            VirtualKey virtualKey = VirtualKey::Space;

            bool isDown = false;
            bool wasPressed = false;
            bool wasHold = false;
            bool wasReleased = false;
        };


        struct AxisData final
        {
            VirtualKey positive;
            VirtualKey negative;

            float force = 0.0f;
            float friction = 0.0f;
            float value = 0.0f;
        };
        std::vector<AxisData> axes;


        auto GetKeyDataOrThrow(VirtualKey const virtualKey) -> KeyData&
        {
            auto const map = keysDataMapping[static_cast<std::underlying_type_t<VirtualKey>>(virtualKey)];
            if(map == NO_KEY_MAPPING)
            {
                throw std::exception("Virtual key is not registered");
            }
            return keysData[map];
        }


        auto HandleMessage(HWND const hWnd, UINT const message, WPARAM const wParam, LPARAM const lParam) -> LRESULT
        {
            switch(message)
            {
            case WM_CLOSE:
                {
                    Close();
                    return 0;
                }
            case WM_KEYDOWN:
                {
                    keysReceivedMessages[wParam].wasDown = true;
                    keysReceivedMessages[wParam].isDown = true;
                    return 0;
                }
            case WM_KEYUP:
                {
                    keysReceivedMessages[wParam].wasUp = true;
                    keysReceivedMessages[wParam].isDown = false;
                    return 0;
                }
            case WM_SIZE:
                { 
                    auto const newWidth = LOWORD(lParam);
                    auto const newHeight = HIWORD(lParam);

                    if(newHeight != height || newWidth != width)
                    {
                        sizeChanged = true;
                    }

                    width = newWidth;
                    height = newHeight;

                    return 0;
                }
            }

            return DefWindowProc(hWnd, message, wParam, lParam);
        }


        auto static CALLBACK WindowProc(HWND const hWnd, UINT const message, WPARAM const wParam, LPARAM const lParam) -> LRESULT
        {
            auto window = reinterpret_cast<WindowSystem*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            if(window)
            {
                return window->HandleMessage(hWnd, message, wParam, lParam);
            }

            return DefWindowProc(hWnd, message, wParam, lParam);

        }


        int width = 0;
        int height = 0;
        bool sizeChanged = false;

        HWND hWnd = nullptr;
        bool isOpen = false;


        LPCWSTR className = L"pepeja class";
        LPCWSTR windowName = nullptr;

        struct KeyReceivedMessages final
        {
            bool wasDown = false;
            bool wasUp = false;

            bool isDown = false;
        };

        std::array<KeyReceivedMessages, 255> keysReceivedMessages;
        std::array<size_t, 255> keysDataMapping{};
        static constexpr size_t NO_KEY_MAPPING = (std::numeric_limits<size_t>::max)();
        std::vector<KeyData> keysData;
    };
}
