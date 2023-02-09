//
// Created by arrayJY on 2023/02/02.
//

#include "stdafx.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "box_renderer.h"
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <stdexcept>

static BoxRenderer renderer;
static void OnResizeFrame(GLFWwindow *window, int width, int height) {
  renderer.OnResize(width, height);
}

int main() {
  constexpr auto width = 800, height = 600;
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto window =
      glfwCreateWindow(width, height, "DirectX12Gaget", nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("Creat window error");
  }
  glfwSetFramebufferSizeCallback(window, OnResizeFrame);

  auto process_keystrokes_input = [](GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);
  };

  auto hwnd = glfwGetWin32Window(window);

  renderer.InitDirectX(
      Renderer::InitInfo{.width = width, .height = height, .hwnd = hwnd});

  OnResizeFrame(window, width, height);

  while (!glfwWindowShouldClose(window)) {
    process_keystrokes_input(window);
    renderer.DrawFrame();
    glfwPollEvents();
  }
  glfwTerminate();

  return 0;
}