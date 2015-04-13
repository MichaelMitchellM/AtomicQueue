
#pragma once

#include <atomic>
#include <cstring> // memcpy

namespace MMM{

	template<typename T>
	class AtomicQueue_1{
	private:

		T* data_;

		std::atomic_uint32_t a_capacity_;
		std::atomic_uint32_t a_size_;
		std::atomic_uint32_t a_head_;
		
		// sentinel variables
		std::atomic_uint32_t a_placing_;
		std::atomic_bool a_resizing_;

	public:
		AtomicQueue_1()
			:
			data_{ new T[16u] },
			a_capacity_{ 16u },
			a_size_{ 0u },
			a_head_{ 0u },
			a_placing_{ 0u }
		{
			// atomic_bool cannot be constructed in the initializer list
			a_resizing_.store(false);

		}

		AtomicQueue_1(unsigned initial_capacity)
			:
			data_{ new T[initial_capacity] },
			a_capacity_{ initial_capacity },
			a_size_{ 0u },
			a_head_{ 0u },
			a_placing_{ 0u }
		{
			// atomic_bool cannot be constructed in the initializer list
			a_resizing_.store(false);

		}

		~AtomicQueue_1(){

			// need thread protection!

			delete[] data_;
		}

		void PushBack(T& data){

			// increment placement sentinel
			a_placing_.fetch_add(1);


			auto size = a_size_.fetch_add(1u);
			auto capacity = a_capacity_.load();

			if (size >= capacity){
				
				a_placing_.fetch_sub(1u);

				auto expected_placing = 0u;
				while (!a_placing_.compare_exchange_weak(expected_placing, 0u)){
					expected_placing = 0u;
				}

				auto expected_resizing = false;
				if (a_resizing_.compare_exchange_strong(expected_resizing, true)){
					
					auto old_array = data_;
					auto new_array = new T[2u * capacity]; // error causer?

					data_ = new_array;

					std::memcpy(new_array, old_array, sizeof(T) * capacity);
					delete[] old_array; // error causer!
					
					// this could be moved before the memcpy

					//size = capacity + 1u;
					a_capacity_.store(2u * capacity);
					a_size_.store(capacity);
					a_resizing_.store(false);

					return PushBack(data);
				}
				else{
					// spin lock
					auto expected_resizing = false;
					while (!a_resizing_.compare_exchange_weak(expected_resizing, false)){
						expected_resizing = false;
					}

					// re-get size
					//size = a_size_.fetch_add(1u);
					
					// perhaps even recurse?
					return PushBack(data);
				}
			}

			// could there be a data race if something tries to store
			// while the array is resized?
			// what about PopFront, the size is incremented before the data is
			// placed in the array?
			// PopFront could try and read that empty space
			

			data_[size] = data;     // error causer!
			a_placing_.fetch_sub(1u);

		}

		T PopFront(){
			// need to git good

			// spin lock
			auto expected = false;
			while (!a_resizing_.compare_exchange_weak(expected, false)){
				expected = false;
			}

			auto head = a_head_.fetch_add(1u);
			auto size = a_size_.load();

			if ((head + 1) >= size){
				expected = false;
				if (a_resizing_.compare_exchange_strong(expected, true)){

					// no need to garbage collect items
					// just set size and head to 0;

					a_head_.store(0u);
					a_size_.store(0u);
					a_resizing_.store(false);

				}
				else{

					// spin lock

					expected = false;
					while (!a_resizing_.compare_exchange_weak(expected, false)){
						expected = false;
					}

					// re-get head -> return null?

				}
			}
			else if (size == 0u){

				//return NULL;

			}

			// hidden data race?
			return a_data_.load()[head];
		}

		unsigned size(){ return a_size_.load(); }
		unsigned head(){ return a_head_.load(); }

	};

}

