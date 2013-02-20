#ifndef __PLSTIM_STIMWINDOW_H
#define __PLSTIM_STIMWINDOW_H

#include <QtGui>

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
    void initialize ();
    void render ();
    void setTextureSize (int twidth, int theight);
public slots:
    void renderNow ();
    void updateShaders ();
protected:
    virtual bool event (QEvent* evt);
    virtual void exposeEvent (QExposeEvent* evt);
    virtual void resizeEvent (QResizeEvent* evt);
private:
    QOpenGLContext* m_context;
    QMap<QString,GLuint> m_fixedFrames;
    QMap<QString,QImage> m_tobind;
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
