#pragma once

namespace resman {
	// fwd
	class ResourceHandle;

	template <unsigned N>
	struct Resource {
		template <unsigned S>
		constexpr Resource(const char (&path)[S]) {}

	private:
		friend ResourceHandle;

		static const char storage_begin[];
		static const unsigned storage_size;
	};

	class ResourceHandle {
		const unsigned res_id = 0;
		const unsigned res_byte_size = 0;
		const char* res_begin_ptr = nullptr;
		const char* res_end_ptr = nullptr;

	public:
		template <unsigned N>
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
		unsigned size() {
			return res_byte_size;
		}
		unsigned id() {
			return res_id;
		}
	};
}
