#pragma once
#include <limits>

namespace SolarSystem
{
    template<typename T>
    class ResourceHandle final
    {
    public:
        ResourceHandle():
            value((std::numeric_limits<size_t>::max)())
        { }

        explicit ResourceHandle(size_t const value):
            value(value)
        { }

        auto GetValue() const -> size_t
        {
            return value;
        }

        auto IsNull() const -> bool
        {
            return value == (std::numeric_limits<size_t>::max)();
        }
    private:
        size_t value;
    };

    template<typename T>
    auto operator==(ResourceHandle<T> left, ResourceHandle<T> right) -> bool
    {
        return left.GetValue() == right.GetValue();
    }

    template<typename T>
    auto operator!=(ResourceHandle<T> left, ResourceHandle<T> right) -> bool
    {
        return !(left == right);
    }
}
