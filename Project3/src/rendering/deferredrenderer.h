#ifndef DEFERREDRENDERER_H
#define DEFERREDRENDERER_H

#include "renderer.h"
#include "gl.h"

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
    void initializeBloom();

    void resize(int width, int height) override;
    void render(Camera *camera) override;
    void renderBloom(Camera* camera);
    void passBlightBrightPixels(FramebufferObject* fbo, const QVector2D& viewportSize,GLenum colorAttachment,uint inputTexture,int inputLod);
    void passBlur(FramebufferObject* pfbo, const QVector2D &viewportSize, GLenum colorAttachment, uint inputTexture, int inputLod, const QVector2D &direction);
    void passBloom(FramebufferObject* fbo, GLenum colorAttachment,uint inputTexture, int maxLod);

    void CreateBuffers(int width, int height);
    void CreateWaterBuffers(int width, int height);
    void CreateBuffersBloom(int width, int height);
    void DeleteBuffers();

    void passBlit();

private:

    void passLightsToProgram();
    void passMeshes(Camera* camera);
    void passBackground(Camera* camera);
    void passOutline(Camera* camera);
    void passWaterScene(Camera* waterCam,uint colorAttachment, int mode);
    void passFinalWater();
    void clearBloomBuffers();

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
    
    //------------Blur/Bloom thingies--------------------//
    ShaderProgram *blitBrightestPixels = nullptr;
    ShaderProgram *blur = nullptr;
    ShaderProgram* bloomProgram = nullptr;
    
    uint rtBright;
    uint rtBloomH;
    FramebufferObject* fboBloom1 = nullptr;
    FramebufferObject* fboBloom2 = nullptr;
    FramebufferObject* fboBloom3 = nullptr;
    FramebufferObject* fboBloom4 = nullptr;
    FramebufferObject* fboBloom5 = nullptr;
    
    uint mipmap_level = 4;
};

#endif // DEFERREDRENDERER_H
