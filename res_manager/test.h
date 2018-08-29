#pragma once

#include "Resource.h"

namespace resman {
	constexpr Resource<427> gResFoo("foo.txt");
	constexpr Resource<213> gResBar("bar.txt");

	//using ResFoo = Resource<427>;
	//template<> const char ResFoo::path[] = "foo.txt";
	//using ResBar = Resource<213>;
	//template<> const char ResBar::path[] = "bar.txt";
}
