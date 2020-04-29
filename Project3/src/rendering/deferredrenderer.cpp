#include "deferredrenderer.h"
#include "gl.h"
#include "framebufferobject.h"
#include "globals.h"
#include "ecs/camera.h"


DeferredRenderer::DeferredRenderer()
{

}

DeferredRenderer::~DeferredRenderer()
{

}

void DeferredRenderer::initialize()
{
    CreateGBuffer(camera->viewportWidth,camera->viewportHeight);
}

void DeferredRenderer::finalize()
{
    DeleteGBuffer();
}

void DeferredRenderer::resize(int width, int height)
{
    DeleteGBuffer();
    CreateGBuffer(width,height);

}
void DeferredRenderer::render(Camera *camera)
{

}

void DeferredRenderer::CreateGBuffer(int width, int height)
{
    gBuffer = new FramebufferObject();
    gBuffer->create();
    gBuffer->bind();

    glGenTextures(1,&tPosition);
    glBindTexture(GL_TEXTURE_2D,tPosition);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(0,tPosition,0);

    glGenTextures(1,&tNormal);
    glBindTexture(GL_TEXTURE_2D,tNormal);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(1,tNormal,0);

    glGenTextures(1,&tMaterial);
    glBindTexture(GL_TEXTURE_2D,tMaterial);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(2,tMaterial,0);

    glGenTextures(1, &tDepth);
    glBindTexture(GL_TEXTURE_2D, tDepth);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,width,height,0, GL_DEPTH_COMPONENT, GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addDepthAttachment(tDepth,0);

    uint attachments[3]={GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2};
    gl->glDrawBuffers(3,attachments);

    gBuffer->checkStatus();
}

void DeferredRenderer::DeleteGBuffer()
{
    glDeleteTextures(1,&tPosition);
    glDeleteTextures(1,&tNormal);
    glDeleteTextures(1,&tMaterial);
    glDeleteTextures(1,&tDepth);

    gBuffer->destroy();
}
