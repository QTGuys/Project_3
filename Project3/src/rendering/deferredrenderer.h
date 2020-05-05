#ifndef DEFERREDRENDERER_H
#define DEFERREDRENDERER_H

#include "renderer.h"

class FramebufferObject;
class ShaderProgram;
class QOpenGLShaderProgram;

class DeferredRenderer : public Renderer
{
public:

    DeferredRenderer();
    ~DeferredRenderer() override;

    void initialize() override;
    void finalize() override;

    void resize(int width, int height) override;
    void render(Camera *camera) override;

    void CreateBuffers(int width, int height);
    void DeleteBuffers();

    void passBlit();
private:
    void passLightsToProgram();
    void passMeshes(Camera* camera);

public:

    FramebufferObject* gBuffer = nullptr;
    FramebufferObject* fBuffer = nullptr;

    uint tPosition = 0;
    uint tNormal = 0;
    uint tMaterial = 0;
    uint tDepth = 0;

   uint fboColor = 0;
   uint fboDepth = 0;

private:
    ShaderProgram *deferredShading = nullptr;
    ShaderProgram* deferredLightingShading = nullptr;
    ShaderProgram *blitProgram = nullptr;

};

#endif // DEFERREDRENDERER_H
