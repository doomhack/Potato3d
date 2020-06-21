#include <QtCore>

#include "common.h"
#include "object3d.h"


Object3d::Object3d()
{
    render = nullptr;
}

Object3d::Object3d(Render* render)
{
    this->render = render;
}

bool Object3d::Setup(unsigned int screenWidth, unsigned int screenHeight, fp hFov, fp zNear, fp zFar, pixel* framebuffer, fp* zBuffer)
{
    this->render = new Render();

    render->Setup(screenWidth, screenHeight, hFov, zNear, zFar, framebuffer, zBuffer);
}

