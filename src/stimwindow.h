#ifndef __PLSTIM_STIMWINDOW_H
#define __PLSTIM_STIMWINDOW_H

#include <QtGui>

#ifdef HAVE_POWERMATE
#include "powermate.h"
#endif // HAVE_POWERMATE

namespace plstim
{
class StimWindow : public QWindow, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit StimWindow (QWindow* parent = NULL);
    ~StimWindow ();
    void addFixedFrame (const QString& name, const QImage& img);
    void showFixedFrame (const QString& name);
    void addAnimatedFrame (const QString& name, const QImage& img);
    void showAnimatedFrames (const QString& name);
    void deleteAnimatedFrames (const QString& name);
    void initialize ();
    void render ();
    void setTextureSize (int twidth, int theight);
    /// Delete everything created for this stimulus window
    void clear ();
public slots:
    void renderNow ();
    void updateShaders ();
signals:
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
private:
    QOpenGLContext* m_context;
    QMap<QString,GLuint> m_fixedFrames;
    QMap<QString,QImage> m_tobind;
    QMap<QString,QVector<GLuint>> m_animatedFrames;
    QMap<QString,QVector<QImage>> m_toabind;
    QOpenGLShaderProgram* m_program;
    QOpenGLShader* m_vshader;
    int tex_width;
    int tex_height;
    GLfloat vertices[12];
    int m_texloc;
    int m_currentFrame;
    QString m_toshow;
};
}

#endif // __PLSTIM_STIMWINDOW_H
