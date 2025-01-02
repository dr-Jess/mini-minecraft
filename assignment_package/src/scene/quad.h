#pragma once

#include "drawable.h"
#include <glm_includes.h>

class Quad : public Drawable
{
public:
    Quad(OpenGLContext* context) : Drawable(context){}
    virtual ~Quad() override;
    void createVBOdata() override;
    GLenum drawMode() override;
};
