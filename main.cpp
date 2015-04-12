
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <thread>
#include <vector>

#include "atomic_queue.hpp"


int main(){

	std::vector<std::thread> threads;

	auto aq = MMM::AtomicQueue<unsigned>();

	auto count = 5u;

	auto func = 
		[&]()
	{
		for (auto i = 0u; i < 10000; ++i){
			aq.PushBack(count);
		}
	};

	for (auto i = 0u; i < 8u; ++i){
		threads.push_back(std::thread{ func });
	}

	for (auto i = 0u; i < 8u; ++i){
		threads[i].join();
	}

	for (auto i = 0u; i < 20; ++i){
		printf("%u : %u\n", i, aq.PopFront());
	}


	std::cin.ignore(1000, '\n');
	return 0;
}
