//
// Created by arrayJY on 2023/02/16.
//
#include "../app.h"
#include "pbr_renderer.h"

int main() {
  PBRRenderer renderer;
  App::MainLoop(&renderer, 1920, 1080);
}