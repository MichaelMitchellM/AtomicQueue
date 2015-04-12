
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>

#include "atomic_queue.hpp"

#define SIZE 1000000u

int main(){

	for (auto tests = 0u; tests < 100u; ++tests){
		std::vector<std::thread> threads;

		auto aq = MMM::AtomicQueue<unsigned>();

		std::atomic_uint32_t counter{ 0u };
		auto count = 0u;

		auto func =
			[&]()
		{
			for (auto i = 0u; i < SIZE; ++i){
				count = counter.fetch_add(1u);
				aq.PushBack(count);
			}
		};

		for (auto i = 0u; i < 8u; ++i){
			threads.push_back(std::thread{ func });
		}

		for (auto i = 0u; i < 8u; ++i){
			threads[i].join();
		}

		for (auto i = 0u; i < 50; ++i){
			//printf("%u : %u\n", i, aq.PopFront());
		}
		
		printf("test # %u\n", tests);
	}

	std::cin.ignore(1000, '\n');
	return 0;
}
