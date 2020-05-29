#ifndef INTERACTION_H
#define INTERACTION_H

#include "qglobal.h"

class FramebufferObject;
class ShaderProgram;
class Entity;

class Interaction
{
public:

    void entityClicked(Entity* entity);

    void initialize();
    bool update();

    void postUpdate();

    void resize(int w, int h);
    void RenderSelection();

    void SelectFromRender();

    bool want_to_mousepick =false;
    uint selection_texture = 0;

private:

    bool idle();
    bool navigate();
    bool focus();
    bool orbit();

    enum State { Idle, Navigating, Focusing, Orbit };
    State state = State::Idle;
    State nextState = State::Idle;

    FramebufferObject* selection_fbo;

    uint selection_depth = 0;
    ShaderProgram* selection_shader;

};

#endif // INTERACTION
