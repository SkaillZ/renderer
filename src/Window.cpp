#include "Window.hpp"

Window::Window(int width, int height)
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

glm::vec2 Window::getMousePosition()
{
	double x, y;
	glfwGetCursorPos(window, &x, &y);

	return glm::vec2(static_cast<float>(x), static_cast<float>(y));
}

void Window::enterFullscreen()
{
	glfwGetWindowPos(window, &windowedX, &windowedY);
	glfwGetWindowSize(window, &windowedWidth, &windowedHeight);

	auto mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
}

void Window::exitFullscreen() const
{
	glfwSetWindowMonitor(window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, GLFW_DONT_CARE);
}
