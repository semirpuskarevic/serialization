#ifndef READER_HPP_
#define READER_HPP_
#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <chrono>
#include <exception>
#include <unordered_map>
#include "network.hpp"
#include "optional.hpp"

namespace asio = boost::asio;
namespace fusion = boost::fusion;

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

template <typename T>
T read_integral_value(asio::const_buffer &buf) {
    T val{0};
    if (sizeof(T) == 1)
        val = network::ntoh(*asio::buffer_cast<const T *>(buf));
    else if (sizeof(T) == 2)
        val = network::ntoh(*static_cast<const emscripten_align1_short *>(
            asio::buffer_cast<const emscripten_align1_short *>(buf)));
    else if (sizeof(T) == 4)
        val = network::ntoh(*static_cast<const emscripten_align1_int *>(
            asio::buffer_cast<const emscripten_align1_int *>(buf)));
    else if (sizeof(T) == 8) {
        val = network::ntoh(*static_cast<const emscripten_align1_int64 *>(
            asio::buffer_cast<const emscripten_align1_int64 *>(buf)));
    }
    return val;
}

#else

template <typename T>
T read_integral_value(asio::const_buffer &buf) {
    T val = network::ntoh(*asio::buffer_cast<const T *>(buf));
    return val;
}
#endif

namespace detail {

template <typename OptType>
struct reader_base {
    mutable asio::const_buffer buf_;
    mutable boost::optional<typename optional_field_set<OptType>::bits_type>
        optMask_;

    reader_base(asio::const_buffer buf) : buf_{std::move(buf)} {}

    template <typename T>
    auto operator()(T &val) const ->
        typename std::enable_if<std::is_integral<T>::value>::type {
        val = read_integral_value<T>(buf_);
        buf_ = buf_ + sizeof(T);
    }

    auto operator()(float &val) const ->
        typename std::enable_if<std::numeric_limits<float>::is_iec559>::type {
        uint32_t encoded;
        (*this)(encoded);
        val = network::unpack754(encoded);
    }

    auto operator()(double &val) const ->
        typename std::enable_if<std::numeric_limits<double>::is_iec559>::type {
        uint64_t encoded;
        (*this)(encoded);
        val = network::unpack754(encoded);
    }

    template <typename T>
    auto operator()(T &val) const ->
        typename std::enable_if<std::is_enum<T>::value>::type {
        typename std::underlying_type<T>::type v;
        (*this)(v);
        val = static_cast<T>(v);
    }

    template <typename T, T v>
    void operator()(std::integral_constant<T, v>) const {
        using int_const_t = std::integral_constant<T, v>;
        typename int_const_t::value_type val;
        (*this)(val);
        if (val != int_const_t::value)
            throw std::domain_error(
                "Bad message: integral_constant values mismatch (actual: " +
                std::to_string(val) + ", expected: " +
                std::to_string(int_const_t::value));
    }

    void operator()(std::string &val) const {
        uint16_t length = 0;
        (*this)(length);
        val = std::string(asio::buffer_cast<char const *>(buf_), length);
        buf_ = buf_ + length;
    }

    template <typename T>
    void operator()(std::vector<T> &val) const {
        uint16_t length = 0;
        (*this)(length);
        for (size_t i = 0; i < length; ++i) {
            T v;
            (*this)(v);
            val.emplace_back(v);
        }
    }

    template <typename K, typename V>
    void operator()(std::unordered_map<K, V> &vals) const {
        uint16_t elCount = 0;
        (*this)(elCount);
        for (size_t i = 0; i < elCount; ++i) {
            std::pair<K, V> val;
            (*this)(val);
            vals.emplace(std::move(val));
        }
    }

    template <typename K, typename V>
    void operator()(std::pair<K, V> &val) const {
        (*this)(val.first);
        (*this)(val.second);
    }

    void operator()(
        std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::microseconds> &dt) const {
        int64_t dateTime;
        (*this)(dateTime);
        dt = std::chrono::time_point<std::chrono::system_clock,
                                     std::chrono::microseconds>{
            std::chrono::duration<int64_t, std::micro>{dateTime}};
    }

    template <typename T>
    auto operator()(T &val) const ->
        typename std::enable_if<fusion::traits::is_sequence<T>::value>::type {
        fusion::for_each(val, *this);
    }

    void operator()(optional_field_set<OptType>) const {
        OptType optInBuf;
        (*this)(optInBuf);
        optMask_ = typename optional_field_set<OptType>::value_type(optInBuf);
    }

    template <typename T, size_t N>
    void operator()(optional_field<T, N> &val) const {
        if (!optMask_) throw std::domain_error("Bad message");
        if ((*optMask_)[N]) {
            T v;
            (*this)(v);
            val = optional_field<T, N>(std::move(v));
        }
    }
};

using reader = reader_base<uint16_t>;

template <typename T>
std::pair<T, asio::const_buffer> read(asio::const_buffer buf) {
    reader r{std::move(buf)};
    T res;
    r(res);
    return std::make_pair(res, r.buf_);
}

} /* detail */

#endif /* ifndef READER_HPP_ */
