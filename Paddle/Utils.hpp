#pragma once

#include <iostream>
#include <vector>

namespace Utils {
	template<typename T>
	void DestroyPtrs(std::vector<T*>& vec) {
		for (size_t i = 0; i < vec.size(); ++i) {
			delete vec[i];
			vec[i] = nullptr;
		}
	}

	template<typename T, std::size_t N>
	void DestroyPtrs(std::array<T*, N>& arr) {
		for (size_t i = 0; i < N; ++i) {
			delete arr[i];
			arr[i] = nullptr;
		}
	}

	void DebugLog(const std::string& message) {
#ifdef _DEBUG
		std::cout << "[Debug] " << message << std::endl;
#endif
	}
}
