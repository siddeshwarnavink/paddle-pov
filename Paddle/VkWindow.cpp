#include "VkWindow.hpp"

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
}