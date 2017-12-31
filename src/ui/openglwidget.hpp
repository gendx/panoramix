/*
    Panoramix - 3D view of your surroundings.
    Copyright (C) 2017  Guillaume Endignoux

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
*/

#ifndef GLWIDGET_HPP
#define GLWIDGET_HPP

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include "geometry/worldmodel.hpp"
#include "util/concurrency.hpp"

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    GLWidget(const std::shared_ptr<WorldModel>& worldModel, QWidget* parent = 0);

    void reload();
    void reloadEye();

private:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    void computeEye(const Point& tmp);
    void drawLabels(QPainter& painter, const QMatrix4x4& pmvMatrix, const QFontMetrics& metrics, int w, int h, int w_pixels, int h_pixels);
    void loadModelView();
    void loadProjection();

    QOpenGLShaderProgram mProgram;
    QMatrix4x4 mProjection;
    QMatrix4x4 mModelView;
    // Vertex buffers for OpenGL
    QOpenGLBuffer mBufferVertices;
    QOpenGLBuffer mBufferNormals;
    QOpenGLBuffer mBufferIndices;
    QOpenGLVertexArrayObject mVAO;
    unsigned int mPointCount;
    unsigned int mTriangleCount;
    unsigned int mTileCount;
    unsigned int mLabelCount;

    const std::shared_ptr<WorldModel>& mWorldModel;
    Point mEyeMercator;
    Point mEyeModel;
    bool mGroundKnown;
    bool mDetached;
    Point mRotate;
    double mSunRotate;
    double mZoom;
    bool mMousePress;
    QPoint mMousePos;

    std::vector<GLfloat> mDepthBuffer;
};

#endif // GLWIDGET_HPP
