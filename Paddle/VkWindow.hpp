#pragma once

#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Vk
{
	class Window
	{

	public:
		Window(int width, int height, std::string title);
		~Window();
		bool ShouldClose() { return glfwWindowShouldClose(window); }
		void PollEvents() { glfwPollEvents(); }
		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		VkExtent2D getExtent() {
			return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
		}
		bool IsKeyPressed(int key) const;
		GLFWwindow *GetWindow() { return window; }
		void UpdateFramebufferSize();
		void Close() { glfwSetWindowShouldClose(window, true); }

	private:
		GLFWwindow* window;
		int width;
		int height;
		std::string title;

		void InitWindow();
	};
}
