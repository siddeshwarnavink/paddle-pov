#pragma once

#include <iostream>
#include <vector>

namespace Utils {
	// This cursed syntax is to avoid double deletion
    template<typename T>
    void DestroyPtr(T*& ptr) {
        delete ptr;
        ptr = nullptr;
    }

    template<typename T>
    void DestroyPtrs(std::vector<T*>& vec) {
        for(size_t i = 0; i < vec.size(); ++i) {
            delete vec[i];
            vec[i] = nullptr;
        }
    }

    template<typename T, std::size_t N>
    void DestroyPtrs(std::array<T*, N>& arr) {
        for(size_t i = 0; i < N; ++i) {
            delete arr[i];
            arr[i] = nullptr;
        }
    }

	void DebugLog(const std::string& message);
}
