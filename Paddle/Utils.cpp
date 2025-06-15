#include "Utils.hpp"

void Utils::DebugLog(const std::string& message) {
#ifdef _DEBUG
	std::cout << "[Debug] " << message << std::endl;
#endif
}
