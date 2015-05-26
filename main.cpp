
#include <atomic>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include <boost/lockfree/queue.hpp>
#include <concurrent_queue.h>

#include "concurrent_queue_1.hpp"
#include "concurrent_queue_2.hpp"

#define NUM_THREADS 8u
#define NUM_RUNS 15u
#define SIZE 1000000u

template<typename _T> using conqueue = MMM::ConcurrentQueue_2<_T>;
template<typename _T> using testqueue = Concurrency::concurrent_queue<_T>;

int main(){

	std::chrono::system_clock::time_point start;
	std::chrono::system_clock::time_point end;

	for (auto tests = 0u; tests < NUM_RUNS; ++tests){
		std::vector<std::thread> threads;
		conqueue<unsigned> aq;
		std::atomic<unsigned> counter{ 0u };
		auto count = 0u;
		srand((unsigned)std::chrono::system_clock::now().time_since_epoch().count());

		auto func =
			[&]()
		{
			for (auto i = 0u; i < SIZE; ++i){
				count = counter.fetch_add(1u);
				aq.PushBack(count);

				/*if (rand() % 29u){
					count = counter.fetch_add(1u);
					aq.PushBack(count);
				}
				else{
					aq.PopFront();
					//printf("%u\n", aq.PopFront());
				}*/
			}
		};

		start = std::chrono::high_resolution_clock::now();
		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads.push_back(std::thread{ func });
		}

		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads[i].join();
		}
		end = std::chrono::high_resolution_clock::now();

		auto time_took = end - start;

		printf("test # %u %ums\n", tests, std::chrono::duration_cast<std::chrono::milliseconds>(time_took));
	}

	/*
	for (auto tests = 0u; tests < NUM_RUNS; ++tests){
		std::vector<std::thread> threads;
		testqueue<unsigned> queue;
		std::atomic<unsigned> counter{ 0u };
		auto count = 0u;
		srand((unsigned)std::chrono::system_clock::now().time_since_epoch().count());

		auto func =
			[&]()
		{
			for (auto i = 0u; i < SIZE; ++i){
				count = counter.fetch_add(1u);
				queue.push(count);
			}
		};

		start = std::chrono::high_resolution_clock::now();
		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads.push_back(std::thread{ func });
		}

		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads[i].join();
		}
		end = std::chrono::high_resolution_clock::now();

		auto time_took = end - start;

		printf("test # %u %ums\n", tests, std::chrono::duration_cast<std::chrono::milliseconds>(time_took));
	}
	*/

	std::cin.ignore(1000, '\n');
	return 0;
}
