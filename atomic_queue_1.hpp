
#pragma once

#include <atomic>   // duh
#include <cstring> // memcpy

#define MMM_MAGIC_RATIO 50u

namespace MMM{

	template<typename T>
	class AtomicQueue_1{
	private:

		T* data_;

		// should these be uint64?
		std::atomic_uint32_t a_capacity_;
		std::atomic_uint32_t a_size_;

		std::atomic_uint32_t a_head_;
		std::atomic_uint32_t a_tail_;

		// sentinel variables
		// uint32 is definately overkill
		std::atomic_uint32_t a_placing_;
		std::atomic_bool a_resizing_;

	public:
		AtomicQueue_1()
			:
			data_{ new T[16u] },
			a_capacity_{ 16u },
			a_size_{ 0u },
			a_head_{ 0u },
			a_tail_{ 0u },
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
			a_tail_{ 0u },
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
			auto size = a_size_.fetch_add(1u);
			auto capacity = a_capacity_.load();

			if (size >= capacity){
				
				// might want to find a way to use c_e_w here instead

				auto expected_resizing = false;
				if (a_resizing_.compare_exchange_strong(expected_resizing, true)){
					
					// unsure if these are safe before teh c_e_w
					auto head = a_head_.load();
					// might want to make this to a float
					auto usage_ratio = 100u * head / capacity;
					auto usable_elements = capacity - head;

					// I would prefer this to be done after the memcpy
					auto expected_placing = 0u;
					while (!a_placing_.compare_exchange_weak(expected_placing, 0u)){
						expected_placing = 0u;
					}

					// pls find a good ratio that is based on measurements!
					if (usage_ratio > MMM_MAGIC_RATIO){
						// more empty space than used space if ratio is 50%
						// no need to resize, just reset head and reduce size and tail

						// elements from data_ to data_ + head need to be destructed / deleted
						// use std::is_pointer
						std::memcpy(data_, data_ + head, sizeof(T) * usable_elements);

						a_head_.store(0u);
						a_size_.store(usable_elements);
						a_tail_.store(usable_elements);
					}
					else{
						auto old_array = data_;
						auto new_array = new T[2u * capacity];
						data_ = new_array;

						// elements from data_ to data_ + head need to be destructed / deleted
						// use std::is_pointer
						std::memcpy(new_array, old_array + head, sizeof(T) * usable_elements);
						delete[] old_array;

						size = capacity;
						a_capacity_.store(2u * capacity);
						a_size_.store(capacity + 1u);
					}

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

			// suspect for data races
			data_[size] = data;

			// increment tail index
			a_tail_.fetch_add(1u);

			// decrement placement sentinel
			a_placing_.fetch_sub(1u);

		}

		// void PushBack(T&& data)

		T PopFront(){

			auto head = a_head_.fetch_add(1u);
			auto tail = a_tail_.load();

			if (tail == 0u){

				// this needs to be better!
				return 0u;

			}
			else if ((head + 1) >= tail){

				// do something meaningful!
				return 0u;

			}

			// data racer, zoom zoom
			auto data = data_[head];

			return data;
		}
		
		unsigned size(){ return a_size_.load(); }
		unsigned head(){ return a_head_.load(); }

	};

}

