#ifndef SIZER_HPP_
#define SIZER_HPP_

#include <boost/asio/buffer.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <unordered_map>
#include "network.hpp"
#include "optional.hpp"
#include <chrono>

namespace asio = boost::asio;
namespace fusion = boost::fusion;

namespace detail {

template <typename OptType>
class sizer_base {
public:
    explicit sizer_base(asio::const_buffer buf)
        : buf_{std::move(buf)}, size_{0} {}

    template <typename T>
    auto operator()(T const &) const ->
        typename std::enable_if<std::is_integral<T>::value>::type {
        constexpr size_t sz = sizeof(T);
        size_ += sz;
        buf_ = buf_ + sz;
    }

    void operator() (float) const {
        uint32_t encoded;
        (*this)(encoded);
    }

    void operator() (double) const {
        uint64_t encoded;
        (*this)(encoded);
    }


    template <typename T>
    auto operator()(T const &) const ->
        typename std::enable_if<std::is_enum<T>::value>::type {
        typename std::underlying_type<T>::type t;
        (*this)(t);
    }

    template <typename T, T v>
    void operator()(std::integral_constant<T, v> const &) const {
        using int_const_t = std::integral_constant<T, v>;
        (*this)(int_const_t::value);
    }

    void operator()(std::string const &) const {
        uint16_t len = read_length_from_buffer();
        size_ += len;
        buf_ = buf_ + len;
    }

    template <typename T>
    void operator()(std::vector<T> const &) const {
        uint16_t len = read_length_from_buffer();
        for (size_t i = 0; i < len; ++i) (*this)(T{});
    }

    template <typename K, typename V>
    void operator()(std::unordered_map<K, V> const &) const {
        uint16_t len = read_length_from_buffer();
        for (size_t i = 0; i < len; ++i) {
            (*this)(std::pair<K, V>{});
        }
    }

    template <typename K, typename V>
    void operator()(std::pair<K, V> const &) const {
        (*this)(K{});
        (*this)(V{});
    }

    void operator()(std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds> const &) const {
        (*this)(int64_t{});
    }

    template <typename T>
    auto operator()(T const &val) const ->
        typename std::enable_if<fusion::traits::is_sequence<T>::value>::type {
        fusion::for_each(val, *this);
    }

    void operator()(optional_field_set<OptType> const &) const {
        opt_mask_ = network::ntoh(*asio::buffer_cast<uint16_t const *>(buf_));
        (*this)(OptType{});
    }

    template <typename T, size_t N>
    void operator()(optional_field<T, N> const &) const {
        if (opt_mask_[N]) {
            (*this)(T{});
        }
    }

    size_t size() const { return size_; }

private:
    mutable asio::const_buffer buf_;
    mutable size_t size_;
    mutable typename optional_field_set<OptType>::bits_type opt_mask_;

    uint16_t read_length_from_buffer() const {
        uint16_t len =
            network::ntoh(*asio::buffer_cast<uint16_t const *>(buf_));
        size_ += sizeof(len);
        buf_ = buf_ + sizeof(len);
        return len;
    }
};

class sizer : public sizer_base<uint16_t> {
    using sizer_base<uint16_t>::sizer_base;
};

template <typename T>
size_t get_size(asio::const_buffer buf) {
    sizer s{std::move(buf)};
    s(T{});
    return s.size();
}

} /* detail */
#endif /* ifndef SIZER_HPP_ */
