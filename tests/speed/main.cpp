
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

#include <iostream>
#include <chrono>
#include <sstream>

#include <yas/binary_oarchive.hpp>
#include <yas/binary_iarchive.hpp>
#include <yas/text_iarchive.hpp>
#include <yas/text_oarchive.hpp>
#include <yas/mem_streams.hpp>

#include <boost/program_options.hpp>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/seq/size.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/enum.hpp>

/***************************************************************************/

enum options {
	 test_boost = 1<<0
	,test_yas   = 1<<1
	,test_binary= 1<<2
	,test_text  = 1<<3
};

/***************************************************************************/

struct struct_of_fundamentals {
	bool a;
	std::uint8_t b;
	std::uint16_t c;
	std::uint32_t d;
	std::uint64_t e;
	float f;
	double g;

	bool operator== (const struct_of_fundamentals &r) const {
		return
			a == r.a &&
			b == r.b &&
			c == r.c &&
			d == r.d &&
			e == r.e &&
			f == r.f &&
			g == r.g
		;
	}

    /** boost.serialization calls only this memfn */
	template<typename Archive>
	void serialize(Archive& ar, const unsigned int) {
		ar & a
			& b
			& c
			& d
			& e
			& f
			& g;
	}

	template<typename Archive>
	void serialize(Archive& ar) {
		ar & a
			& b
			& c
			& d
			& e
			& f
			& g;
	}
};

/***************************************************************************/

struct test_result {
	test_result()
		:save{}
		,load{}
		,size{}
	{}

	std::chrono::milliseconds save;
	std::chrono::milliseconds load;
	std::size_t size;
};

/***************************************************************************/

template<typename T, typename OA>
std::chrono::milliseconds save(const T &t, OA &oa, const std::size_t iterations) {
	auto s1 = std::chrono::system_clock::now();
	for ( size_t idx = 0; idx < iterations; ++idx ) {
		oa & t;
	}
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - s1);
}

template<typename T, typename IA>
std::chrono::milliseconds load(T &t, IA &ia, const std::size_t iterations) {
	auto s1 = std::chrono::system_clock::now();
	for ( size_t idx = 0; idx < iterations; ++idx ) {
		ia & t;
	}
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - s1);
}

/***************************************************************************/

template<typename T, typename OA, typename IA>
test_result boost_test(const T &ot, T &it, const std::size_t iterations, const std::size_t preallocated) {
	test_result res;
	std::string sb;
	sb.reserve(preallocated);
	std::ostringstream os(sb);
	OA oa(os, boost::archive::no_header);
	res.save = save(ot, oa, iterations);

	res.size = os.str().length();

	std::istringstream is(os.str());
	IA ia(is, boost::archive::no_header);
	res.load = load(it, ia, iterations);

	return res;
}

/***************************************************************************/

template<typename T, typename OA, typename IA>
test_result yas_test(const T &ot, T &it, const std::size_t iterations, const std::size_t preallocated) {
	test_result res;
	yas::mem_ostream os(preallocated);
	OA oa(os);
	res.save = save(ot, oa, iterations);

	res.size = os.get_intrusive_buffer().size;

	const yas::intrusive_buffer buf = os.get_intrusive_buffer();
	yas::mem_istream is(buf.data, buf.size);
	IA ia(is);
	res.load = load(it, ia, iterations);

	return res;
}

/***************************************************************************/

template<typename T>
void test(int options, const T &ot, T &it, const char *pref, const char *comment, const std::size_t iterations) {
	const std::size_t preallocated = sizeof(T)*iterations*10;

	if ( ((options & test_boost) || (options & test_yas)) && ((options & test_binary) || (options & test_text) ) ) {
		std::cout
		<< pref << comment << std::endl
		<< pref << "(sizeof(T)=" << sizeof(T) << ", iterations=" << iterations << ", preallocated=" << preallocated << " bytes)"
		<< std::endl;
	} else {
		return;
	}

	if ( options & test_binary ) {
		std::cout << "binary:" << std::endl;
		test_result bb, yb;
		if ( options & test_boost ) {
			bb = boost_test<
				 T
				,boost::archive::binary_oarchive
				,boost::archive::binary_iarchive
			>(ot, it, iterations, preallocated);
			std::cout
			<< "   boost save time  : " << bb.save.count() << " ms" << std::endl
			<< "   boost load time  : " << bb.load.count() << " ms" << std::endl
			<< "   boost data size  : " << bb.size << std::endl;
		}
		if ( options & test_yas ) {
			yb = yas_test<
				 T
                ,yas::binary_oarchive<yas::mem_ostream, yas::binary|yas::no_header>
                ,yas::binary_iarchive<yas::mem_istream, yas::binary|yas::no_header>
			>(ot, it, iterations, preallocated);
			std::cout
			<< "   yas save time    : " << yb.save.count() << " ms" << std::endl
			<< "   yas load time    : " << yb.load.count() << " ms" << std::endl
			<< "   yas data size    : " << yb.size << std::endl;
		}
		if ( (options & test_boost ) && (options & test_yas ) ) {
			std::cout
			<< "   yas save speed up: " << (((double)bb.save.count())/((double)yb.save.count())) << std::endl
			<< "   yas load speed up: " << (((double)bb.load.count())/((double)yb.load.count())) << std::endl;
		}
	}

	if ( options & test_text ) {
		std::cout << "text:" << std::endl;
		test_result yt, bt;
		if ( options & test_boost ) {
			bt = boost_test<
				 T
				,boost::archive::text_oarchive
				,boost::archive::text_iarchive
			>(ot, it, iterations, preallocated);
			std::cout
			<< "   boost save time  : " << bt.save.count() << " ms" << std::endl
			<< "   boost load time  : " << bt.load.count() << " ms" << std::endl
			<< "   boost data size  : " << bt.size << std::endl;
		}
		if ( options & test_yas ) {
			yt = yas_test<
				 T
                ,yas::text_oarchive<yas::mem_ostream, yas::text|yas::no_header>
                ,yas::text_iarchive<yas::mem_istream, yas::text|yas::no_header>
			>(ot, it, iterations, preallocated);
			std::cout
			<< "   yas save time    : " << yt.save.count() << " ms" << std::endl
			<< "   yas load time    : " << yt.load.count() << " ms" << std::endl
			<< "   yas data size    : " << yt.size << std::endl;
		}
		if ( (options & test_boost ) && (options & test_yas ) ) {
			std::cout
			<< "   yas save speed up: " << (((double)bt.save.count())/((double)yt.save.count())) << std::endl
			<< "   yas load speed up: " << (((double)bt.load.count())/((double)yt.load.count())) << std::endl;
		}
	}

	std::cout << std::endl << std::endl;
}

/***************************************************************************/

#define GENERATE_TESTS_ITEM_IMPL(type, init) { \
		const type ovar = {BOOST_PP_TUPLE_ENUM(init)}; \
		type ivar; \
		test<type>(options, ovar, ivar, ">>>>>>>>: ", "test for \"" BOOST_PP_STRINGIZE(type) "\" type", iterations); \
		assert(ivar == ovar); \
	}

#define GENERATE_TESTS_ITEM(unused, idx, seq) \
	GENERATE_TESTS_ITEM_IMPL( \
		 BOOST_PP_TUPLE_ELEM(0, BOOST_PP_SEQ_ELEM(idx, seq)) \
		,BOOST_PP_TUPLE_ELEM(1, BOOST_PP_SEQ_ELEM(idx, seq)) \
	)

#define GENERATE_TESTS(seq) \
	BOOST_PP_REPEAT( \
		 BOOST_PP_SEQ_SIZE(seq) \
		,GENERATE_TESTS_ITEM \
		,seq \
	)

/***************************************************************************/

int main(int ac, char **av) {
	setvbuf(stdout, 0, _IONBF, 0);
	std::cout << "platform bits: " << (sizeof(void*) == sizeof(std::uint64_t) ? "64" : "32") << std::endl;

	int options = test_boost|test_yas|test_binary|test_text;

	namespace po = boost::program_options;
	po::variables_map optsmap;

	try {
		po::options_description desc;
		desc.add_options()
			("help,h"     , "show help")
			("no-boost,b" , "no test boost serialization")
			("no-yas,y"   , "no test yas serialization")
			("no-text,t"  , "no test text archives")
			("no-binary,x", "no test binary archives")
		;

		po::store(po::command_line_parser(ac, av).options(desc).run(), optsmap);
		po::notify(optsmap);

		if ( optsmap.count("help") ) {
			std::cout << desc << std::endl;
			return 0;
		}

		if ( optsmap.count("no-boost") )
			options ^= test_boost;
		if ( optsmap.count("no-yas") )
			options ^= test_yas;
		if ( optsmap.count("no-text") )
			options ^= test_text;
		if ( optsmap.count("no-binary") )
			options ^= test_binary;
	} catch (const std::exception &ex) {
		std::cout << "exception was thrown: " << ex.what() << std::endl;
		return 1;
	}

	try {
		enum { iterations = 1024u*1024u*4u };

		GENERATE_TESTS(
			((bool          , (true)))
			((std::uint8_t  , ('y')))
			((std::uint16_t , (0xDEAD)))
			((std::uint32_t , (0xDEADBEAF)))
			((std::uint64_t , (0xDEADBEAFDEADBEAF)))
			((float         , (3.141)))
			((double        , (3.141)))
			((struct_of_fundamentals, (0, 1, 0xDEAD, 0xDEADBEEF, 0xDEADBEEFDEADBEEF, 3.141, 3.141)))
		)
	} catch (const std::exception &ex) {
		std::cout << "[exception]: " << ex.what() << std::endl;
	}
}

/***************************************************************************/
