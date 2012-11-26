#ifndef __PLSTIM_GLWIDGET_H
#define __PLSTIM_GLWIDGET_H

#include <map>
#include <vector>

#include <QtOpenGL>
#include <QGLFunctions>


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

    std::vector<GLuint> unamed_frames;
    std::map<QString,GLuint> named_frames;

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

  public:
    MyGLWidget (const QGLFormat& format, QWidget* parent);

  protected:
    virtual void keyPressEvent (QKeyEvent* evt);

  public:

    bool add_frame (const QImage& img);

    void delete_unamed_frames ();

    void delete_named_frames ();

    bool add_frame (const QString& name, const QImage& img);

    void empty_frame ();

    void show_frame (const QString& name);

    void show_frames ();

    void full_screen ();

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
