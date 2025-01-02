#include "framebuffer.h"
#include <iostream>

FrameBuffer::FrameBuffer(OpenGLContext *context,
                         unsigned int width, unsigned int height,
                         float devicePixelRatio)
    : glContext(context), m_frameBuffer(-1),
    m_outputTexture(context), m_depthRenderBuffer(-1),
    m_width(width), m_height(height),
    m_devicePixelRatio(devicePixelRatio), m_created(false)
{}

void FrameBuffer::resize(unsigned int width, unsigned int height,
                         float devicePixelRatio) {
    m_width = width;
    m_height = height;
    m_devicePixelRatio = devicePixelRatio;
}

void FrameBuffer::create(bool depth) {
    // Initialize the frame buffers and render textures
    glContext->glGenFramebuffers(1, &m_frameBuffer);
    if (!depth) {
        glContext->glGenRenderbuffers(1, &m_depthRenderBuffer);
    }
    if (depth) {
        m_outputTexture.create(nullptr, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT);
    } else {
        m_outputTexture.create(nullptr, GL_RGB, GL_RGB);
    }

    glContext->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    // Set up our texture to an empty image with a width and
    // height equal to our own.
    if (depth) {
        m_outputTexture.bufferPixelData(m_width * m_devicePixelRatio,
                                        m_height * m_devicePixelRatio,
                                        GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, nullptr);
    } else {
        m_outputTexture.bufferPixelData(m_width * m_devicePixelRatio,
                                        m_height * m_devicePixelRatio,
                                        GL_RGBA, GL_BGRA, nullptr);
    }

    // Initialize our depth buffer
    if (!depth) {
        glContext->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
        glContext->glRenderbufferStorage(GL_RENDERBUFFER,
                                         GL_DEPTH_COMPONENT,
                                         m_width * m_devicePixelRatio,
                                         m_height * m_devicePixelRatio);
        glContext->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);
    }

    // Set m_renderedTexture as the color output of our frame buffer
    if (depth) {
        glContext->glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_outputTexture.getHandle(), 0);
    } else {
        glContext->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_outputTexture.getHandle(), 0);
    }
    // Sets the color output of the fragment shader to be stored in GL_COLOR_ATTACHMENT0,
    // which we previously set to m_renderedTexture
    GLenum drawBuffers[1];
    if (depth) {
        drawBuffers[0] = GL_NONE;
    } else {
        drawBuffers[0] = GL_COLOR_ATTACHMENT0;
    }
    glContext->glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

    m_created = true;
    if(glContext->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        m_created = false;
        std::cout << "Frame buffer did not initialize correctly..." << std::endl;
    }
}

void FrameBuffer::destroy() {
    if(m_created) {
        m_created = false;
        glContext->glDeleteFramebuffers(1, &m_frameBuffer);
        glContext->glDeleteRenderbuffers(1, &m_depthRenderBuffer);
        m_outputTexture.destroy();
    }
}

void FrameBuffer::bindFrameBuffer() {
    glContext->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
}

void FrameBuffer::bindToTextureSlot(unsigned int slot) {
    m_textureSlot = slot;
    m_outputTexture.bind(slot);
}

unsigned int FrameBuffer::getTextureSlot() const {
    return m_textureSlot;
}
