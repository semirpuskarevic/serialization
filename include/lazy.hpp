#ifndef LAZY_HPP_
#define LAZY_HPP_

#include <boost/asio/buffer.hpp>
#include <boost/optional.hpp>
#include <exception>

namespace asio = boost::asio;

namespace detail {

template <typename T>
struct lazy {
    asio::const_buffer buf_;
    mutable boost::optional<T> val_;

    lazy() : val_(T()) {}

    lazy(T const& val) : val_(val) {}

    lazy(asio::const_buffer const& buf);

    T& get();
    const T& get() const;

    bool has_value() const { return !!val_; }
    size_t buffer_size() const { return asio::buffer_size(buf_); }
};
} /* detail */

#include "sizer.hpp"
template <typename T>
detail::lazy<T>::lazy(asio::const_buffer const& buf)
    : buf_(asio::buffer_cast<void const*>(buf), get_size<T>(buf)) {}

#include "reader.hpp"
template <typename T>
T& detail::lazy<T>::get() {
    if (!has_value()) val_ = read<T>(buf_).first;
    return *val_;
}

template <typename T>
const T& detail::lazy<T>::get() const {
    if (!has_value()) val_ = read<T>(buf_).first;
    return *val_;
}
#endif /* ifndef LAZY_HPP_ */
