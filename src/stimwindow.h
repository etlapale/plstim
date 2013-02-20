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
    void initialize ();
    void render ();
public slots:
    void renderNow ();
protected:
    virtual bool event (QEvent* evt);
    virtual void exposeEvent (QExposeEvent* evt);
    virtual void resizeEvent (QResizeEvent* evt);
private:
    QOpenGLContext* m_context;
};
}

#endif // __PLSTIM_STIMWINDOW_H
