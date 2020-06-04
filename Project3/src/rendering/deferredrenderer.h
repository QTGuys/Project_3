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
    void passBackground(Camera* camera);
    void passOutline(Camera* camera);

public:

    FramebufferObject* gBuffer = nullptr;
    FramebufferObject* fBuffer = nullptr;
    FramebufferObject* fOutline = nullptr;

    uint tPosition = 0;
    uint tNormal = 0;
    uint tMaterial = 0;
    uint tDepth = 0;
    uint tBackground = 0;
    uint tOutline = 0;

   uint fboColor = 0;

private:
    ShaderProgram *deferredShading = nullptr;
    ShaderProgram* deferredLightingShading = nullptr;
    ShaderProgram *blitProgram = nullptr;
    ShaderProgram* backgroundProgram = nullptr;
    ShaderProgram* outlineProgram = nullptr;

};

#endif // DEFERREDRENDERER_H
