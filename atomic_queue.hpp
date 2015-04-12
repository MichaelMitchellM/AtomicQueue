
#pragma once

#include <atomic>
#include <cstdio>
#include <cstring> // memcpy

namespace MMM{

	template<typename T>
	class AtomicQueue{
	private:

		T* data_;

		std::atomic_uint32_t capacity_;
		std::atomic_uint32_t head_;
		std::atomic_uint32_t size_;
		
		std::atomic_bool resizing_;

	public:
		AtomicQueue()
			:
			data_{ new T[16u] },
			capacity_{ 16u },
			size_{ 0u },
			head_{ 0u }
		{
			resizing_.store(false);

		}

		AtomicQueue(unsigned initial_capacity)
			:
			data_{ new T[initial_capacity] },
			capacity_{ initial_capacity },
			size_{ 0u },
			head_{ 0u }
		{
			resizing_.store(false);

		}

		~AtomicQueue(){

			// need thread protection!

			delete[] data_;
		}

		void PushBack(T& data){

			auto size = size_.fetch_add(1u);
			auto capacity = capacity_.load();

			if (size >= capacity){
				auto expected = false;
				if (resizing_.compare_exchange_strong(expected, true)){
					printf("resizing: %u\n", capacity);
					// possibly try to expand instead of memcpy

					auto new_array = new T[capacity * 2u];
					std::memcpy(new_array, data_, sizeof(T) * capacity);
					
					// do I need to delete each individual element?
					delete[] data_;

					data_ = new_array;


					capacity_.store(2u * capacity);
					size_.store(capacity);
					resizing_.store(false);
				}
				else{
					// spin lock

					// do while loop?
					expected = false;
					while (!resizing_.compare_exchange_weak(expected, false)){
						expected = false;
					}

					// re-get size
					size = size_.fetch_add(1u);
					
					// perhaps even recurse?
					//return PushBack(data);
				}
			}

			// could there be a data race if something tries to store
			// while the array is resized?
			// what about PopFront, the size is incremented before the data is
			// placed in the array?
			// PopFront could try and read that empty space
			data_[size] = data;
		}

		T& PopFront(){
			// need to git good

			// spin lock
			auto expected = false;
			while (!resizing_.compare_exchange_weak(expected, false)){
				expected = false;
			}

			auto head = head_.fetch_add(1u);
			auto size = size_.load();

			if ((head + 1) >= size){
				expected = false;
				if (resizing_.compare_exchange_strong(expected, true)){

					// no need to garbage collect items
					// just set size and head to 0;

					head_.store(0u);
					size_.store(0u);
					resizing_.store(false);

				}
				else{

					// spin lock

					expected = false;
					while (!resizing_.compare_exchange_weak(expected, false)){
						expected = false;
					}

					// re-get head -> return null?

				}
			}
			else if (size == 0u){

				//return NULL;

			}

			return data_[head];
		}

		unsigned size(){ return size_.load(); }
		unsigned head(){ return head_.load(); }

	};

}

