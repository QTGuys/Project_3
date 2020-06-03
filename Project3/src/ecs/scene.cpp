#include "scene.h"
#include "globals.h"
#include "resources/mesh.h"
#include "resources/texture.h"
#include "resources/material.h"
#include "globals.h"
#include <QJsonArray>


// Scene //////////////////////////////////////////////////////////////////

Scene::Scene()
{
    backgroundColor= QVector4D(.02,0.0,0.37,0.0);
}

Scene::~Scene()
{
    for (auto entity : entities)
    {
        delete entity;
    }
}

int Scene::numEntities() const
{
    return entities.size();
}

int Scene::IndexFromEntity(Entity* entity)
{
    for(int i =0; i<entities.size();++i)
    {
        if(entities[i]==entity)
            return i;
    }
    return -1;
}

Entity *Scene::addEntity()
{
    Entity *entity = new Entity;
    entities.push_back(entity);
    return entity;
}

Entity *Scene::entityAt(int index)
{
    return entities[index];
}

void Scene::removeEntityAt(int index)
{
    delete entities[index];
    entities.removeAt(index);
}

Component *Scene::findComponent(ComponentType ctype)
{
   for (auto entity : entities) {
       auto comp = entity->findComponent(ctype);
       if (comp != nullptr) return comp;
   }
   return nullptr;
}

void Scene::clear()
{
    for (auto entity : entities)
    {
        delete entity;
    }
    entities.clear();
}

void Scene::handleResourcesAboutToDie()
{
    for (auto entity : entities)
    {
        if (entity->meshRenderer)
        {
            entity->meshRenderer->handleResourcesAboutToDie();
        }
    }
}

void Scene::read(const QJsonObject &json)
{
}

void Scene::write(QJsonObject &json)
{
}

