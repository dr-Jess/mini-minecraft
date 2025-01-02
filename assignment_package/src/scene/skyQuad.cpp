#include "skyQuad.h"
#include "iostream"

SkyQuad::SkyQuad(OpenGLContext* context) : Drawable(context) {}

void SkyQuad::create() {
    // indices for two triangles in quad
    GLuint idx[6] = {0, 1, 2, 0, 2, 3};

    //vertex positions in ndc
    glm::vec4 vert_pos[4] {glm::vec4(-1.f, -1.f, 1.f, 1.f),
                          glm::vec4(1.f, -1.f, 1.f, 1.f),
                          glm::vec4(1.f, 1.f, 1.f, 1.f),
                          glm::vec4(-1.f, 1.f, 1.f, 1.f)};

    indexCounts[SKY_INDEX] = 6;

    std::vector<glm::vec4> vec;

    for (int i = 0; i < 4; i++) {
        vec.push_back(vert_pos[i]);
    }

    generateBuffer(SKY_INDEX);
    if (bindBuffer(SKY_INDEX)) {
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    }

    //took out uv (not interleaved anymore)
    generateBuffer(SKY_INTERLEAVED);
    if (bindBuffer(SKY_INTERLEAVED)) {
        mp_context->glBufferData(GL_ARRAY_BUFFER, vec.size() * sizeof(glm::vec4), vec.data(), GL_STATIC_DRAW);
    }
}

void SkyQuad::createVBOdata() {
    create();
}
