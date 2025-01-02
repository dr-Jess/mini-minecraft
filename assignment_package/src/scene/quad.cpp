#include "quad.h"

Quad::~Quad()
{}

void Quad::createVBOdata(){
    GLuint idx[6] = {0, 1, 2, 0, 2, 3};
    glm::vec4 pos[4] = {glm::vec4(-1,-1,1,1), glm::vec4(1,-1,1,1),
                        glm::vec4(1,1,1,1), glm::vec4(-1,1,1,1)};
    glm::vec4 norm[4] = {glm::vec4(0,0,1,0), glm::vec4(0,0,1,0),
                        glm::vec4(0,0,1,0), glm::vec4(0,0,1,0)};
    glm::vec4 uv[4] = {glm::vec4(0, 0, 0, 0), glm::vec4(1, 0, 0, 0),
                        glm::vec4(1, 1, 0, 0), glm::vec4(0, 1, 0, 0)};
    std::vector<glm::vec4> opq_interleaved;
    for (int i = 0; i < 4; i++){
        opq_interleaved.push_back(pos[i]);
        opq_interleaved.push_back(norm[i]);
        opq_interleaved.push_back(uv[i]);
    }

    generateBuffer(OPQ_INTERLEAVED);
    generateBuffer(OPQ_INDEX);

    if (bindBuffer(OPQ_INTERLEAVED)) {
        mp_context->glBufferData(GL_ARRAY_BUFFER, opq_interleaved.size() * sizeof(glm::vec4), opq_interleaved.data(), GL_STATIC_DRAW);
    }

    if (bindBuffer(OPQ_INDEX)) {
        mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), idx, GL_STATIC_DRAW);
    }

    indexCounts[OPQ_INDEX] = 6;
    indexCounts[OPQ_INTERLEAVED] = 12;
}

GLenum Quad::drawMode(){
    return GL_TRIANGLES;
}
