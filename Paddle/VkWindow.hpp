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

	private:
		GLFWwindow* window;
		const int width;
		const int height;
		std::string title;

		void InitWindow();
	};
}