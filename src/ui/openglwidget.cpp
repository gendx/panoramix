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

#include "openglwidget.hpp"

#include <cmath>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QResource>
#include <QPainter>
#include "config.hpp"
#include "geometry/astro.hpp"

GLWidget::GLWidget(const std::shared_ptr<WorldModel>& worldModel, QWidget* parent) :
    QOpenGLWidget(parent),
    mBufferVertices(QOpenGLBuffer::VertexBuffer),
    mBufferNormals(QOpenGLBuffer::VertexBuffer),
    mBufferIndices(QOpenGLBuffer::IndexBuffer),
    mPointCount(0),
    mTriangleCount(0),
    mTileCount(0),
    mLabelCount(0),
    mWorldModel(worldModel),
    mEyeMercator(0, 0, 0),
    mEyeModel(0, 0, 0),
    mGroundKnown(false),
    mDetached(false),
    mRotate(180, 0, 0), // Towards South
    mSunRotate(270), // South
    mZoom(0),
    mMousePress(false)
{
    this->setMinimumSize(200, 200);
    this->setCursor(Qt::OpenHandCursor);
    this->setFocusPolicy(Qt::StrongFocus);
}

void GLWidget::reload()
{
    std::cerr << "GLWidget::reload() started" << std::endl;

    std::shared_ptr<WorldModel::Mesh> mesh;
    mWorldModel->mesh().swap(mesh);

    // Another call to reload() already processed this mesh.
    if (!mesh)
    {
        std::cerr << "GLWidget::reload() cancelled" << std::endl;
        return;
    }

    mPointCount = mesh->pointCount;
    mTriangleCount = mesh->triangleCount;
    auto& vertices = mesh->vertices;
    auto& normals = mesh->normals;
    auto& indices = mesh->indices;
    mTileCount = mesh->tileCount;
    mLabelCount = mesh->labelCount;

    std::cerr << "GLWidget::reload() with " << mPointCount << " points" << std::endl;

    // Update VBOs
    mBufferVertices.bind();
    mBufferVertices.allocate(vertices.data(), sizeof(GLfloat)*vertices.size());
    mBufferVertices.release();

    mBufferNormals.bind();
    mBufferNormals.allocate(normals.data(), sizeof(GLfloat)*normals.size());
    mBufferNormals.release();

    mBufferIndices.bind();
    mBufferIndices.allocate(indices.data(), sizeof(GLuint)*indices.size());
    mBufferIndices.release();

    std::cerr << "GLWidget::reload() finished" << std::endl;
}

void GLWidget::reloadEye()
{
    Point pt = mWorldModel->selection().get();
    pt.z = mEyeMercator.z;
    computeEye(pt);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();

    std::cerr << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_NORMALIZE);
    glClearColor(1, 1, 1, 1);

    // Shaders
#ifdef USE_EARTH_CURVATURE
    mProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader_sphere.vert");
#else
    mProgram.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shaders/shader_flat.vert");
#endif
    std::cerr << "Vertex shader compile output: " << mProgram.log().toStdString() << std::endl;
    mProgram.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/shader.frag");
    std::cerr << "Fragment shader compile output: " << mProgram.log().toStdString() << std::endl;
    mProgram.link();
    std::cerr << "Shader linking output: " << mProgram.log().toStdString() << std::endl;

    // Vertex buffer objects
    mBufferVertices.create();
    mBufferVertices.bind();
    mBufferVertices.setUsagePattern(QOpenGLBuffer::StaticDraw);
    mBufferVertices.allocate(nullptr, 0);
    mBufferVertices.release();

    mBufferNormals.create();
    mBufferNormals.bind();
    mBufferNormals.setUsagePattern(QOpenGLBuffer::StaticDraw);
    mBufferNormals.allocate(nullptr, 0);
    mBufferNormals.release();

    mBufferIndices.create();
    mBufferIndices.bind();
    mBufferIndices.setUsagePattern(QOpenGLBuffer::StaticDraw);
    mBufferIndices.allocate(nullptr, 0);
    mBufferIndices.release();

    // Vertex array object
    mVAO.create();
    mProgram.bind();

    int vertexLocation = mProgram.attributeLocation("vertex");
    int normalLocation = mProgram.attributeLocation("normal");

    mVAO.bind();
        mBufferVertices.bind();
        mProgram.enableAttributeArray(vertexLocation);
#ifdef USE_EARTH_CURVATURE
        mProgram.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 4);
#else
        mProgram.setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3);
#endif
        mBufferVertices.release();

        mBufferNormals.bind();
        mProgram.enableAttributeArray(normalLocation);
        mProgram.setAttributeBuffer(normalLocation, GL_FLOAT, 0, 3);
        mBufferNormals.release();
    mVAO.release();

    mProgram.disableAttributeArray(vertexLocation);
    mProgram.disableAttributeArray(normalLocation);
    mProgram.release();
}

void GLWidget::paintGL()
{
    /* constexpr */ double pi = std::atan(1)*4;

    QPainter painter;
    painter.begin(this);
    painter.beginNativePainting();


    /** 3D scene **/
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    this->loadModelView();

    QMatrix4x4 pmvMatrix = mProjection * mModelView;

    double sunTheta = mSunRotate * pi / 180.0;
    Point light(std::cos(sunTheta), std::sin(sunTheta), 1);
    light.normalize3();

    int matrixLocation = mProgram.uniformLocation("matrix");
    int lightLocation = mProgram.uniformLocation("lightOrigin");

    mProgram.bind();
    mProgram.setUniformValue(matrixLocation, pmvMatrix);
    mProgram.setUniformValue(lightLocation, light.x, light.y, light.z, 0);
    mVAO.bind();
    mBufferIndices.bind();

    glDrawElements(GL_TRIANGLES, 3*mTriangleCount, GL_UNSIGNED_INT, nullptr);

    mBufferIndices.release();
    mVAO.release();
    mProgram.release();

    // Read depth buffer
    int w = width();
    int h = height();
    int w_pixels = RETINA_FACTOR*w;
    int h_pixels = RETINA_FACTOR*h;
    mDepthBuffer.resize(w_pixels*h_pixels);
    glReadPixels(0, 0, w_pixels, h_pixels, GL_DEPTH_COMPONENT, GL_FLOAT, mDepthBuffer.data());


    /** 2D graphics **/
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    painter.endNativePainting();

    QFont font("Georgia", 15);
    QFontMetrics metrics(font);

    painter.setFont(font);
    painter.setPen(Qt::black);

    // Labels
    this->drawLabels(painter, pmvMatrix, metrics, w, h, w_pixels, h_pixels);

    // Status bar
    Point selection = mWorldModel->selection().get();
    double lat = Astro::mercatorToLatDeg(selection);
    double lon = Astro::mercatorToLonDeg(selection);

    QString status = "GPS coordinates " + QString::number(lat) + ", " + QString::number(lon)
            + " | view altitude " + QString::number(mEyeMercator.z + VIEWER_HEIGHT) + " m"
    ;
    if (mGroundKnown)
        status += " | ground altitude " + QString::number(selection.z) + " m";

    QString statistics = QString::number(mTileCount) + " tiles"
            + " | " + QString::number(mLabelCount) + " labels in area"
            + " | " + QString::number(mPointCount) + " vertices"
            + " | " + QString::number(mTriangleCount) + " triangles"
    ;

    int ypadding = 1*RETINA_FACTOR;
    int yrect = h - ypadding - 2*metrics.height();
    painter.fillRect(0, yrect, w, h - yrect, QColor(0xFF, 0xFF, 0xFF, 0xC0));
    painter.drawLine(0, yrect, w, yrect);
    int ytext = yrect + metrics.height() - metrics.descent();
    painter.drawText(0, ytext, status);
    painter.drawText(0, ytext + metrics.height(), statistics);

    painter.drawText(0, yrect, w, metrics.height(), Qt::AlignRight, QString::fromUtf8("Data \xC2\xA9 Mapbox \xC2\xA9 OpenStreetMap"));
}

void GLWidget::drawLabels(QPainter& painter, const QMatrix4x4& pmvMatrix, const QFontMetrics& metrics, int w, int h, int w_pixels, int h_pixels)
{
    std::vector<QRect> acceptedLabels;

    std::shared_ptr<std::vector<Label>> labels = mWorldModel->visibleLabels().get();
    for (auto&& l : *labels)
    {
        // Find point on screen
        QVector3D proj = pmvMatrix.map(QVector3D(l.point.x, l.point.y, l.point.z));
        if (proj.x() < -1.0 || proj.x() > 1.0 || proj.y() < -1.0 || proj.y() >= 1.0)
            continue;
        int x = (0.5 + 0.5*proj.x()) * w;
        int y = (0.5 - 0.5*proj.y()) * h;

        // Find depth test, PEAKS_DEPTH_HEIGHT meters above peak ground
        QVector3D projDepth = pmvMatrix.map(QVector3D(l.point.x, l.point.y, l.point.z + PEAKS_DEPTH_HEIGHT));
        if (projDepth.x() < -1.0 || projDepth.x() > 1.0 || projDepth.y() < -1.0)
            continue;
        int xdepth = (0.5 + 0.5*projDepth.x()) * w_pixels;

        // Adjust to screen
        int ydepth = projDepth.y() <= 1.0
                ? (0.5 + 0.5*projDepth.y()) * h_pixels
                : h_pixels;
        int y_pixels = (0.5 + 0.5*proj.y()) * h_pixels;
        if (ydepth >= h_pixels && y_pixels < h_pixels)
            ydepth = h_pixels - 1;

        if (xdepth < 0 || xdepth >= w_pixels || ydepth < 0 || ydepth >= h_pixels)
            continue;

        // Read depth buffer and convert back to [-1, 1] range for OpenGL consistency
        float depth = mDepthBuffer[xdepth + ydepth*w_pixels];
        depth = 2*depth - 1;

        if (projDepth.z() <= 1.0 && projDepth.z() <= depth)
        {
            // Compute bounding rectangle
            QString name = QString::fromUtf8(l.name.c_str());
            if (l.hasElevation)
                name += "\n" + QString::number(l.elevation) + " m";
            double distance = std::round(std::sqrt((l.point - mEyeModel).dist2()) / 100.0) / 10.0;
            name += "\n@ " + QString::number(distance) + " km";

            QRect rect = metrics.boundingRect(QRect(0, 0, w, h), Qt::AlignCenter, name);
            int xpadding = 2*RETINA_FACTOR;
            int ypadding = 1*RETINA_FACTOR;
            int triangleSize = 5*RETINA_FACTOR;
            int xmargin = 5*RETINA_FACTOR;
            int ymargin = 8*RETINA_FACTOR;
            rect.setWidth(rect.width() + 2*xpadding);
            rect.setHeight(rect.height() + 2*ypadding);
            rect.moveLeft(x - rect.width()/2);
            rect.moveBottom(y - triangleSize);

            // Check for collision with other labels
            QRect boundRect(rect.x() - xmargin, rect.y() - ymargin, rect.width() + 2*xmargin, rect.height() + triangleSize + 2*ymargin);
            bool accepted = true;
            for (auto& r : acceptedLabels)
            {
                if (boundRect.intersects(r))
                {
                    accepted = false;
                    break;
                }
            }
            if (!accepted)
                continue;
            acceptedLabels.push_back(boundRect);

            // Draw label
            QPainterPath path;
            path.moveTo(rect.topLeft());
            path.lineTo(rect.bottomLeft());
            path.lineTo(x-triangleSize, rect.bottom());
            path.lineTo(x, y);
            path.lineTo(x+triangleSize, rect.bottom());
            path.lineTo(rect.bottomRight());
            path.lineTo(rect.topRight());
            path.lineTo(rect.topLeft());

            QColor fill;
            switch (l.type) {
            case Label::PEAK:
                fill = QColor(0xFF, 0xFF, 0xC0, 0xC0); break;
            case Label::SADDLE:
                fill = QColor(0xD0, 0xD0, 0xFF, 0xC0); break;
            case Label::VOLCANO:
                fill = QColor(0xFF, 0xC0, 0xC0, 0xC0); break;
            default:
                fill = QColor(0xFF, 0xFF, 0xFF, 0xC0); break;
            }
            painter.fillPath(path, fill);
            painter.drawPath(path);

            painter.drawText(rect, Qt::AlignCenter, name);

            // Limit number of labels on screen
#ifdef MAX_LABELS_IN_VIEW
            if (acceptedLabels.size() == MAX_LABELS_IN_VIEW)
                break;
#endif
        }
    }
}

void GLWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, RETINA_FACTOR*width, RETINA_FACTOR*height);

    this->loadProjection();
}

void GLWidget::loadModelView()
{
    /* constexpr */ double pi = std::atan(1)*4;
    double theta = mRotate.y * pi / 180.0;
    double phi = mRotate.x * pi / 180.0;

    // TODO: more precise lookAt() instead of *1000
    Point origin = mWorldModel->origin().get();
    mEyeModel = Astro::mercatorToModel(mEyeMercator, origin);
    mEyeModel.z += VIEWER_HEIGHT;
    Point center = mEyeModel + Point(std::cos(theta)*std::sin(phi), std::cos(theta)*std::cos(phi), std::sin(theta))*1000 /* TODO: here */;

    mModelView.setToIdentity();
    mModelView.lookAt(QVector3D(mEyeModel.x, mEyeModel.y, mEyeModel.z), // eye
                      QVector3D(center.x, center.y, center.z), // center
                      QVector3D(0, 0, 1)); // up
}

void GLWidget::loadProjection()
{
    // Adaptive near plane from 1 to 100 meters to avoid depth glitches.
    double u = 0.5 + 0.5 * std::tanh((mZoom - 8.0) / 4.0);
    double near = 1.0*(1.0-u) + 100.0*u;
    // up to 200 km
    double far = 200000.0;
    double scaleFactor = 0.0005 * std::pow(2, -mZoom / 4.0) * near;
    double w = scaleFactor * width();
    double h = scaleFactor * height();
    mProjection.setToIdentity();
    mProjection.frustum(-w, w, -h, h, near, far);
}


void GLWidget::wheelEvent(QWheelEvent* event)
{
    mZoom += event->delta() / 120.0;
    event->accept();
    this->loadProjection();
    this->update();
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (mMousePress)
    {
        QPoint diff = event->pos() - mMousePos;

        double scaleFactor = std::pow(2, mZoom / 4.0 + 3.0);
        double dx = diff.x();
        double dy = diff.y();

        mRotate.x -= dx / scaleFactor;
        mRotate.y += dy / (2.0*scaleFactor);

        if (mRotate.y > 89.99)
            mRotate.y = 89.99;
        else if (mRotate.y < -89.99)
            mRotate.y = -89.99;

        this->update();
    }

    mMousePos = event->pos();
    event->accept();
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton)
        return;

    mMousePress = true;
    mMousePos = event->pos();
    this->setCursor(Qt::ClosedHandCursor);

    event->accept();
}

void GLWidget::mouseReleaseEvent(QMouseEvent* event)
{
    mMousePress = false;
    this->setCursor(Qt::OpenHandCursor);
    event->accept();
}

void GLWidget::keyPressEvent(QKeyEvent* event)
{
    /* constexpr */ double pi = std::atan(1)*4;

    Point origin = mWorldModel->origin().get();
    Point tmp = Astro::mercatorToModel(mEyeMercator, origin);
    double phi = mRotate.x * pi / 180.0;

    bool change = false;
    // TODO: follow Earth curvature if USE_EARTH_CURVATURE
    if (event->key() == Qt::Key_Up) {
        tmp += Point(std::sin(phi), std::cos(phi)) * 30;
        change = true;
    } else if (event->key() == Qt::Key_Down) {
        tmp -= Point(std::sin(phi), std::cos(phi)) * 30;
        change = true;
    } else if (event->key() == Qt::Key_PageUp) {
        tmp += Point(std::sin(phi), std::cos(phi)) * 1000;
        change = true;
    } else if (event->key() == Qt::Key_PageDown) {
        tmp -= Point(std::sin(phi), std::cos(phi)) * 1000;
        change = true;
    } else if (event->key() == Qt::Key_U) {
        tmp.z += 50;
        mDetached = true;
        change = true;
    } else if (event->key() == Qt::Key_D) {
        tmp.z -= 50;
        mDetached = true;
        change = true;
    } else if (event->key() == Qt::Key_T) {
        tmp.z += 500;
        mDetached = true;
        change = true;
    } else if (event->key() == Qt::Key_B) {
        tmp.z -= 500;
        mDetached = true;
        change = true;
    } else if (event->key() == Qt::Key_G) {
        mDetached = false;
        change = true;
    } else if (event->key() == Qt::Key_Left) {
        double scaleFactor = std::pow(2, mZoom / 4.0 + 3.0);
        mRotate.x -= 16 / scaleFactor;
        change = true;
    } else if (event->key() == Qt::Key_Right) {
        double scaleFactor = std::pow(2, mZoom / 4.0 + 3.0);
        mRotate.x += 16 / scaleFactor;
        change = true;
    } else if (event->key() == Qt::Key_I) {
        mZoom += 0.25;
        this->loadProjection();
        change = true;
    } else if (event->key() == Qt::Key_O) {
        mZoom -= 0.25;
        this->loadProjection();
        change = true;
    } else if (event->key() == Qt::Key_M) {
        mSunRotate += 5;
        change = true;
    } else if (event->key() == Qt::Key_E) {
        mSunRotate -= 5;
        change = true;
    }

    if (change)
    {
        computeEye(Astro::mercatorFromModel(tmp, origin));
        this->update();
        event->accept();
    }
    else
        event->ignore();
}

void GLWidget::computeEye(const Point& tmp)
{
    auto delaunay = mWorldModel->delaunay().get();
    assert(delaunay);
    auto ground = delaunay->findTrianglePoint(tmp);

    Point newSelection;
    mGroundKnown = (bool)ground;
    if (mGroundKnown)
    {
        newSelection = *ground;
        if (mDetached && tmp.z < ground->z)
            mDetached = false;
    }
    else
    {
        newSelection = tmp;
        mDetached = true;
    }

    if (mDetached)
        mEyeMercator = tmp;
    else
        mEyeMercator = newSelection;

    mWorldModel->selection().set(std::move(newSelection));
}
