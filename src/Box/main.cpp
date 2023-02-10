//
// Created by arrayJY on 2023/02/02.
//

#include "../app.h"
#include "box_renderer.h"

int main() {
  BoxRenderer renderer;
  App::MainLoop(&renderer, 800, 600);
}