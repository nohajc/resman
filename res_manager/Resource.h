#pragma once

#include <cstdint>

namespace resman {
	// fwd
	class ResourceHandle;

	template <uint32_t N>
	struct Resource {
		template <uint32_t S>
		constexpr Resource(const char (&path)[S]) {}

	private:
		friend ResourceHandle;

		static const char storage_begin[];
		static const uint32_t storage_size;
	};

	class ResourceHandle {
		const uint32_t res_id = 0;
		const uint32_t res_byte_size = 0;
		const char* res_begin_ptr = nullptr;
		const char* res_end_ptr = nullptr;

	public:
		template <uint32_t N>
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
		uint32_t size() {
			return res_byte_size;
		}
		uint32_t id() {
			return res_id;
		}
	};
}
