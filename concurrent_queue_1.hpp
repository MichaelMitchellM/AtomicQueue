
#pragma once

#include <atomic>
#include <cstring> // for memcpy

#define MMM_MAGIC_RATIO 50u

namespace MMM{

	template<typename _T>
	class ConcurrentQueue_1{
	private:

		// clang-3.6 on ubuntu was not taking
		// std::atomic_uint32_t
		using atomic_uint32 = std::atomic<unsigned>;

		// array of data
		// data is pushed onto the back
		// data is popped from the from
		_T* data_;

		// maximum number of elements the
		// queue can currently have
		atomic_uint32 a_capacity_;

		// current number of elements
		// the queue has
		atomic_uint32 a_size_;

		// current index of the head data
		// starts from 0
		atomic_uint32 a_head_;

		// current index of the tail
		// place to put new elements
		atomic_uint32 a_tail_;

		// --- sentinel variables ---
		// variables used to sycronize
		// data access and manipution
		// from multiple threads

		// number of threads currently trying
		// to push data onto the queue
		atomic_uint32 a_pushing_;

		// number of threads currently trying
		// to pop data from the queue
		atomic_uint32 a_popping_;

		// true when the queue is resizing
		// the data array
		std::atomic_bool a_resizing_;

	public:

		// Default constructor, sets array size to 16
		ConcurrentQueue_1()
			:
			data_{ new _T[16u] },
			a_capacity_{ 16u },
			a_size_{ 0u },
			a_head_{ 0u },
			a_tail_{ 0u },
			a_pushing_{ 0u },
			a_popping_{ 0u }
		{

			a_resizing_.store(false);

		}

		// Constructor for user specified starting capacity
		ConcurrentQueue_1(unsigned initial_capacity)
			:
			data_{ new _T[initial_capacity] },
			a_capacity_{ initial_capacity },
			a_size_{ 0u },
			a_head_{ 0u },
			a_tail_{ 0u },
			a_pushing_{ 0u },
			a_popping_{ 0u }
		{

			a_resizing_.store(false);

		}

		// destructor
		~ConcurrentQueue_1(){

			// _TODO
			// wait for threads to finish popping data
			// prevent other threads from pushing or popping

			delete[] data_;
		}


		// Pushes Data on to the next available spot in the array
		// If there are no more spots, resize the array
		void PushBack(_T& data){

			// increment the counter for
			// the number of threads currently
			// adding elements onto the array
			a_pushing_.fetch_add(1u);

			// atomic increment the tail of the array
			// store the index of the tail before the incremenet
			auto tail = a_tail_.fetch_add(1u);

			// atomic load the maximum capacity of the array
			auto capacity = a_capacity_.load();

			// check if the array needs to be resized or updated
			if (tail >= capacity){

				// decrease the number of
				// threads currently trying
				// to add an element to the array
				a_pushing_.fetch_sub(1u);

				// !!! what if a thread comes through here and the resizing finishes?

				// if statement that singles out a thread
				// and uses it to resize the queue
				// redirects all other threads to a spin lock
				// and has them wait for the chosen thread to finish resizing
				auto expected_resizing = false;
				if (a_resizing_.compare_exchange_strong(expected_resizing, true)){
					
					// spin lock
					// waits for all threads to finish pushing
					auto expected_pushing = 0u;
					while (!a_pushing_.compare_exchange_weak(expected_pushing, 0u)){
						expected_pushing = 0u;
					}

					// increment the number of threads pushing
					// * the resizing thread will add its data after resizing
					a_pushing_.fetch_add(1u);

					// spin lock
					// waits for all threads to finish popping
					auto expected_popping = 0u;
					while (!a_popping_.compare_exchange_strong(expected_popping, 0u)){
						expected_popping = 0u;
					}

					// load the index of the head
					// * place where things get popped from
					auto head = a_head_.load();

					// integer percentage of how much of the array is used
					auto usage_ratio = 100u * head / capacity;

					// number of elements that have not already been popped off
					auto usable_elements = capacity - head;

					// ! Need to find a good ratio,
					// ! preferably one that is made from measurements
					
					// If statement to determing if there is more empty space
					// (part of the array from 0 to head that contains already popped elements)
					// If more than half of the array is empty space,
					// move the usable data of the array down to index 0
					// instead of resizing the whole array
					if (usage_ratio > MMM_MAGIC_RATIO){
						// more empty space (elements that have already been popped)
						// than used space if ratio is 50%.
						// No need to resize, just reset head and reduce size and tail

						// ! elements from data_ to data_ + head need to be destructed / deleted
						// ? use std::is_pointer

						// shift the usable elements down to the start of the array
						std::memcpy(data_, data_ + head, sizeof(_T) * usable_elements);

						// reset the head index back to 0
						a_head_.store(0u);

						// set the tail index to the
						// number of usable elements
						a_tail_.store(usable_elements);
					}
					else{
						// Need to expand the array

						// store pointer to current array of data
						auto old_array = data_;

						// allocate a new array
						// double the size of the current one
						// ! if the current capacity is not a multiple of 2,
						// ! we should find the next multiple of 2 greater than
						// ! the current capacity, and then use the double of that
						auto new_array = new _T[2u * capacity];

						// set the queue's data array to the new array
						data_ = new_array;

						// ! elements from data_[0] to data_[head] need to be destructed / deleted
						// ? use std::is_pointer

						// copy the usable elements from the old array to
						// the beginning of the new array
						std::memcpy(new_array, old_array + head, sizeof(_T) * usable_elements);
						
						// delete the old array
						delete[] old_array;

						// set the tail equal to
						// the capcity of the old array
						// * this is used to add the element
						// * that the thread originially intended too
						tail = capacity;

						// set the capcity to double the old array's cap
						a_capacity_.store(2u * capacity);

						// set the tail to the new capcity + 1
						// * +1 because this thread will add its data
						a_tail_.store(capacity + 1u);
					}

					// signal that resizing of the array is over
					a_resizing_.store(false);
				}
				else{

					// spin lock
					// makes the thread wait until the chosen thread
					// is done resizing the array
					auto expected_resizing = false;
					while (!a_resizing_.compare_exchange_weak(expected_resizing, false)){
						expected_resizing = false;
					}

					// Once the resizing is done, retry adding the element
					// ? there could be a case where it recurses until stack overflow
					// ? maybe create a loop to retry adding instead
					PushBack(data);
					return;
				}
			}
			
			// ! suspect for data races
			data_[tail] = data;

			// increment the size
			// (number of not-popped elements)
			a_size_.fetch_add(1u);

			// decrement placement sentinel
			a_pushing_.fetch_sub(1u);

		}

		// void Resize()

		// void PushBack(_T&& data)

		_T PopFront(){

			a_popping_.fetch_add(1u);

			auto swoop = true;

			// spin lock
			auto expected_resizing = false;
			while (!a_resizing_.compare_exchange_weak(expected_resizing, false)){
				if (swoop){
					a_popping_.fetch_sub(1u);
					swoop = false;
				}
				expected_resizing = false;
			}
			if (!swoop) a_popping_.fetch_add(1u);

			auto head = a_head_.fetch_add(1u);
			auto tail = a_tail_.load();

			// ! TODO: make sure size is greater than 0

			_T data = 0u;

			// ! data racer, zoom zoom
			data = data_[head];

			a_popping_.fetch_sub(1u);

			return data;
		}
		
		unsigned size(){ return a_size_.load(); }
		unsigned head(){ return a_head_.load(); }

	};

}

