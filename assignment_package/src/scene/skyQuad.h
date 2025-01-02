#pragma once

#include "drawable.h"
#include <glm_includes.h>

class SkyQuad : public Drawable {
public:
    SkyQuad(OpenGLContext* context);

    void create();
    void createVBOdata() override;
    GLenum drawMode() override { return GL_TRIANGLES; }

private:
    void generateUVBuffer();
};
