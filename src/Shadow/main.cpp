//
// Created by arrayJY on 2023/03/27.
//


#include "../app.h"
#include "shadow_renderer.h"

int main() {
    ShadowRenderer renderer;
    App::MainLoop(&renderer, 1920, 1080);
}