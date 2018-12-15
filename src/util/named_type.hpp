#pragma once

#include <type_traits>

// Enable empty base class optimization with multiple inheritance on Visual Studio.
#if defined(_MSC_VER) && _MSC_VER >= 1910
#  define FLUENT_EBCO __declspec(empty_bases)
#else
#  define FLUENT_EBCO
#endif

namespace fluent
{

template <typename T, typename Parameter, template<typename> class... Skills>
class FLUENT_EBCO NamedType : public Skills<NamedType<T, Parameter, Skills...>>...
{
public:
    using UnderlyingType = T;
    
    explicit constexpr NamedType(T const& value) : m_value(value) {}
    
    constexpr T& Get() { return m_value; }
    constexpr T const& Get() const {return m_value; }

	operator T&() { return m_value; }
    
    struct argument
    {
        template<typename U>
        NamedType operator=(U&& value) const
        {
            return NamedType(std::forward<U>(value));
        }
        argument() = default;
        argument(argument const&) = delete;
        argument(argument &&) = delete;
        argument& operator=(argument const&) = delete;
        argument& operator=(argument &&) = delete;
    };
    
private:
    T m_value;
};

} /* fluent */
