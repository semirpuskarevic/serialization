#ifndef WRITER_HPP_
#define WRITER_HPP_
#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <chrono>
#include <exception>
#include <limits>
#include <string>
#include <type_traits>
#include <unordered_map>
#include "network.hpp"
#include "optional.hpp"

namespace asio = boost::asio;
namespace fusion = boost::fusion;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
template <typename T>
void write_integral_value(T const &val, asio::mutable_buffer &buf) {
    if (sizeof(T) == 1)
        *asio::buffer_cast<T *>(buf) = network::hton(val);
    else if (sizeof(T) == 2)
        *static_cast<emscripten_align1_short *>(
            asio::buffer_cast<emscripten_align1_short *>(buf)) =
            network::hton(val);
    else if (sizeof(T) == 4)
        *static_cast<emscripten_align1_int *>(
            asio::buffer_cast<emscripten_align1_int *>(buf)) =
            network::hton(val);
    else if (sizeof(T) == 8) {
        *static_cast<emscripten_align1_int64 *>(
            asio::buffer_cast<emscripten_align1_int64 *>(buf)) =
            network::hton(val);
    }
}

#else
template <typename T>
void write_integral_value(T const &val, asio::mutable_buffer &buf) {
    *asio::buffer_cast<T *>(buf) = network::hton(val);
}
#endif

namespace detail {

template <typename T>
struct lazy;

template <typename OptType>
struct writer_base {
    using opt_field_set_value_type =
        typename optional_field_set<OptType>::value_type;

    mutable asio::mutable_buffer buf_;
    mutable typename optional_field_set<OptType>::bits_type opt_mask_;
    mutable opt_field_set_value_type *opt_in_buf_ = nullptr;

    explicit writer_base(asio::mutable_buffer buf) : buf_{std::move(buf)} {}

    template <typename T>
    auto operator()(T const &val) const ->
        typename std::enable_if<std::is_integral<T>::value>::type {
        size_t bs = sizeof(T);
        // NOTE:: Check if this is too pesimistic. Maybe can be optimized.
        if (bs > asio::buffer_size(buf_)) {
            throw std::overflow_error("No space in buffer");
        }
        write_integral_value(val, buf_);
        buf_ = buf_ + sizeof(T);
    }

    template <typename T>
    auto operator()(T val) const ->
        typename std::enable_if<std::numeric_limits<typename std::enable_if<
            std::is_floating_point<T>::value, T>::type>::is_iec559>::type {
        auto encoded = network::pack754(val);
        (*this)(encoded);
    }

    template <typename T>
    auto operator()(T const &val) const ->
        typename std::enable_if<std::is_enum<T>::value>::type {
        using utype = typename std::underlying_type<T>::type;
        (*this)(static_cast<utype>(val));
    }

    template <typename T, T v>
    void operator()(std::integral_constant<T, v>) const {
        using int_const_t = std::integral_constant<T, v>;
        (*this)(int_const_t::value);
    }

    void operator()(std::string const &val) const {
        size_t bs = val.length() + sizeof(uint16_t);

        if (bs > asio::buffer_size(buf_)) {
            throw std::overflow_error("No space in buffer");
        }

        (*this)(static_cast<uint16_t>(val.length()));
        asio::buffer_copy(buf_, asio::buffer(val));
        buf_ = buf_ + val.length();
    }

    template <typename T>
    void operator()(std::vector<T> const &vals) const {
        (*this)(static_cast<uint16_t>(vals.size()));
        for (const auto &v : vals) (*this)(v);
    }

    template <typename K, typename V>
    void operator()(std::unordered_map<K, V> const &vals) const {
        (*this)(static_cast<uint16_t>(vals.size()));
        for (const auto &kv : vals) {
            (*this)(kv);
        }
    }

    template <typename K, typename V>
    void operator()(std::pair<K, V> const &val) const {
        (*this)(val.first);
        (*this)(val.second);
    }

    void operator()(
        std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::microseconds> const &val) const {
        int64_t dateTime = val.time_since_epoch().count();
        (*this)(dateTime);
    }

    void operator()(optional_field_set<OptType>) const {
        opt_mask_.reset();
        constexpr size_t bs = sizeof(opt_field_set_value_type);
        if (bs > asio::buffer_size(buf_)) {
            throw std::overflow_error("No space in buffer");
        }

        opt_in_buf_ = asio::buffer_cast<opt_field_set_value_type *>(buf_);
        *opt_in_buf_ = network::hton(
            static_cast<opt_field_set_value_type>(opt_mask_.to_ulong()));
        buf_ = buf_ + sizeof(opt_field_set_value_type);
    }

    template <typename T, size_t N>
    void operator()(optional_field<T, N> const &val) const {
        if (!opt_in_buf_) throw std::domain_error("Bad message");
        if (val) {
            opt_mask_.set(N);
            *opt_in_buf_ = network::hton(
                static_cast<opt_field_set_value_type>(opt_mask_.to_ulong()));
            (*this)(*val);
        }
    }

    template <typename T>
    auto operator()(T const &val) const ->
        typename std::enable_if<fusion::traits::is_sequence<T>::value>::type {
        fusion::for_each(val, *this);
    }

    template <typename T>
    void operator()(lazy<T> const &val) const;
};

using writer = writer_base<uint16_t>;

template <typename T>
asio::mutable_buffer write(asio::mutable_buffer buf, T const &val) {
    writer w{std::move(buf)};
    w(val);
    return w.buf_;
}

} /* detail */

#include "lazy.hpp"
template <typename OptType>
template <typename T>
void detail::writer_base<OptType>::operator()(lazy<T> const &val) const {
    (*this)(val.get());
}

#endif /* ifndef WRITER_HPP_ */
