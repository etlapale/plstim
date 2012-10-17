#ifndef __PLSTIM_GLWIDGET_H
#define __PLSTIM_GLWIDGET_H

#include <stdlib.h>
#include <iostream>
#include <sstream>

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
  public:
    MyGLWidget (const QGLFormat& format, QWidget* parent)
      : QGLWidget (format, parent),
	nframes (3)
    {}
  protected:
    void assert_gl_error (const std::string& msg)
    {
      if (glGetError () != GL_NO_ERROR) {
	std::cerr << "error: " << msg << std::endl;
	exit (1);
      }
    }

    virtual void initializeGL ()
    {
      using namespace std;

      std::cout << "initializeGL()" << std::endl;

      glClearColor (1, 0, 0, 0);

      // Create textures for the main frames
      std::cout << "  creating " << nframes << " textures" << endl;
      tframes = new GLuint[nframes];
      if (tframes == NULL) {
	std::cerr << "could not allocated memory for the textures" << std::endl;
	exit (1);
      }
      glGenTextures (nframes, tframes);
      if (glGetError () != GL_NO_ERROR) {
	std::cerr << "could not generate the textures" << endl;
	exit (1);
      }

      // Create a fragment shader for the texture
      stringstream ss;
      static const char *fshader_txt = 
	"varying mediump vec2 tex_coord;\n"
	"uniform sampler2D texture;\n"
	"void main() {\n"
	//"  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
	"  gl_FragColor = texture2D(texture, tex_coord);\n"
	//"  gl_FragColor = vec4(tex_coord.x, tex_coord.y, 0, 1);\n"
	"}\n";
      GLuint fshader = glCreateShader (GL_FRAGMENT_SHADER);
      assert_gl_error ("create a fragment shader");
      if (fshader == 0) {
	fprintf (stderr, "could not create a fragment shader (0x%x)\n",
		 glGetError ());
	exit (1);
      }
      glShaderSource (fshader, 1, (const char **) &fshader_txt, NULL);
      glCompileShader (fshader);
      GLint stat;
      glGetShaderiv (fshader, GL_COMPILE_STATUS, &stat);
      if (stat == 0) {
	GLsizei len;
	char log[1024];
	glGetShaderInfoLog (fshader, 1024, &len, log);
	cerr << "could not compile the fragment shader:" << endl
	     << log << endl;
	exit (1);
      }
    }

    void resizeGL (int w, int h)
    {
      std::cout << "resizeGL(" << w << ", " << h << ")" << std::endl;
    }

    void paintGL ()
    {
      std::cout << "paintGL()" << std::endl;
    }
  };
}

#endif // __PLSTIM_GLWIDGET_H
