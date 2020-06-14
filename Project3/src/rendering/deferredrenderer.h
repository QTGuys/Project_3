#ifndef DEFERREDRENDERER_H
#define DEFERREDRENDERER_H

#include "renderer.h"

class FramebufferObject;
class ShaderProgram;
class QOpenGLShaderProgram;
class Texture;
class Entity;

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
    void CreateWaterBuffers(int width, int height);
    void DeleteBuffers();

    void passBlit();

private:

    void passLightsToProgram();
    void passMeshes(Camera* camera);
    void passBackground(Camera* camera);
    void passOutline(Camera* camera);
    void passWaterScene(Camera* waterCam,uint colorAttachment, int mode);
    void passFinalWater();

public:

    FramebufferObject* gBuffer = nullptr;
    FramebufferObject* fBuffer = nullptr;
    FramebufferObject* fOutline = nullptr;

    FramebufferObject* fboReflection = nullptr;
    FramebufferObject* fboRefraction = nullptr;

    uint tPosition = 0;
    uint tNormal = 0;
    uint tMaterial = 0;
    uint tDepth = 0;
    uint tBackground = 0;
    uint tOutline = 0;

    uint rtReflection=0;
    uint rtRefraction=0;
    uint rtReflectionDepth=0;
    uint rtRefractionDepth=0;

    uint fboColor = 0;

    Texture* normalMap;
    Texture* dudvMap;

    Entity* water=nullptr;

private:

    ShaderProgram *deferredShading = nullptr;
    ShaderProgram* deferredLightingShading = nullptr;
    ShaderProgram *blitProgram = nullptr;
    ShaderProgram* backgroundProgram = nullptr;
    ShaderProgram* outlineProgram = nullptr;
    ShaderProgram* waterClippingProgram=nullptr;
    ShaderProgram* waterRenderProgram=nullptr;


};

#endif // DEFERREDRENDERER_H
