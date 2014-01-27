#ifndef __PLSTIM_STIMWINDOW_H
#define __PLSTIM_STIMWINDOW_H

#include <QtGui>
#include <QOpenGLFunctions_3_0>

#ifdef HAVE_POWERMATE
#include "powermate.h"
#endif // HAVE_POWERMATE

namespace plstim
{
class StimWindow : public QWindow, protected QOpenGLFunctions_3_0
{
    Q_OBJECT
public:
    explicit StimWindow (QScreen* scr=NULL);
    ~StimWindow ();
    void addFixedFrame (const QString& name, const QImage& img);
    void showFixedFrame (const QString& name);
    void addAnimatedFrame (const QString& name, const QImage& img);
    void showAnimatedFrames (const QString& name);
    void deleteAnimatedFrames (const QString& name);
    void render ();
    void setTextureSize (int twidth, int theight);
    /// Delete everything created for this stimulus window
    void clear ();
public slots:
    void renderNow ();
    void updateShaders ();
signals:
    void exposed ();
    void keyPressed (QKeyEvent* evt);
#ifdef HAVE_POWERMATE
    void powerMateRotation (PowerMateEvent* evt);
    void powerMateButtonPressed (PowerMateEvent* evt);
    void powerMateButtonReleased (PowerMateEvent* evt);
#endif // HAVE_POWERMATE
protected:
    virtual bool event (QEvent* evt);
    virtual void exposeEvent (QExposeEvent* evt);
    virtual void resizeEvent (QResizeEvent* evt);
    virtual void keyPressEvent (QKeyEvent* evt);

    void setupOpenGL ();
private:
    QOpenGLContext* m_context;
    QMap<QString,QOpenGLTexture*> m_fixedFrames;
    QMap<QString,QVector<QOpenGLTexture*>> m_animatedFrames;
    QOpenGLShaderProgram* m_program;
    QOpenGLShader* m_vshader;
    int tex_width;
    int tex_height;
    GLfloat vertices[12];
    int m_texloc;
    QOpenGLTexture* m_currentFrame;
    GLuint m_vao;
    GLuint m_vbo;
};
}

#endif // __PLSTIM_STIMWINDOW_H
