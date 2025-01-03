#pragma once
#include "texture.h"

class FrameBuffer {
private:
    OpenGLContext *glContext;
    GLuint m_frameBuffer;
    Texture m_outputTexture;
    GLuint m_depthRenderBuffer;

    unsigned int m_width, m_height;
    float m_devicePixelRatio;
    bool m_created;

    unsigned int m_textureSlot;

public:
    FrameBuffer(OpenGLContext *context,
                unsigned int width, unsigned int height,
                float devicePixelRatio);
    // Make sure to call resize from MyGL::resizeGL
    // to keep your frame buffer up to date with
    // your screen dimensions
    void resize(unsigned int width, unsigned int height,
                float devicePixelRatio);
    // Initialize all GPU-side data required
    void create(bool depth = false);
    // Deallocate all GPU-side data
    void destroy();
    void bindFrameBuffer();
    // Associate our output texture with the indicated texture slot
    void bindToTextureSlot(unsigned int slot);
    unsigned int getTextureSlot() const;
};
