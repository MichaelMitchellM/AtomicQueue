
#pragma once

#include <atomic>   // duh
#include <cstring> // memcpy

namespace MMM{

	template<typename T>
	class AtomicQueue_1{
	private:

		T* data_;

		// should these be uint64?
		std::atomic_uint32_t a_capacity_;
		std::atomic_uint32_t a_size_;
		std::atomic_uint32_t a_head_;
		
		// sentinel variables
		// uint32 is definately overkill
		std::atomic_uint32_t a_placing_;
		std::atomic_uint32_t a_getting_;
		std::atomic_bool a_resizing_;
		std::atomic_bool a_resetting_;

	public:
		AtomicQueue_1()
			:
			data_{ new T[16u] },
			a_capacity_{ 16u },
			a_size_{ 0u },
			a_head_{ 0u },
			a_placing_{ 0u },
			a_getting_{ 0u }

		{
			// atomic_bool cannot be constructed in the initializer list
			a_resizing_.store(false);
			a_resetting_.store(false);

		}

		AtomicQueue_1(unsigned initial_capacity)
			:
			data_{ new T[initial_capacity] },
			a_capacity_{ initial_capacity },
			a_size_{ 0u },
			a_head_{ 0u },
			a_placing_{ 0u },
			a_getting_{ 0u }
		{
			// atomic_bool cannot be constructed in the initializer list
			a_resizing_.store(false);
			a_resetting_.store(false);

		}

		~AtomicQueue_1(){

			// need thread protection!

			delete[] data_;
		}

		void PushBack(T& data){
			auto size = a_size_.fetch_add(1u);
			auto capacity = a_capacity_.load();

			if (size >= capacity){
				
				auto expected_resizing = false;
				if (a_resizing_.compare_exchange_strong(expected_resizing, true)){
					
					// I would prefer this to be done after the memcpy
					// if possible ofc
					auto expected_placing = 0u;
					while (!a_placing_.compare_exchange_weak(expected_placing, 0u)){
						expected_placing = 0u;
					}

					auto old_array = data_;
					auto new_array = new T[2u * capacity];
					data_ = new_array;

					std::memcpy(new_array, old_array, sizeof(T) * capacity);
					delete[] old_array;
					
					size = capacity;
					a_capacity_.store(2u * capacity);
					a_size_.store(capacity + 1u);
					a_resizing_.store(false);
				}
				else{
					// spin lock
					auto expected_resizing = false;
					while (!a_resizing_.compare_exchange_weak(expected_resizing, false)){
						expected_resizing = false;
					}

					// re-get size
					size = a_size_.fetch_add(1u);
				}
			}
			
			// increment placement sentinel
			a_placing_.fetch_add(1u);

			// this needs to be watched,
			// it is a suspect for data races
			data_[size] = data;

			// decrement placement sentinel
			a_placing_.fetch_sub(1u);

		}

		// void PushBack(T&& data)

		T PopFront(){

			auto head = a_head_.fetch_add(1u);
			auto size = a_size_.load();

			// need to watch out for the case when trying to pop something while resizing

			if ((head + 1) >= size){
				auto expected_reset = false;
				if (a_resetting_.compare_exchange_strong(expected_reset, true)){

					auto expected_getting = 0u;
					while (!a_placing_.compare_exchange_weak(expected_getting, 0u)){
						expected_getting = 0u;
					}

					a_head_.store(0u);
					a_size_.store(0u);

					a_resetting_.store(false);

				}
				else{

					auto expected_reset = false;
					while (!a_resetting_.compare_exchange_weak(expected_reset, false)){
						expected_reset = false;
					}

				}

			}
			else if (size == 0u){

				return 0u;

			}

			a_getting_.fetch_add(1u);

			// data race!
			auto data = data_[head];

			a_getting_.fetch_sub(1u);

			return data;
		}
		
		unsigned size(){ return a_size_.load(); }
		unsigned head(){ return a_head_.load(); }

	};

}

