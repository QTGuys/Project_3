#ifndef DEFERREDRENDERER_H
#define DEFERREDRENDERER_H

#include "renderer.h"

class FramebufferObject;
class ShaderProgram;

class DeferredRenderer : public Renderer
{
public:

    DeferredRenderer();
    ~DeferredRenderer() override;

    void initialize() override;
    void finalize() override;

    void resize(int width, int height) override;
    void render(Camera *camera) override;

    void CreateGBuffer(int width, int height);
    void DeleteGBuffer();

    void passBlit();
private:

    void passMeshes(Camera* camera);

public:

    FramebufferObject* gBuffer;

    uint tPosition = 0;
    uint tNormal = 0;
    uint tMaterial = 0;
    uint tDepth = 0;

private:
    ShaderProgram *deferredShading = nullptr;
    ShaderProgram *blitProgram;

};

#endif // DEFERREDRENDERER_H
