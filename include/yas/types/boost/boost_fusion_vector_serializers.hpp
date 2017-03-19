
// Copyright (c) 2010-2016 niXman (i dot nixman dog gmail dot com). All
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

#ifndef _yas__binary__boost_fusion_vector_serializer_hpp
#define _yas__binary__boost_fusion_vector_serializer_hpp

#include <yas/detail/config/config.hpp>

#if defined(YAS_SERIALIZE_BOOST_TYPES)
#include <yas/detail/type_traits/type_traits.hpp>
#include <yas/detail/type_traits/serializer.hpp>
#include <yas/detail/io/serialization_exception.hpp>

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/container/vector/vector_fwd.hpp>
#include <boost/fusion/include/vector_fwd.hpp>

namespace yas {
namespace detail {

/***************************************************************************/

namespace {

template<std::size_t I = 0, typename Archive, typename... Tp>
typename std::enable_if<I == sizeof...(Tp), Archive&>::type
apply(Archive &ar, boost::fusion::vector<Tp...> &)
{ return ar; }

template<std::size_t I = 0, typename Archive, typename... Tp>
typename std::enable_if<I < sizeof...(Tp), Archive&>::type
apply(Archive &ar, boost::fusion::vector<Tp...> &t) {
	ar & boost::fusion::at_c<I>(t);
	return apply<I+1>(ar, t);
}

} // anon ns

template<std::size_t F, typename... Types>
struct serializer<
	 type_prop::not_a_fundamental
	,ser_method::use_internal_serializer
	,F
	,boost::fusion::vector<Types...>
> {
	template<typename Archive>
	static Archive& save(Archive& ar, const boost::fusion::vector<Types...> &vector) {
		if (F & options::binary) {
			ar.write(YAS_SCAST(std::uint8_t, sizeof...(Types)));
		} else {
			ar.write(sizeof...(Types));
		}

		return apply(ar, YAS_CCAST(boost::fusion::vector<Types...> &, vector));
	}

	template<typename Archive>
	static Archive& load(Archive& ar, boost::fusion::vector<Types...> &vector) {
		if (F & options::binary) {
			std::uint8_t size = 0;
			ar.read(size);
			if ( size != sizeof...(Types) ) YAS_THROW_BAD_SIZE_ON_DESERIALIZE("fusion::vector");
		} else {
			std::uint32_t size = 0;
			ar.read(size);
			if ( size != sizeof...(Types) ) YAS_THROW_BAD_SIZE_ON_DESERIALIZE("fusion::vector");
		}

		return apply(ar, vector);
	}
};

/**************************************************************************/

} // namespace detail
} // namespace yas

#endif // defined(YAS_SERIALIZE_BOOST_TYPES)

#endif // _yas__binary__boost_fusion_vector_serializer_hpp
