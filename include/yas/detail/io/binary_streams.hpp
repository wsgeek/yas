
// Copyright (c) 2010-2019 niXman (i dot nixman dog gmail dot com). All
// rights reserved.
//
// This file is part of YAS(https://github.com/niXman/yas) project.
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifndef __yas__detail__io__binary_streams_hpp
#define __yas__detail__io__binary_streams_hpp

#include <yas/detail/config/config.hpp>
#include <yas/detail/io/io_exceptions.hpp>
#include <yas/detail/io/endian_conv.hpp>
#include <yas/detail/type_traits/type_traits.hpp>

namespace yas {
namespace detail {

/**************************************************************************/

template<typename OS, std::size_t F>
struct binary_ostream {
    binary_ostream(OS &os)
        :os(os)
    {}

    // TODO:
    void write_seq_size(std::size_t size) {
        const auto tsize = __YAS_SCAST(std::uint64_t, size);
        write(tsize);
    }

    // for arrays
    template<typename T>
    void write(const T *ptr, std::size_t size) {
        __YAS_THROW_WRITE_ERROR(size != os.write(ptr, size));
    }

    template<typename T>
    void write(T v, __YAS_ENABLE_IF_IS_ANY_OF(T, char, signed char, unsigned char, bool)) {
        __YAS_THROW_WRITE_ERROR(sizeof(v) != os.write(&v, sizeof(v)));
    }

    // for signed
    template<typename T>
    void write(T v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::int16_t, std::int32_t, std::int64_t)) {
        __YAS_CONSTEXPR_IF ( F & yas::compacted ) {
            if ( v >= 0 ) {
                typename std::make_unsigned<T>::type uv = v;
                if ( uv >= (1u<<6) ) {
                    const std::uint8_t ns = storage_size(uv);
                    write(__YAS_SCAST(std::uint8_t, ns | __YAS_SCAST(std::uint8_t, 0u<<7)));
                    __YAS_THROW_WRITE_ERROR(ns != os.write(&uv, ns));
                } else {
                    // one byte
                    uv |= __YAS_SCAST(std::uint8_t, (1u<<6|0u<<7));
                    __YAS_THROW_WRITE_ERROR(1 != os.write(&uv, 1));
                }
            } else {
                typename std::make_unsigned<T>::type uv = std::abs(v);
                if ( uv >= (1u<<6) ) {
                    const std::uint8_t ns = storage_size(uv);
                    write(__YAS_SCAST(std::uint8_t, ns | __YAS_SCAST(std::uint8_t, 1u<<7)));
                    __YAS_THROW_WRITE_ERROR(ns != os.write(&uv, ns));
                } else {
                    // one byte
                    uv |= __YAS_SCAST(std::uint8_t, (1u<<6|1u<<7));
                    __YAS_THROW_WRITE_ERROR(1 != os.write(&uv, 1));
                }
            }
        } else {
            v = endian_converter<__YAS_BSWAP_NEEDED(F)>::bswap(v);
            __YAS_THROW_WRITE_ERROR(sizeof(v) != os.write(&v, sizeof(v)));
        }
    }

    // for unsigned
    template<typename T>
    void write(T v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::uint16_t, std::uint32_t, std::uint64_t)) {
        __YAS_CONSTEXPR_IF ( F & yas::compacted ) {
            if ( v >= (1u<<7) ) {
                const std::uint8_t ns = storage_size(v);
                write(ns);
                __YAS_THROW_WRITE_ERROR(ns != os.write(&v, ns));
            } else {
                // one byte
                v |= __YAS_SCAST(std::uint8_t, 1u<<7);
                __YAS_THROW_WRITE_ERROR(1 != os.write(&v, 1));
            }
        } else {
            v = endian_converter<__YAS_BSWAP_NEEDED(F)>::bswap(v);
            __YAS_THROW_WRITE_ERROR(sizeof(v) != os.write(&v, sizeof(v)));
        }
    }

    // for float and double
    template<typename T>
    void write(const T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, float, double)) {
        const auto r = endian_converter<__YAS_BSWAP_NEEDED(F)>::to_network(v);
        __YAS_THROW_WRITE_ERROR(sizeof(r) != os.write(&r, sizeof(r)));
    }

private:
    OS &os;

private:
    template<typename T>
    static constexpr std::uint8_t storage_size(const T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::uint16_t)) {
        return __YAS_SCAST(std::uint8_t, (v < (1u<<8 )) ? 1u : 2u);
    }
    template<typename T>
    static constexpr std::uint8_t storage_size(const T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::uint32_t)) {
        return __YAS_SCAST(std::uint8_t,
            (v < (1u<<16) )
                ? ((v < (1u<<8 )) ? 1u : 2u)
                : ((v < (1u<<24)) ? 3u : 4u)
        );
    }
    template<typename T>
    static constexpr std::uint8_t storage_size(const T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::uint64_t)) {
        return __YAS_SCAST(std::uint8_t,
            (v < (1ull<<32))
                ? (v < (1u<<16) )
                    ? ((v < (1u<<8 )) ? 1u : 2u)
                    : ((v < (1u<<24)) ? 3u : 4u)
                : (v < (1ull<<48) )
                    ? ((v < (1ull<<40)) ? 5u : 6u)
                    : ((v < (1ull<<56)) ? 7u : 8u)
        );
    }
};

/***************************************************************************/

template<typename IS, std::size_t F>
struct binary_istream {
    binary_istream(IS &is)
        :is(is)
    {}

    std::size_t read_seq_size() {
        std::uint64_t size{};
        read(size);

        return __YAS_SCAST(std::size_t, size);
    }

    bool empty() const { return is.empty(); }
    char peekch() const { return is.peekch(); }
    char getch() { return is.getch(); }
    void ungetch(char ch) { is.ungetch(ch); }

    // for arrays
    std::size_t read(void *ptr, std::size_t size) {
        __YAS_THROW_READ_ERROR(size != is.read(ptr, size));

        return size;
    }

    // for chars & bools
    template<typename T>
    void read(T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, char, signed char, unsigned char, bool)) {
        __YAS_THROW_READ_ERROR(sizeof(v) != is.read(&v, sizeof(v)));
    }

    // for signed
    template<typename T>
    void read(T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::int16_t, std::int32_t, std::int64_t)) {
        __YAS_CONSTEXPR_IF ( F & yas::compacted ) {
            std::uint8_t ns = __YAS_SCAST(std::uint8_t, is.getch());
            const bool neg = __YAS_SCAST(bool, (ns >> 7) & 1u);
            const bool onebyte = __YAS_SCAST(bool, (ns >> 6) & 1u);
            ns &= ~((1u << 7) | (1u << 6));
            if ( !onebyte ) {
                typename std::make_unsigned<T>::type av = 0;
                __YAS_THROW_READ_STORAGE_SIZE_ERROR(sizeof(av) < ns);
                __YAS_THROW_READ_ERROR(ns != is.read(&av, ns));
                v = (neg ? -__YAS_SCAST(T, av) : __YAS_SCAST(T, av));
            } else {
                v = (neg ? -__YAS_SCAST(T, ns) : __YAS_SCAST(T, ns));
            }
        } else {
            __YAS_THROW_READ_ERROR(sizeof(v) != is.read(&v, sizeof(v)));
            v = endian_converter<__YAS_BSWAP_NEEDED(F)>::bswap(v);
        }
    }

    // for unsigned
    template<typename T>
    void read(T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, std::uint16_t, std::uint32_t, std::uint64_t)) {
       __YAS_CONSTEXPR_IF ( F & yas::compacted ) {
            std::uint8_t ns = __YAS_SCAST(std::uint8_t, is.getch());
            const bool onebyte = __YAS_SCAST(bool, (ns >> 7) & 1u);
            ns &= ~(1u << 7);
            if ( !onebyte ) {
                __YAS_THROW_READ_STORAGE_SIZE_ERROR(sizeof(v) < ns);
                __YAS_THROW_READ_ERROR(ns != is.read(&v, ns));
            } else {
                v = __YAS_SCAST(T, ns);
            }
        } else {
            __YAS_THROW_READ_ERROR(sizeof(v) != is.read(&v, sizeof(v)));
            v = endian_converter<__YAS_BSWAP_NEEDED(F)>::bswap(v);
        }
    }

    // for floats and doubles
    template<typename T>
    void read(T &v, __YAS_ENABLE_IF_IS_ANY_OF(T, float, double)) {
        typename storage_type<T>::type r;
        __YAS_THROW_READ_ERROR(sizeof(r) != is.read(&r, sizeof(r)));
        v = endian_converter<__YAS_BSWAP_NEEDED(F)>::template from_network<T>(r);
    }

private:
    IS &is;
};

/**************************************************************************/

} // ns detail
} // ns yas

#endif // __yas__detail__io__binary_streams_hpp
