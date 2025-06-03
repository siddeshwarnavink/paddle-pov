#include "VkWindow.hpp"

#include <stdexcept>

namespace Vk {
	Window::Window(int width, int height, std::string title) : width(width), height(height), title(title) {
		InitWindow();
	}

	Window::~Window() {
		if (window) {
			glfwDestroyWindow(window);
			glfwTerminate();
		}
	}

	void Window::InitWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	}

	void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to craete window surface");
		}
	}
}