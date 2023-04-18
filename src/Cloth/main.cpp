#include "../app.h"
#include "cloth_renderer.h"

int main() {
    ClothRenderer renderer;
    App::MainLoop(&renderer, 1920, 1080);
}