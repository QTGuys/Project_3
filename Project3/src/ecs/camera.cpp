#include "camera.h"
#include <QtMath>
#include <QMatrix3x3>


Camera::Camera()
{
    // Camera position
    position = QVector3D(0.0, 2.0, 6.0);
}

QVector4D Camera::getLeftRightBottomTop()
{
    const float aspectRatio = float(viewportWidth) / viewportHeight;
    const float alpha = qDegreesToRadians(fovy * 0.5);
    const float top = znear * qTan(alpha);
    const float bottom = -top;
    const float right = top * aspectRatio;
    const float left = -right;
    QVector4D params(left, right, bottom, top);
    return params;
}

QVector3D Camera::screenPointToWorldRay(int x, int y)
{
    QVector4D lrbt = getLeftRightBottomTop();
    const float l = lrbt.x();
    const float r = lrbt.y();
    const float b = lrbt.z();
    const float t = lrbt.w();
    const float rayX = l + (r - l) * x / viewportWidth;
    const float rayY = b + (t - b) * (viewportHeight - y - 1) / viewportHeight;
    const float rayZ = -znear;
    QVector3D rayViewspace = QVector3D(rayX, rayY, rayZ);

    prepareMatrices();
    QVector3D rayWorldspace = QVector3D(worldMatrix * QVector4D(rayViewspace, 0.0));

    return rayWorldspace;
}

QVector3D Camera::screenDisplacementToWorldVector(int x0, int y0, int x1, int y1, const QVector3D &worldPoint)
{
    const QVector3D worldVectorZNear = screenPointToWorldRay(x1, y1) - screenPointToWorldRay(x0, y0);
    const QVector3D eyeWorldspace = QVector3D(worldMatrix * QVector4D(0.0, 0.0, 0.0, 1.0));
    const float distance = (worldPoint - eyeWorldspace).length();
    const QVector3D worldVector = distance * worldVectorZNear / znear;
    return worldVector;
}

QVector2D Camera::worldToScreenPoint(const QVector3D &pointWorld)
{
    QVector4D lrbt = getLeftRightBottomTop();
    const float l = lrbt.x();
    //const float r = lrbt.y();
    const float b = lrbt.z();
    const float t = lrbt.w();
    const QVector3D pointEye = viewMatrix * pointWorld;
    const QVector2D pointEyeZnear = znear * pointEye.toVector2D() / pointEye.z();
    const QVector2D pointViewport = (pointEyeZnear - QVector2D(l, b)) / QVector2D(t - l, t - b);
    const QVector2D pointScreen = pointViewport * QVector2D(viewportWidth, viewportHeight);
    return pointScreen;
}

void Camera::prepareMatrices()
{
    worldMatrix.setToIdentity();
    worldMatrix.translate(position);
    worldMatrix.rotate(yaw, QVector3D(0.0, 1.0, 0.0));
    worldMatrix.rotate(pitch, QVector3D(1.0, 0.0, 0.0));

    viewMatrix = worldMatrix.inverted();

    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(fovy, float(viewportWidth) / viewportHeight, znear, zfar);
}

void Camera::LookAt(QVector3D point)
{
//    QVector4D camFront= worldMatrix*QVector4D(0.0, 0.0, -1.0, 0.0);
//    QVector4D camRight = worldMatrix*QVector4D(1.0, 0.0, 0.0,0.0);
//    QVector4D camUp = worldMatrix*QVector4D(0.0,1.0,0.0,0.0);

//    QVector3D f = point - position;
//    QVector4D newFront = QVector4D(f,0.0);
//    newFront.normalize();

//    //New Right vector
//    QVector4D newRight = QVector4D(QVector3D::crossProduct(newFront.toVector3D(),camUp.toVector3D()),0.0);
//    newRight.normalize();

//    //New up vector
//    QVector4D newUp=QVector4D(QVector3D::crossProduct(newRight.toVector3D(),newFront.toVector3D()),0.0);
//    newUp.normalize();

//    float rightDiff = qRadiansToDegrees(qAcos(QVector4D::dotProduct(newRight,camRight)));
//    float upDiff = qRadiansToDegrees(qAcos(QVector4D::dotProduct(newUp,camUp)));

//    QVector3D crossYaw = QVector3D::crossProduct(newRight.toVector3D(),camRight.toVector3D());
//    crossYaw.normalize();
//    QVector3D crossPitch = QVector3D::crossProduct(newUp.toVector3D(),camUp.toVector3D());
//    crossPitch.normalize();

//    if(QVector3D::dotProduct(QVector3D(0.0,1.0,0.0),crossYaw)<0.0)
//    {
//        rightDiff=-rightDiff;
//    }

//    if(QVector3D::dotProduct(crossPitch,QVector3D(1.0,0.0,0.0))<0.0)
//    {
//        upDiff=-upDiff;
//    }

//    yaw-=rightDiff;
//    pitch-=upDiff;

    QVector3D direction = point - position;

       direction.normalize();

       pitch = qRadiansToDegrees(asin(direction.y()));

      yaw = -(qRadiansToDegrees(atan2(direction.x(),direction.z()))-180.0);


       if (yaw < 0.0f) yaw += 360.0f;
       if (yaw > 360.0f) yaw -= 360.0f;
       if (pitch > 89.0f) pitch = 89.0f;
       if (pitch < -89.0f) pitch = -89.0f;

       //printf("\nPitch:%f Yaw:%f",pitch,yaw);
}


