#pragma once
#include <Unknwnbase.h>



namespace SolarSystem
{

    template<typename T>
    class IUnknownUniquePtr final
    {


    public:
        IUnknownUniquePtr() = default;

        explicit IUnknownUniquePtr(IUnknown* iUnknown)
            : iUnknown(iUnknown)
        { }

        IUnknownUniquePtr(IUnknownUniquePtr const& other) = delete;

        auto operator=(IUnknownUniquePtr const& other) = delete;

        IUnknownUniquePtr(IUnknownUniquePtr&& other) noexcept
            : iUnknown(other.iUnknown)
        {
            other.iUnknown = nullptr;
        }

        auto operator=(IUnknownUniquePtr&& other) noexcept -> IUnknownUniquePtr &
        {
            iUnknown = other.iUnknown;
            other.iUnknown = nullptr;
            return *this;
        }

        ~IUnknownUniquePtr()
        {
            Reset();
        }

        auto Get() -> T*
        {
            return iUnknown;
        }

        auto Get() const -> T const*
        {
            return iUnknown;
        }

        auto GetAddress() const -> T* const*
        {
            return &iUnknown;
        }

        auto Reset() -> void
        {
            if(iUnknown)
            {
                iUnknown->Release();
                iUnknown = nullptr;
            }
        }

        auto ResetAndGetAddress() -> T**
        {
            Reset();
            return &iUnknown;
        }

        auto operator->() -> T*
        {
            return iUnknown;
        }

    private:
        T* iUnknown = nullptr;
    };
}