#pragma once

#include <cstddef>
#include <type_traits>
#include <stdexcept>
#include <utility>

/*template <typename T, size_t N>
constexpr size_t array_size(T(&)[N]) {
	return N;
}

constexpr size_t array_size(...) {
	return 1;
}*/


namespace resman {
	template <size_t N>
	struct Resource {
		constexpr Resource(const char* path) {}
		//static const char path[];
	};

	//template<size_t N> const char Resource<N>::path[] = "";


	/*template <typename>
	struct is_defined_t;

	template <size_t N>
	struct is_defined_t<Resource<N>> {
		static constexpr bool value = array_size(Resource<N>::path) > 1;
	};

	template <typename R>
	static constexpr bool is_defined = is_defined_t<R>::value;*/

	template <size_t N> // TODO: change to static Resource members
	extern const char resource_storage_begin[];

	template <size_t N>
	extern const uint32_t resource_storage_size;


	class ResourceHandle {
		const size_t res_id = 0;
		const uint32_t res_byte_size = 0;
		const char* res_begin_ptr = nullptr;
		const char* res_end_ptr = nullptr;

	public:
		template <size_t N>
		ResourceHandle(Resource<N>/*, std::enable_if_t<is_defined<Resource<N>>, void*> = 0*/)
			: res_id(N)
			, res_byte_size(resource_storage_size<N>)
			, res_begin_ptr(resource_storage_begin<N>)
			, res_end_ptr(res_begin_ptr + res_byte_size)
		{}

		const char* begin() {
			return res_begin_ptr;
		}
		const char* end() {
			return res_end_ptr;
		}
		size_t size() {
			return res_byte_size;
		}
		size_t id() {
			return res_id;
		}
	};

	/*namespace detail {
		constexpr int combine(int p) {
			return p;
		}

		template<class... Is>
		constexpr int combine(int val, int p0, Is... pp) {
			return combine(val * 10 + p0, pp...);
		}

		constexpr int parse(char C) {
			return (C >= '0' && C <= '9')
				? C - '0'
				: throw std::out_of_range("only decimal digits are allowed");
		}
	}

	template <char... Digits>
	constexpr auto operator"" _res() {
		return Resource<detail::combine(0, detail::parse(Digits)...)>{};
	}*/
}
