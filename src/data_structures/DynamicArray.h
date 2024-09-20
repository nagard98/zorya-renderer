#ifndef DYNAMIC_ARRAY_H_
#define DYNAMIC_ARRAY_H_

#include "utils/Arena.h"

#include <type_traits>

namespace zorya
{

	class Dynamic_Array_Storage
	{
	public:
		Dynamic_Array_Storage(uint64_t _element_size, uint64_t _alignment, Arena* _arena);

		template <typename T>
		void push_back(T&& value)
		{
			//static_assert(sizeof(T) == element_size, "Type mismatch");
			//static_assert(alignof(T) == alignment, "Element alignment mismatch with storage");
			static_assert(std::is_trivially_copyable<T>::value == true, "Array currently supports only trivially copyable members");
			zassert(sizeof(T) == element_size);
			zassert(alignof(T) == alignment);

			if (size == capacity) zassert(false); //TODO
			*(static_cast<T*>(data) + size) = std::forward<T>(value);
			size += 1;
		}

		template <typename T>
		void pop_back()
		{
			zassert(size > 0);
			size -= 1;
			memset((static_cast<T*>(data) + size), 0, sizeof(T));
		}

		template <typename T>
		T& get(uint64_t index)
		{
			zassert(index < size);
			return *(static_cast<T*>(data) + index);
		}

	private:

		void* data;
		uint64_t size;
		uint64_t capacity;
		uint64_t element_size;
		uint64_t alignment;

		Arena* arena;
	};



	template <typename T>
	class Dynamic_Array_View
	{

	public:
		Dynamic_Array_View(Dynamic_Array_Storage& storage) : dynamic_array_storage(storage) {}

		T& operator[](uint64_t index);

		void push_back(T&& value);
		void pop_back();


	private:
		Dynamic_Array_Storage& dynamic_array_storage;

	};


	template <typename T>
	class Dynamic_Array : public Dynamic_Array_View<T>
	{

	public:
		Dynamic_Array(Arena& arena) : storage(sizeof(T), alignof(T), &arena), Dynamic_Array_View<T>(storage) {}
	
	private:
		Dynamic_Array_Storage storage;

	};


	template<typename T>
	inline T& Dynamic_Array_View<T>::operator[](uint64_t index)
	{
		return dynamic_array_storage.get<T>(index);
	}

	template<typename T>
	inline void Dynamic_Array_View<T>::push_back(T&& value)
	{
		dynamic_array_storage.push_back<T>(std::forward<T>(value));
	}

	template<typename T>
	inline void Dynamic_Array_View<T>::pop_back()
	{
		dynamic_array_storage.pop_back<T>();
	}

}
#endif // !DYNAMIC_ARRAY_H_
