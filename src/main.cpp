#include "stdafx.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <stdexcept>

int main() {
  constexpr int width = 800, height = 600;

  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto window =
      glfwCreateWindow(width, height, "DirectX 12 Demo", nullptr, nullptr);

  if(!window) {
    throw std::runtime_error("GLFW create window failed.");
  }

  auto hwnd = glfwGetWin32Window(window);
}
