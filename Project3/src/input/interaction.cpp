#include "interaction.h"
#include "globals.h"
#include "resources/mesh.h"
#include <QtMath>
#include <QVector2D>
#include <QOpenGLShaderProgram>
#include "rendering/framebufferobject.h"
#include "rendering/gl.h"
#include "resources/shaderprogram.h"


void Interaction::initialize()
{
    selection_fbo = new FramebufferObject();
    selection_fbo->create();

   selection_shader = resourceManager->createShaderProgram();
   selection_shader->name = "Selection shading";
   selection_shader->vertexShaderFilename = "res/shaders/selection_shading.vert";
   selection_shader->fragmentShaderFilename = "res/shaders/selection_shading.frag";
   selection_shader->includeForSerialization = false;
}

bool Interaction::update()
{
    bool changed = false;

    switch (state)
    {
    case State::Idle:
        changed = idle();
        break;

    case State::Navigating:
        changed = navigate();
        break;

    case State::Focusing:
        changed = focus();
        break;
    }

    return changed;
}

bool Interaction::idle()
{
    if (input->mouseButtons[Qt::RightButton] == MouseButtonState::Pressed)
    {
        nextState = State::Navigating;
    }
    else if (input->mouseButtons[Qt::LeftButton] == MouseButtonState::Press)
    {
        // TODO: Left click
        want_to_mousepick = true;
        emit selection->onClick();
    }
    else if(selection->count > 0)
    {
        if (input->keys[Qt::Key_F] == KeyState::Press)
        {
            nextState = State::Focusing;
        }
    }

    return false;
}

bool Interaction::navigate()
{
    static float v = 0.0f; // Instant speed
    static const float a = 5.0f; // Constant acceleration
    static const float t = 1.0/60.0f; // Delta time

    bool pollEvents = input->mouseButtons[Qt::RightButton] == MouseButtonState::Pressed;
    bool cameraChanged = false;

    // Mouse delta smoothing
    static float mousex_delta_prev[3] = {};
    static float mousey_delta_prev[3] = {};
    static int curr_mousex_delta_prev = 0;
    static int curr_mousey_delta_prev = 0;
    float mousey_delta = 0.0f;
    float mousex_delta = 0.0f;
    if (pollEvents) {
        mousex_delta_prev[curr_mousex_delta_prev] = (input->mousex - input->mousex_prev);
        mousey_delta_prev[curr_mousey_delta_prev] = (input->mousey - input->mousey_prev);
        curr_mousex_delta_prev = curr_mousex_delta_prev % 3;
        curr_mousey_delta_prev = curr_mousey_delta_prev % 3;
        mousex_delta += mousex_delta_prev[0] * 0.33;
        mousex_delta += mousex_delta_prev[1] * 0.33;
        mousex_delta += mousex_delta_prev[2] * 0.33;
        mousey_delta += mousey_delta_prev[0] * 0.33;
        mousey_delta += mousey_delta_prev[1] * 0.33;
        mousey_delta += mousey_delta_prev[2] * 0.33;
    }

    float &yaw = camera->yaw;
    float &pitch = camera->pitch;

    // Camera navigation
    if (mousex_delta != 0 || mousey_delta != 0)
    {
        cameraChanged = true;
        yaw -= 0.5f * mousex_delta;
        pitch -= 0.5f * mousey_delta;
        while (yaw < 0.0f) yaw += 360.0f;
        while (yaw > 360.0f) yaw -= 360.0f;
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    static QVector3D speedVector;
    speedVector *= 0.99;

    bool accelerating = false;
    if (input->keys[Qt::Key_W] == KeyState::Pressed) // Front
    {
        accelerating = true;
        speedVector += QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                        sinf(qDegreesToRadians(pitch)),
                                        -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch))) * a * t;
    }
    if (input->keys[Qt::Key_A] == KeyState::Pressed) // Left
    {
        accelerating = true;
        speedVector -= QVector3D(cosf(qDegreesToRadians(yaw)),
                                        0.0f,
                                        -sinf(qDegreesToRadians(yaw))) * a * t;
    }
    if (input->keys[Qt::Key_S] == KeyState::Pressed) // Back
    {
        accelerating = true;
        speedVector -= QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                        sinf(qDegreesToRadians(pitch)),
                                        -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch))) * a * t;
    }
    if (input->keys[Qt::Key_D] == KeyState::Pressed) // Right
    {
        accelerating = true;
        speedVector += QVector3D(cosf(qDegreesToRadians(yaw)),
                                        0.0f,
                                        -sinf(qDegreesToRadians(yaw))) * a * t;
    }
    if (input->keys[Qt::Key_Q] == KeyState::Pressed) // Up
    {
        accelerating = true;
        QVector3D front=QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                  sinf(qDegreesToRadians(pitch)),
                                  -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)));

        QVector3D right = QVector3D(cosf(qDegreesToRadians(yaw)),
                                    0.0f,
                                    -sinf(qDegreesToRadians(yaw)));

        QVector3D up = QVector3D::crossProduct(right,front)*a*t;

        speedVector -= up;
    }
    if (input->keys[Qt::Key_E] == KeyState::Pressed) // Down
    {
        accelerating = true;
        QVector3D front=QVector3D(-sinf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)),
                                  sinf(qDegreesToRadians(pitch)),
                                  -cosf(qDegreesToRadians(yaw)) * cosf(qDegreesToRadians(pitch)));

        QVector3D right = QVector3D(cosf(qDegreesToRadians(yaw)),
                                    0.0f,
                                    -sinf(qDegreesToRadians(yaw)));

        QVector3D up = QVector3D::crossProduct(right,front)*a*t;

        speedVector += up;
    }

    if (!accelerating) {
        speedVector *= 0.9;
    }

    camera->position += speedVector * t;

    if (!(pollEvents ||
        speedVector.length() > 0.01f||
        qAbs(mousex_delta) > 0.1f ||
        qAbs(mousey_delta) > 0.1f))
    {
        nextState = State::Idle;
    }

    return true;
}

bool Interaction::focus()
{
    static bool idle = true;
    static float time = 0.0;
    static QVector3D initialCameraPosition;
    static QVector3D finalCameraPosition;
    if (idle) {
        idle = false;
        time = 0.0f;
        initialCameraPosition = camera->position;

        Entity *entity = selection->entities[0];

        float entityRadius = 0.5;
        if (entity->meshRenderer != nullptr && entity->meshRenderer->mesh != nullptr)
        {
            auto mesh = entity->meshRenderer->mesh;
            const QVector3D minBounds = entity->transform->matrix() * mesh->bounds.min;
            const QVector3D maxBounds = entity->transform->matrix() * mesh->bounds.max;
            entityRadius = (maxBounds - minBounds).length();
        }

        QVector3D entityPosition = entity->transform->position;
        QVector3D viewingDirection = QVector3D(camera->worldMatrix * QVector4D(0.0, 0.0, -1.0, 0.0));
        QVector3D displacement = - 1.5 * entityRadius * viewingDirection.normalized();
        finalCameraPosition = entityPosition + displacement;
    }

    const float focusDuration = 0.5f;
    time = qMin(focusDuration, time + 1.0f/60.0f); // TODO: Use frame delta time
    const float t = qPow(qSin(3.14159f * 0.5f * time / focusDuration), 0.5);

    camera->position = (1.0f - t) * initialCameraPosition + t * finalCameraPosition;

    if (t == 1.0f) {
        nextState = State::Idle;
        idle = true;;
    }

    return true;
}

void Interaction::RenderSelection()
{
    OpenGLErrorGuard guard("Interatcion::RenderSelection()");
    selection_fbo->bind();

    // Clear color
    gl->glClearDepth(1.0);
    gl->glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Passes
    QOpenGLShaderProgram &program = selection_shader->program;

    if (program.bind())
    {
        program.setUniformValue("viewMatrix", camera->viewMatrix);
        program.setUniformValue("projectionMatrix", camera->projectionMatrix);

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

                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                float test = (float)meshRenderer->entity->selection_code;
                program.setUniformValue("SelectionCode",(float)meshRenderer->entity->selection_code);
                for (auto submesh : mesh->submeshes)
                {
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

                program.setUniformValue("worldViewMatrix", worldViewMatrix);
                program.setUniformValue("SelectionColor",(float)lightSource->entity->selection_code);

                for (auto submesh : resourceManager->sphere->submeshes)
                {
                    submesh->draw();
                }
            }
        }

        program.release();
    }

    selection_fbo->release();
}

void Interaction::SelectFromRender()
{
    selection_fbo->bind();

    GLfloat* pixels = (GLfloat*)malloc(sizeof(GLfloat)*3);
    glReadPixels(input->mousex, camera->viewportHeight-input->mousey,1,1,GL_RGB,GL_FLOAT,pixels);
    selection_fbo->release();

    for(auto item:scene->entities)
    {
        float sub = qAbs(item->selection_code - pixels[0]);

        if( sub < 0.01)
        {
            selection->select(item);
        }
    }
}

void Interaction::postUpdate()
{
    state = nextState;
}

void Interaction::resize(int w, int h)
{
    OpenGLErrorGuard guard("Interaction::resize()");

    gl->glGenTextures(1, &selection_texture);
    gl->glBindTexture(GL_TEXTURE_2D, selection_texture);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, w, h, 0, GL_RGB, GL_FLOAT, nullptr);

    if (selection_depth == 0) gl->glDeleteTextures(1, &selection_depth);
    gl->glGenTextures(1, &selection_depth);
    gl->glBindTexture(GL_TEXTURE_2D, selection_depth);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    selection_fbo->bind();
    selection_fbo->addColorAttachment(0, selection_texture);
    selection_fbo->addDepthAttachment(selection_depth);
    selection_fbo->checkStatus();
    selection_fbo->release();
}
