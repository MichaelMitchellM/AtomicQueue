
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include "concurrent_queue_1.hpp"

#define NUM_THREADS 8u
#define SIZE 1000000u

#define QUEUE MMM::ConcurrentQueue_1

int main(){

	for (auto tests = 0u; tests < 100u; ++tests){
		std::vector<std::thread> threads;

		auto aq = QUEUE<unsigned>();

		std::atomic_uint32_t counter{ 0u };
		auto count = 0u;

		srand(std::chrono::system_clock::now().time_since_epoch().count());

		aq.PopFront();

		auto func =
			[&]()
		{
			for (auto i = 0u; i < SIZE; ++i){
				if (rand() % 29u){
					count = counter.fetch_add(1u);
					aq.PushBack(count);
				}
				else{
					aq.PopFront();
					//printf("%u\n", aq.PopFront());
				}
			}
		};

		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads.push_back(std::thread{ func });
		}

		for (auto i = 0u; i < NUM_THREADS; ++i){
			threads[i].join();
		}

		/*
		for (auto i = 0u; i < 50u; ++i){
			printf("%u : %u\n", i, aq.PopFront());
		}
		*/

		printf("test # %u\n", tests);
	}

	//std::cin.ignore(1000, '\n');
	return 0;
}
