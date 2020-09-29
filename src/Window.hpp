#pragma once
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>


class Window
{
public:
	Window(int width, int height);
	virtual ~Window();

	GLFWwindow* getNativeWindowPointer() const { return window; }
	bool shouldClose() const { return glfwWindowShouldClose(window); }
	bool isFullscreen() const { return glfwGetWindowMonitor(window) != nullptr; }
	int getKey(int key) const { return glfwGetKey(window, key); }

	bool isMouseLocked() const { return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED; }
	void setMouseLocked(bool locked) const { glfwSetInputMode(window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL); }
	glm::vec2 getMousePosition();

	void close() const { glfwSetWindowShouldClose(window, true); }

	void enterFullscreen();
	void exitFullscreen() const;

private:
	GLFWwindow* window = nullptr;
	int windowedX = 0, windowedY = 0, windowedWidth = 0, windowedHeight = 0;
};

