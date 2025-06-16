#include "Utils.hpp"

#include <iostream>
#include <random>

void Utils::DebugLog(const std::string& message) {
#ifdef _DEBUG
	std::cout << "[Debug] " << message << std::endl;
#endif
}

bool Utils::RandomChance(float prob) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::bernoulli_distribution dis(prob);
	return dis(gen);
}

int Utils::RandomNumber(int start, int end) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(start, end);
	return dis(gen);
}
