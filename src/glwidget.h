#ifndef __PLSTIM_GLWIDGET_H
#define __PLSTIM_GLWIDGET_H

#include <map>
#include <vector>

#include <QtOpenGL>
#include <QGLFunctions>

#ifdef HAVE_POWERMATE
#include "powermate.h"
#endif // HAVE_POWERMATE

namespace plstim
{
  class MyGLWidget : public QGLWidget, protected QGLFunctions
  {
  Q_OBJECT
  public:
    int nframes;
    GLuint* tframes;
    int gl_width;
    int gl_height;
    int tex_width;
    int tex_height;
    GLuint fshader;
    GLuint vshader;
    GLuint program;
    bool vshader_attached;

    std::map<QString,std::vector<GLuint>> animated_frames;
    std::map<QString,GLuint> fixed_frames;

    GLint texloc;

  protected:
    bool first_shader_update;
    float hratio;
    float vratio;

    GLfloat vertices[12];
    GLuint current_frame;

    bool waiting_fullscreen;

  signals:
    void gl_initialised ();
    void can_run_trial ();
    void normal_screen_restored ();
    void key_press_event (QKeyEvent* evt);
#ifdef HAVE_POWERMATE
    void powermate_event (PowerMateEvent* evt);
#endif // HAVE_POWERMATE

  public:
    MyGLWidget (const QGLFormat& format, QWidget* parent);

  protected:
#ifdef HAVE_POWERMATE
    virtual bool event (QEvent* event);
#endif // HAVE_POWERMATE
    virtual void keyPressEvent (QKeyEvent* evt);

  public:

    bool add_animated_frame (const QString& name, const QImage& img);

    bool add_fixed_frame (const QString& name, const QImage& img);

    void delete_animated_frames (const QString& name);

    void delete_animated_frames ();

    void delete_fixed_frames ();

    void empty_frame ();

    void show_fixed_frame (const QString& name);

    void show_animated_frames (const QString& name);

    void full_screen (int x=0, int y=0);

    void normal_screen ();

    void update_texture_size (int twidth, int theight);

    void update_shaders (); 

  protected:
    void assert_gl_error (const std::string& msg);

    virtual void initializeGL ();

    void resizeGL (int w, int h);

    void paintGL ();
  };
}

#endif // __PLSTIM_GLWIDGET_H
