#include "deferredrenderer.h"
#include "gl.h"
#include "framebufferobject.h"

#include "resources/material.h"
#include "resources/mesh.h"
#include "resources/texture.h"
#include "resources/shaderprogram.h"
#include "resources/resourcemanager.h"

#include "globals.h"
#include "ecs/camera.h"


DeferredRenderer::DeferredRenderer()
{
    addTexture("Positions");
    addTexture("Normals");
    addTexture("Material");
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

    CreateGBuffer(camera->viewportWidth,camera->viewportHeight);

    blitProgram = resourceManager->createShaderProgram();
    blitProgram->name = "Blit";
    blitProgram->vertexShaderFilename = "res/shaders/blit.vert";
    blitProgram->fragmentShaderFilename = "res/shaders/blit.frag";
    blitProgram->includeForSerialization = false;


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
    gBuffer->bind();

    // Clear color
    gl->glClearDepth(1.0);
    gl->glClearColor(miscSettings->backgroundColor.redF(),
                     miscSettings->backgroundColor.greenF(),
                     miscSettings->backgroundColor.blueF(),
                     1.0);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Passes
    passMeshes(camera);

    gBuffer->release();

    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    passBlit();
}

void DeferredRenderer::CreateGBuffer(int width, int height)
{
    gBuffer = new FramebufferObject();
    gBuffer->create();
    gBuffer->bind();

    glGenTextures(1,&tPosition);
    glBindTexture(GL_TEXTURE_2D,tPosition);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(0,tPosition,0);

    glGenTextures(1,&tNormal);
    glBindTexture(GL_TEXTURE_2D,tNormal);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB16F,width,height,0,GL_RGB,GL_FLOAT,NULL);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gBuffer->addColorAttachment(1,tNormal,0);

    glGenTextures(1,&tMaterial);
    glBindTexture(GL_TEXTURE_2D,tMaterial);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,width,height,0,GL_RGBA,GL_UNSIGNED_BYTE,NULL);
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
                QMatrix3x3 normalMatrix = worldViewMatrix.normalMatrix();


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
                QMatrix3x3 normalMatrix = worldViewMatrix.normalMatrix();
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

void DeferredRenderer::passBlit()
{
    gl->glDisable(GL_DEPTH_TEST);

    QOpenGLShaderProgram &program = blitProgram->program;

    if (program.bind())
    {
        program.setUniformValue("colorTexture", 0);
        gl->glActiveTexture(GL_TEXTURE0);

        if (shownTexture() == "Positions") {
            gl->glBindTexture(GL_TEXTURE_2D, tPosition);
        }  else if(shownTexture() == "Normals"){
             gl->glBindTexture(GL_TEXTURE_2D, tNormal);
        } else if(shownTexture() == "Material"){
             gl->glBindTexture(GL_TEXTURE_2D, tMaterial);
        } else
              gl->glBindTexture(GL_TEXTURE_2D, resourceManager->texWhite->textureId());



        resourceManager->quad->submeshes[0]->draw();
    }

    gl->glEnable(GL_DEPTH_TEST);
}
