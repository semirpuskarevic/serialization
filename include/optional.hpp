#ifndef OPTIONAL_HPP_
#define OPTIONAL_HPP_

#include <cstddef>
#include <climits>
#include <bitset>

#include <boost/optional.hpp>

namespace detail
{
    template<typename T, size_t N = CHAR_BIT * sizeof(T)>
        struct optional_field_set {
            using value_type = T;
            using bits_type = std::bitset<N>;
        };

    template<typename T, size_t N>
        struct optional_field : boost::optional<T> {
            constexpr static const size_t bit = N;

            optional_field() = default;
            optional_field(const T& v) : boost::optional<T>(v){}
        };
} /* detail */ 

#endif /* ifndef OPTIONAL_HPP_ */
