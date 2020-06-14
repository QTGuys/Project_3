#include "deferredrenderer.h"
#include "gl.h"
#include "framebufferobject.h"

#include "resources/material.h"
#include "resources/mesh.h"
#include "resources/texture.h"
#include "resources/shaderprogram.h"
#include "resources/resourcemanager.h"
#include "resources/texture.h"

#include "globals.h"
#include "ecs/camera.h"


DeferredRenderer::DeferredRenderer()
{
    addTexture("Positions");
    addTexture("Normals");
    addTexture("Material");
    addTexture("Depth");
    addTexture("Final Deferred");
}

DeferredRenderer::~DeferredRenderer()
{

}

void DeferredRenderer::initialize()
{
    // Create programs

    deferredShading = resourceManager->createShaderProgram();
    deferredShading->name = "Deferred shading";
    deferredShading->vertexShaderFilename = "res/shaders/deferred_shading.vert";
    deferredShading->fragmentShaderFilename = "res/shaders/deferred_shading.frag";
    deferredShading->includeForSerialization = false;

    outlineProgram = resourceManager->createShaderProgram();
    outlineProgram->name = "Outline shading";
    outlineProgram->vertexShaderFilename = "res/shaders/outline_shading.vert";
    outlineProgram->fragmentShaderFilename = "res/shaders/outline_shading.frag";
    outlineProgram->includeForSerialization = false;

    waterClippingProgram=resourceManager->createShaderProgram();
    waterClippingProgram->name = "Water shading";
    waterClippingProgram->vertexShaderFilename = "res/shaders/water_clipping_render.vert";
    waterClippingProgram->fragmentShaderFilename = "res/shaders/water_clipping_render.frag";
    waterClippingProgram->includeForSerialization = false;

    waterRenderProgram=resourceManager->createShaderProgram();
    waterRenderProgram->name = "Water render";
    waterRenderProgram->vertexShaderFilename = "res/shaders/water_texture_shader.vert";
    waterRenderProgram->fragmentShaderFilename = "res/shaders/water_texture_shader.frag";
    waterRenderProgram->includeForSerialization = false;


    gBuffer = new FramebufferObject();
    gBuffer->create();

    fBuffer=new FramebufferObject();
    fBuffer->create();

    fOutline = new FramebufferObject();
    fOutline->create();

    fboReflection = new FramebufferObject();
    fboReflection->create();
    fboRefraction = new FramebufferObject();
    fboRefraction->create();

    CreateBuffers(camera->viewportWidth,camera->viewportHeight);

    blitProgram = resourceManager->createShaderProgram();
    blitProgram->name = "Blit";
    blitProgram->vertexShaderFilename = "res/shaders/blit.vert";
    blitProgram->fragmentShaderFilename = "res/shaders/blit.frag";
    blitProgram->includeForSerialization = false;

    deferredLightingShading = resourceManager->createShaderProgram();
    deferredLightingShading->name = "Deferred Lighting shading";
    deferredLightingShading->vertexShaderFilename = "res/shaders/deferred_lighting_shading.vert";
    deferredLightingShading->fragmentShaderFilename = "res/shaders/deferred_lighting_shading.frag";
    deferredLightingShading->includeForSerialization = false;

    backgroundProgram = resourceManager->createShaderProgram();
    backgroundProgram->name = "Background shading";
    backgroundProgram->vertexShaderFilename = "res/shaders/background_shading.vert";
    backgroundProgram->fragmentShaderFilename = "res/shaders/background_shading.frag";
    backgroundProgram->includeForSerialization = false;

    //Water textures
    normalMap = resourceManager->createTexture();
    dudvMap = resourceManager->createTexture();

    normalMap->loadTexture("res/WaterTextures/normalmap.png");
    dudvMap->loadTexture("res/WaterTextures/dudvmap.png");

    water = new Entity();
    water->name="Water";
    water->addComponent(ComponentType::MeshRenderer);
    water->meshRenderer->mesh=resourceManager->plane;
    water->transform->scale*=10.0f;
}

void DeferredRenderer::finalize()
{
    DeleteBuffers();
    normalMap->destroy();
    dudvMap->destroy();
}

void DeferredRenderer::resize(int width, int height)
{
    DeleteBuffers();
    CreateBuffers(width,height);

}
void DeferredRenderer::render(Camera *camera)
{
    OpenGLErrorGuard guard("DeferredRenderer::render()");

    if(scene->renderWater)
    {
        fboReflection->bind();
        Camera reflectionCam = *camera;
        reflectionCam.position.setY(-reflectionCam.position.y());
        reflectionCam.pitch=-reflectionCam.pitch;
        reflectionCam.viewportHeight=camera->viewportHeight;
        reflectionCam.viewportWidth=camera->viewportWidth;
        reflectionCam.prepareMatrices();

        passWaterScene(&reflectionCam,GL_COLOR_ATTACHMENT0,0);

        fboReflection->release();

        fboRefraction->bind();
        Camera refractionCam = *camera;
        refractionCam.position.setY(refractionCam.position.y());
        refractionCam.pitch=refractionCam.pitch;
        refractionCam.viewportHeight=camera->viewportHeight;
        refractionCam.viewportWidth=camera->viewportWidth;
        refractionCam.prepareMatrices();

        passWaterScene(&refractionCam,GL_COLOR_ATTACHMENT0,1);

        fboRefraction->release();
    }

    //-----------------Geometry Pass------------------//

    gBuffer->bind();
    gl->glEnable(GL_DEPTH_TEST);
    gl->glDepthMask(true);
    gl->glDisable(GL_BLEND);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Passes
    passMeshes(camera);
    gBuffer->release();


    //----------------------------------------------------//



    fOutline->bind();
    gl->glClear(GL_COLOR_BUFFER_BIT);
    if(selection->entities[0] != nullptr && selection->entities[0]->meshRenderer != nullptr)
    {
        passOutline(camera);
    }
    fOutline->release();

    //---------------------Shading Pass--------------//

    fBuffer->bind();
    //Lightning
    //depth mask deactivated
    //blending active and additive
    //culling deactivated for directional and depth activated
    //culling backface for point and depth deactivated
    gl->glClear(GL_COLOR_BUFFER_BIT);
    gl->glDepthMask(false);
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);
    gl->glBlendEquation(GL_FUNC_ADD);
    gl->glBlendFunc(GL_ONE, GL_ONE);
    passLightsToProgram();

    //Background and grid
    //depth test deactivated
    //blending active with transparency (glsrc alpha,gl_one minus src alpha)
    //culling deactivated
    gl->glDepthMask(true);
    //gl->glDisable(GL_DEPTH_TEST);
    gl->glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    gl->glDisable(GL_CULL_FACE);

    if(scene->renderWater)
    {
        passFinalWater();
    }

    passBackground(camera);

    fBuffer->release();

    //------------------------------------------------//
    gl->glDisable(GL_BLEND);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    passBlit();

}


void DeferredRenderer::CreateBuffers(int width, int height)
{
    OpenGLErrorGuard guard("Deferred:CreateBuffs");

    gBuffer->bind();

    glGenTextures(1,&tPosition);
    glBindTexture(GL_TEXTURE_2D,tPosition);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(0,tPosition,0);

    glGenTextures(1,&tNormal);
    glBindTexture(GL_TEXTURE_2D,tNormal);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB32F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(1,tNormal,0);

    glGenTextures(1,&tMaterial);
    glBindTexture(GL_TEXTURE_2D,tMaterial);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_FLOAT,NULL);
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
    gBuffer->release();

    fBuffer->bind();

    gl->glGenTextures(1, &fboColor);
    gl->glBindTexture(GL_TEXTURE_2D, fboColor);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    // Attach textures to the fbo

    fBuffer->bind();
    fBuffer->addColorAttachment(0, fboColor);
    fBuffer->addDepthAttachment(tDepth,0);
    fBuffer->checkStatus();
    fBuffer->release();

    fOutline->bind();

    glGenTextures(1,&tOutline);
    glBindTexture(GL_TEXTURE_2D,tOutline);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(0,tOutline,0);

    fOutline->checkStatus();
    fOutline->release();

    CreateWaterBuffers(width,height);
}

void DeferredRenderer::CreateWaterBuffers(int width, int height)
{
    glGenTextures(1,&rtReflection);
    glBindTexture(GL_TEXTURE_2D,rtReflection);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1,&rtRefraction);
    glBindTexture(GL_TEXTURE_2D,rtRefraction);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenTextures(1,&rtReflectionDepth);
    glBindTexture(GL_TEXTURE_2D,rtReflectionDepth);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glGenTextures(1,&rtRefractionDepth);
    glBindTexture(GL_TEXTURE_2D,rtRefractionDepth);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    fboReflection->bind();
    fboReflection->addColorAttachment(0, rtReflection);
    fboReflection->addDepthAttachment(rtReflectionDepth,0);
    fboReflection->checkStatus();
    fboReflection->release();

    fboRefraction->bind();
    fboRefraction->addColorAttachment(0, rtRefraction);
    fboRefraction->addDepthAttachment(rtRefractionDepth,0);
    fboRefraction->checkStatus();
    fboRefraction->release();

}

void DeferredRenderer::DeleteBuffers()
{
    glDeleteTextures(1,&tPosition);
    glDeleteTextures(1,&tNormal);
    glDeleteTextures(1,&tMaterial);
    glDeleteTextures(1,&tDepth);

    gl->glDeleteTextures(1, &fboColor);
}

void DeferredRenderer::passLightsToProgram()
{
    QOpenGLShaderProgram &program = deferredLightingShading->program;

    if (program.bind())
    {
        program.setUniformValue("gPosition",0);
        gl->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tPosition);

        program.setUniformValue("gNormal",1);
        gl->glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, tNormal);

        program.setUniformValue("gAlbedoSpec",2);
        gl->glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, tMaterial);

        program.setUniformValue("gBackground",3);
        gl->glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, tBackground);

        program.setUniformValue("viewPos",camera->position);
        program.setUniformValue("camViewPort", camera->viewportWidth,camera->viewportHeight);

         for (auto entity : scene->entities)
         {
             if (entity->active && entity->lightSource != nullptr)
             {
                 auto light = entity->lightSource;

                 program.setUniformValue("lightType", int(light->type));
                 program.setUniformValue("lightPosition", QVector3D(entity->transform->matrix() * QVector4D(0.0, 0.0, 0.0, 1.0)));
                 program.setUniformValue("lightDirection", QVector3D(entity->transform->matrix() * QVector4D(0.0, 1.0, 0.0, 0.0)));

                 QVector3D color(light->color.redF(), light->color.greenF(), light->color.blueF());
                 program.setUniformValue("lightColor", color * light->intensity);
                 program.setUniformValue("lightRange", light->range);

                 QMatrix4x4 scaleMatrix = QMatrix4x4();
                 QMatrix4x4 worldMatrix = entity->transform->matrix();

                 if(int(light->type)==0)
                 {
                    scaleMatrix.scale(light->range, light->range, light->range);
                    program.setUniformValue("viewMatrix", camera->viewMatrix);
                    program.setUniformValue("projectionMatrix", camera->projectionMatrix);
                    program.setUniformValue("worldMatrix", worldMatrix*scaleMatrix);
                   //program.setUniformValue("viewMatrix", QMatrix4x4());
                   //program.setUniformValue("projectionMatrix", QMatrix4x4());
                   //program.setUniformValue("worldMatrix", QMatrix4x4());
                 }
                 else
                 {
                     program.setUniformValue("viewMatrix", QMatrix4x4());
                     program.setUniformValue("projectionMatrix", QMatrix4x4());
                     program.setUniformValue("worldMatrix", QMatrix4x4());
                 }


                 if(int(light->type)==0)
                 {
                     glDisable(GL_DEPTH_TEST);
                     glEnable(GL_CULL_FACE);
                     glCullFace(GL_FRONT);

                     for (auto submesh : resourceManager->sphere->submeshes)
                     {
                         submesh->draw();
                     }
                     glCullFace(GL_BACK);
                     glDisable(GL_CULL_FACE);
                     glEnable(GL_DEPTH_TEST);
                 }
                 else
                 {
                     glDisable(GL_CULL_FACE);
                     resourceManager->quad->submeshes[0]->draw();
                     glEnable(GL_CULL_FACE);
                 }
             }
         }
     }
}


void DeferredRenderer::passMeshes(Camera *camera)
{
    QOpenGLShaderProgram &program = deferredShading->program;

    if (program.bind())
    {
        program.setUniformValue("viewMatrix", camera->viewMatrix);
        program.setUniformValue("projectionMatrix", camera->projectionMatrix);

        //sendLightsToProgram(program, camera->viewMatrix);

        QVector<MeshRenderer*> meshRenderers;
        QVector<LightSource*> lightSources;

        // Get components
        for (auto entity : scene->entities)
        {
            if (entity->active)
            {
                if (entity->meshRenderer != nullptr) { meshRenderers.push_back(entity->meshRenderer); }
                if (entity->lightSource != nullptr) { lightSources.push_back(entity->lightSource); }
            }
        }

        // Meshes
        for (auto meshRenderer : meshRenderers)
        {
            auto mesh = meshRenderer->mesh;

            if (mesh != nullptr)
            {
                QMatrix4x4 worldMatrix = meshRenderer->entity->transform->matrix();
                QMatrix4x4 worldViewMatrix = camera->viewMatrix * worldMatrix;
                QMatrix3x3 normalMatrix = worldMatrix.normalMatrix();

                program.setUniformValue("worldMatrix", worldMatrix);
                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                program.setUniformValue("normalMatrix", normalMatrix);
                program.setUniformValue("cameraPos", camera->position);

                int materialIndex = 0;
                for (auto submesh : mesh->submeshes)
                {
                    // Get material from the component
                    Material *material = nullptr;
                    if (materialIndex < meshRenderer->materials.size()) {
                        material = meshRenderer->materials[materialIndex];
                    }
                    if (material == nullptr) {
                        material = resourceManager->materialWhite;
                    }
                    materialIndex++;

#define SEND_TEXTURE(uniformName, tex1, tex2, texUnit) \
    program.setUniformValue(uniformName, texUnit); \
    if (tex1 != nullptr) { \
    tex1->bind(texUnit); \
                } else { \
    tex2->bind(texUnit); \
                }

                    // Send the material to the shader
                    program.setUniformValue("albedo", material->albedo);
                    program.setUniformValue("emissive", material->emissive);
                    program.setUniformValue("specular", material->specular);
                    program.setUniformValue("smoothness", material->smoothness);
                    program.setUniformValue("bumpiness", material->bumpiness);
                    program.setUniformValue("tiling", material->tiling);
                    SEND_TEXTURE("albedoTexture", material->albedoTexture, resourceManager->texWhite, 0);
                    SEND_TEXTURE("emissiveTexture", material->emissiveTexture, resourceManager->texBlack, 1);
                    SEND_TEXTURE("specularTexture", material->specularTexture, resourceManager->texBlack, 2);
                    SEND_TEXTURE("normalTexture", material->normalsTexture, resourceManager->texNormal, 3);
                    SEND_TEXTURE("bumpTexture", material->bumpTexture, resourceManager->texWhite, 4);

                    submesh->draw();
                }
            }
        }

        // Light spheres
        if (miscSettings->renderLightSources)
        {
            for (auto lightSource : lightSources)
            {
                QMatrix4x4 worldMatrix = lightSource->entity->transform->matrix();
                QMatrix4x4 scaleMatrix; scaleMatrix.scale(0.1f, 0.1f, 0.1f);
                QMatrix4x4 worldViewMatrix = camera->viewMatrix * worldMatrix * scaleMatrix;
                QMatrix3x3 normalMatrix = worldMatrix.normalMatrix();
                program.setUniformValue("worldMatrix", worldMatrix);
                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                program.setUniformValue("normalMatrix", normalMatrix);

                for (auto submesh : resourceManager->sphere->submeshes)
                {
                    // Send the material to the shader
                    Material *material = resourceManager->materialLight;
                    program.setUniformValue("albedo", material->albedo);
                    program.setUniformValue("emissive", material->emissive);
                    program.setUniformValue("smoothness", material->smoothness);

                    submesh->draw();
                }
            }
        }

        program.release();
    }
}

void DeferredRenderer::passBackground(Camera *camera)
{
    //GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT3};
    //gl->glDrawBuffers(1,drawBuffers);

    OpenGLState glState;
    glState.depthTest = true;
    glState.depthFunc = GL_LEQUAL;
    glState.apply();

    QOpenGLShaderProgram &program = backgroundProgram->program;

    if(program.bind())
    {
        QVector4D viewportParams = camera->getLeftRightBottomTop();
        program.setUniformValue("viewportSize",QVector2D(camera->viewportWidth,camera->viewportHeight));
        program.setUniformValue("left",viewportParams.x());
        program.setUniformValue("right",viewportParams.y());
        program.setUniformValue("bottom",viewportParams.z());
        program.setUniformValue("top",viewportParams.w());
        program.setUniformValue("znear",camera->znear);
        program.setUniformValue("zfar",camera->zfar);
        program.setUniformValue("worldMatrix",camera->worldMatrix);
        program.setUniformValue("viewMatrix",camera->viewMatrix);
        program.setUniformValue("projectionMatrix",camera->projectionMatrix);

        program.setUniformValue("backgroundColor", scene->backgroundColor);
        program.setUniformValue("grid",scene->renderGrid);

        program.setUniformValue("mask",0);
        gl->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tOutline);


        resourceManager->quad->submeshes[0]->draw();


        program.release();
    }
    glState.reset();
}

void DeferredRenderer::passOutline(Camera *camera)
{
    QOpenGLShaderProgram &program = outlineProgram->program;

    if(program.bind())
    {
        Entity* selec = selection->entities[0];
        QMatrix4x4 worldMatrix = selec->transform->matrix();

        QMatrix4x4 worldViewMatrix = camera->viewMatrix * worldMatrix;

        program.setUniformValue("worldViewMatrix", worldViewMatrix);
        program.setUniformValue("projectionMatrix", camera->projectionMatrix);


        for (auto submesh : selec->meshRenderer->mesh->submeshes)
        {
            submesh->draw();
        }

        program.release();
    }
}

void DeferredRenderer::passWaterScene(Camera *waterCam, uint colorAttachment, int mode)
{
    QOpenGLShaderProgram &program = waterClippingProgram->program;

    gl->glDrawBuffer(colorAttachment);

    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_CLIP_DISTANCE0);

    gl->glClearColor(0.0f,0.0f,0.0f,0.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(program.bind())
    {
        program.setUniformValue("viewMatrix", waterCam->viewMatrix);
        program.setUniformValue("projectionMatrix", waterCam->projectionMatrix);

        if(mode == 0)
        {
            program.setUniformValue("clippingPlane",QVector4D(0,1,0,0));
        }
        else
        {
            program.setUniformValue("clippingPlane",QVector4D(0,-1,0,0));
        }

        QVector<MeshRenderer*> meshRenderers;

        // Get components
        for (auto entity : scene->entities)
        {
            if (entity->active)
            {
                if (entity->meshRenderer != nullptr) { meshRenderers.push_back(entity->meshRenderer); }
            }
        }

        // Meshes
        for (auto meshRenderer : meshRenderers)
        {
            auto mesh = meshRenderer->mesh;

            if (mesh != nullptr)
            {
                QMatrix4x4 worldMatrix = meshRenderer->entity->transform->matrix();
                QMatrix4x4 worldViewMatrix = waterCam->viewMatrix * worldMatrix;
                QMatrix3x3 normalMatrix = worldMatrix.normalMatrix();


                program.setUniformValue("worldMatrix", worldMatrix);
                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                program.setUniformValue("normalMatrix", normalMatrix);
                program.setUniformValue("cameraPos", waterCam->position);

                int materialIndex = 0;
                for (auto submesh : mesh->submeshes)
                {
                    // Get material from the component
                    Material *material = nullptr;
                    if (materialIndex < meshRenderer->materials.size()) {
                        material = meshRenderer->materials[materialIndex];
                    }
                    if (material == nullptr) {
                        material = resourceManager->materialWhite;
                    }
                    materialIndex++;

#define SEND_TEXTURE(uniformName, tex1, tex2, texUnit) \
    program.setUniformValue(uniformName, texUnit); \
    if (tex1 != nullptr) { \
    tex1->bind(texUnit); \
                } else { \
    tex2->bind(texUnit); \
                }

                    // Send the material to the shader
                    //program.setUniformValue("albedo", material->albedo);
                    //program.setUniformValue("emissive", material->emissive);
                    //program.setUniformValue("specular", material->specular);
                    //program.setUniformValue("smoothness", material->smoothness);
                    //program.setUniformValue("bumpiness", material->bumpiness);
                    //program.setUniformValue("tiling", material->tiling);
                    SEND_TEXTURE("albedoTexture", material->albedoTexture, resourceManager->texWhite, 0);
                    //SEND_TEXTURE("emissiveTexture", material->emissiveTexture, resourceManager->texBlack, 1);
                    //SEND_TEXTURE("specularTexture", material->specularTexture, resourceManager->texBlack, 2);
                    //SEND_TEXTURE("normalTexture", material->normalsTexture, resourceManager->texNormal, 3);
                    //SEND_TEXTURE("bumpTexture", material->bumpTexture, resourceManager->texWhite, 4);

                    submesh->draw();
                }
            }
        }

        program.release();
    }

    gl->glDisable(GL_CLIP_DISTANCE0);
}

void DeferredRenderer::passFinalWater()
{
    QOpenGLShaderProgram &program = waterRenderProgram->program;

    if(program.bind())
    {
        program.setUniformValue("viewportSize",QVector2D(camera->viewportWidth,camera->viewportHeight));
        program.setUniformValue("projectionMatrix", camera->projectionMatrix);
        program.setUniformValue("worldMatrix",water->transform->matrix());
        program.setUniformValue("worldViewMatrix",(camera->viewMatrix*water->transform->matrix()));
        program.setUniformValue("viewMatrixInv",camera->viewMatrix.inverted());
        program.setUniformValue("projectionMatrixInv",camera->projectionMatrix.inverted());

        program.setUniformValue("reflectionMap",0);
        gl->glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, rtReflection);

        program.setUniformValue("refractionMap",1);
        gl->glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, rtRefraction);

        program.setUniformValue("reflectionDepth",2);
        gl->glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, rtReflectionDepth);

        program.setUniformValue("refractionDepth",3);
        gl->glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, rtRefractionDepth);

        program.setUniformValue("normalMap",4);
        gl->glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, normalMap->textureId());

        program.setUniformValue("dudvMap",5);
        gl->glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, dudvMap->textureId());

        water->meshRenderer->mesh->submeshes[0]->draw();
        program.release();
    }

}

void DeferredRenderer::passBlit()
{
       gl->glDisable(GL_DEPTH_TEST);

        QOpenGLShaderProgram &program = blitProgram->program;

        if (program.bind())
        {
            program.setUniformValue("colorTexture", 0);
            program.setUniformValue("camHeight", camera->position.y());
            program.setUniformValue("water", scene->renderWater);

            gl->glActiveTexture(GL_TEXTURE0);

            if (shownTexture() == "Positions")
            {
                gl->glBindTexture(GL_TEXTURE_2D, tPosition);
            }
            else if(shownTexture() == "Normals")
            {
                 gl->glBindTexture(GL_TEXTURE_2D, tNormal);
            }
            else if(shownTexture() == "Material")
            {
                 gl->glBindTexture(GL_TEXTURE_2D, rtReflection);
            }
            else if(shownTexture() == "Depth")
            {
                 gl->glBindTexture(GL_TEXTURE_2D, normalMap->textureId());
            }
            else if(shownTexture() == "Final Deferred")
            {
                gl->glBindTexture(GL_TEXTURE_2D, fboColor);
            }

            resourceManager->quad->submeshes[0]->draw();
        }



        gl->glEnable(GL_DEPTH_TEST);

}
