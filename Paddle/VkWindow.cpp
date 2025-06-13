#include "VkWindow.hpp"

#include <stdexcept>

namespace Vk {
	constexpr int WINDOW_X = 100;
	constexpr int WINDOW_Y = 100;

	constexpr int DESIGNED_WIDTH = 1920;
	constexpr int DESIGNED_HEIGHT = 1080;
	constexpr float DESIGNED_ASPECT = float(DESIGNED_WIDTH) / float(DESIGNED_HEIGHT);

	Window::Window(int width, int height, std::string title) : width(width), height(height), title(title) {
		isFullscreen = true;
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
		
		GLFWmonitor* mon = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(mon);
		glfwSetWindowMonitor(window, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
	}

	void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to craete window surface");
		}
	}

	bool Window::IsKeyPressed(int key) const {
		return glfwGetKey(window, key) == GLFW_PRESS;
	}

	void Window::ToggleFullscreen() {
		GLFWmonitor* mon = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(mon);

		if (isFullscreen) {
			glfwSetWindowMonitor(window, nullptr, WINDOW_X, WINDOW_Y, width, height, 0);
			isFullscreen = false;
		}
		else {
			glfwSetWindowMonitor(window, mon, 0, 0, mode->width, mode->height, mode->refreshRate);
			isFullscreen = true;
		}
	}

	
	void Window::SetAspectViewport(VkCommandBuffer cmd, int fbWidth, int fbHeight) {
		float fbAspect = float(fbWidth) / float(fbHeight);
		int vpWidth, vpHeight, vpX, vpY;
		if (fbAspect > DESIGNED_ASPECT) {
			vpHeight = fbHeight;
			vpWidth = int(DESIGNED_ASPECT * fbHeight);
			vpX = (fbWidth - vpWidth) / 2;
			vpY = 0;
		}
		else {
			vpWidth = fbWidth;
			vpHeight = int(fbWidth / DESIGNED_ASPECT);
			vpX = 0;
			vpY = (fbHeight - vpHeight) / 2;
		}

		VkViewport viewport{};
		viewport.x = float(vpX);
		viewport.y = float(vpY);
		viewport.width = float(vpWidth);
		viewport.height = float(vpHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		glfwGetFramebufferSize(window, &width, &height);
	}
}
