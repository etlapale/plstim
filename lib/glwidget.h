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
    {
    }

    void update_size (int tex_width, int tex_height) {
      setMinimumSize (tex_width, tex_height);
    }

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
      initializeGLFunctions ();

      glClearColor (0, 0, 0, 0);

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
	//"varying mediump vec2 tex_coord;\n"
	//"uniform sampler2D texture;\n"
	"void main() {\n"
	"  gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);\n"
	//"  gl_FragColor = texture2D(texture, tex_coord);\n"
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

#if 0
      // Compute the offset to center the stimulus
      GLfloat offx = (win_width-tex_width)/2.0f;
      GLfloat offy = (win_height-tex_height)/2.0f;

      float tx_conv = tex_width / (2.0f * win_width);
      float ty_conv = tex_height / (2.0f * win_height);

      tx_conv = 1/2.0f;
      ty_conv = 1/2.0f;

      // Create an identity vertex shader
      ss.str("");
      ss << fixed << setprecision(12)
	// This projection matrix maps OpenGL coordinates
	// to device coordinates (pixels)
	<< "const mat4 proj_matrix = mat4("
	<< (2.0/win_width) << ", 0.0, 0.0, -1.0," << endl
	<< "0.0, " << -(2.0/win_height) << ", 0.0, 1.0," << endl
	<< "0.0, 0.0, -1.0, 0.0," << endl
	<< "0.0, 0.0, 0.0, 1.0);" << endl
	<< "attribute vec2 ppos;" << endl
	<< "varying vec2 tex_coord;" << endl
	<< "void main () {" << endl
	<< "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0) * proj_matrix;" << endl
	<< "  tex_coord = vec2((ppos.x-" << offx << ")/" << (float) tex_width 
	<< ", (ppos.y-" << offy << ")/" << (float) tex_height << " );" << endl
	<< "}" << endl;
      auto vshader_str = ss.str();
      cout << "vertex shader:" << endl << vshader_str << endl;

      GLuint vshader = glCreateShader (GL_VERTEX_SHADER);
      if (vshader == 0) {
	fprintf (stderr, "could not create a vertex shader (0x%x)\n",
		 glGetError ());
	return 1;
      }
      assert_gl_error ("create a vertex shader");
#endif
    }

    void resizeGL (int w, int h)
    {
      std::cout << "resizeGL(" << w << ", " << h << ")" << std::endl;
      glClear (GL_COLOR_BUFFER_BIT);
    }

    void paintGL ()
    {
      std::cout << "paintGL()" << std::endl;
    }
  };
}

#endif // __PLSTIM_GLWIDGET_H
