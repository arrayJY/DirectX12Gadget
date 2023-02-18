//
// Created by arrayJY on 2023/02/10.
//

#include "app.h"

#define GLFW_EXPOSE_NATIVE_WIN32
#include "renderer.h"
#include <glfw/glfw3.h>
#include <glfw/glfw3native.h>
#include <stdexcept>

void App::MainLoop(Renderer *renderer, unsigned width, unsigned height) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  auto window =
      glfwCreateWindow(width, height, "DirectX12Gadget", nullptr, nullptr);
  if (!window) {
    throw std::runtime_error("Creat window error");
  }
  glfwSetFramebufferSizeCallback(window, Renderer::OnResizeFrame);

  glfwSetKeyCallback(window, Renderer::OnKeyboardInput);
  glfwSetMouseButtonCallback(window, Renderer::OnMouseButtonInput);
  glfwSetCursorPosCallback(window, Renderer::OnMousePostionInput);

  auto process_keystrokes_input = [](GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, 1);
  };

  auto hwnd = glfwGetWin32Window(window);

  renderer->InitDirectX(
      Renderer::InitInfo{.width = width, .height = height, .hwnd = hwnd});

  while (!glfwWindowShouldClose(window)) {
    process_keystrokes_input(window);
    renderer->DrawFrame();
    glfwPollEvents();
  }
  glfwTerminate();
}