#ifndef SCENE_H
#define SCENE_H

#include <QVector>
#include <QJsonObject>

class Entity;
class Component;

#include "entity.h"

class Scene
{
public:
    Scene();
    ~Scene();

    int numEntities() const;
    int IndexFromEntity(Entity* entity);
    Entity *addEntity();
    Entity *entityAt(int index);
    void removeEntityAt(int index);

    Component *findComponent(ComponentType ctype);

    void clear();

    void handleResourcesAboutToDie();

    void read(const QJsonObject &json);
    void write(QJsonObject &json);

    QVector<Entity*> entities;
    QVector4D backgroundColor;
    Entity* selectedEntity = nullptr;
};


#endif // SCENE_H
