#pragma once

#include <cstddef>
#include <type_traits>
#include <stdexcept>
#include <utility>

namespace resman {
	// fwd
	class ResourceHandle;

	template <size_t N>
	struct Resource {
		constexpr Resource(const char* path) {}

	private:
		friend ResourceHandle;

		static const char storage_begin[];
		static const uint32_t storage_size;
	};

	class ResourceHandle {
		const size_t res_id = 0;
		const uint32_t res_byte_size = 0;
		const char* res_begin_ptr = nullptr;
		const char* res_end_ptr = nullptr;

	public:
		template <size_t N>
		ResourceHandle(Resource<N>)
			: res_id(N)
			, res_byte_size(Resource<N>::storage_size)
			, res_begin_ptr(Resource<N>::storage_begin)
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
}
