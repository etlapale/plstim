#ifndef __PLSTIM_GLWIDGET_H
#define __PLSTIM_GLWIDGET_H

#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#include <QtOpenGL>
#include <QGLFunctions>


// TODO: split that into a .cc
using namespace std;


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
  public:
    MyGLWidget (const QGLFormat& format, QWidget* parent)
      : QGLWidget (format, parent),
	nframes (3),
	gl_width (0), gl_height (0),
	tex_width (0), tex_height (0),
	fshader (0), vshader (0), program (0),
	vshader_attached (false)
    {
    }

    void update_texture_size (int twidth, int theight) {
      // No change necessary
      if (twidth == tex_width && theight == tex_height)
	return;

      qDebug () << "setting tex dims to" << twidth << theight;

      tex_width = twidth;
      tex_height = theight;

      //setMinimumSize (tex_width, tex_height);
      setMinimumSize (tex_width/2, tex_height/2);
      update_shaders ();
    }

    void update_shaders () {
      // Not yet ready
      if (tex_width == 0 || tex_height == 0
	  || gl_width == 0 || gl_height == 0)
	return;

      // Compute the offset to center the stimulus
      GLfloat offx = gl_width < tex_width ? 0 : (gl_width-tex_width)/2.0f;
      GLfloat offy = gl_height < tex_height ? 0 : (gl_height-tex_height)/2.0f;
      qDebug () << tex_width << tex_height << gl_width << gl_height;

      float tx_conv = tex_width / (2.0f * gl_width);
      float ty_conv = tex_height / (2.0f * gl_height);

      tx_conv = 1/2.0f;
      ty_conv = 1/2.0f;

      // Create an identity vertex shader
      stringstream ss;
      ss << fixed << setprecision(12)
	// This projection matrix maps OpenGL coordinates
	// to device coordinates (pixels)
	<< "const mat4 proj_matrix = mat4("
	<< (2.0/gl_width) << ", 0.0, 0.0, -1.0," << endl
	<< "0.0, " << -(2.0/gl_height) << ", 0.0, 1.0," << endl
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


      const char* vshader_txt = vshader_str.c_str();
      glShaderSource (vshader, 1, (const char **) &vshader_txt, NULL);
      glCompileShader (vshader);
      int stat;
      glGetShaderiv (vshader, GL_COMPILE_STATUS, &stat);
      if (stat == 0) {
	GLsizei len;
	char log[1024];
	glGetShaderInfoLog (vshader, 1024, &len, log);
	cerr << "could not compile the vertex shader:" << endl
	     << log << endl;
	exit (1);
      }

      // Add the fragment shader to the current program
      if (vshader_attached)
	glDetachShader (program, vshader);
      else
	vshader_attached = true;
      glAttachShader (program, vshader);
      assert_gl_error ("use attach the vertex shader");

      // Link the updated shaders in the program
      glLinkProgram (program);
      assert_gl_error ("use link the program");
      glGetProgramiv (program, GL_LINK_STATUS, &stat);
      if (stat == 0) {
	GLsizei len;
	char log[1024];
	glGetProgramInfoLog (program, 1024, &len, log);
	fprintf (stderr,
		 "could not link the shaders:\n%s\n", log);
	exit (1);
      }
      glUseProgram (program);
      assert_gl_error ("use the shaders program");


      int ppos = glGetAttribLocation (program, "ppos");
      if (ppos == -1) {
	fprintf (stderr, "could not get attribute ‘ppos’\n");
	exit (1);
      }

      // Rectangle covering the full texture
      static GLfloat vertices[] = {
	offx, offy,
	offx, offy + tex_width,
	offx + tex_width, offy + tex_height,

	offx + tex_width, offy + tex_height,
	offx, offy,
	offx + tex_width, offy
      };
      glEnableVertexAttribArray (ppos);
      glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glClear (GL_COLOR_BUFFER_BIT);

      /*
      glActiveTexture (GL_TEXTURE0);

      texloc = glGetUniformLocation (program, "texture");
      if (texloc == -1) {
	cerr << "could not get location of ‘texture’" << endl;
	return 1;
      }
      assert_gl_error ("get location of uniform ‘texture’");
      */
    }

  protected:
    void assert_gl_error (const std::string& msg)
    {
      auto err = glGetError ();
      if (err != GL_NO_ERROR) {
	std::cerr << "error: could not " << msg << ": ";
	switch (err) {
	case GL_INVALID_ENUM:
	  cerr << "invalid enum";
	  break;
	case GL_INVALID_VALUE:
	  cerr << "invalid value";
	  break;
	case GL_INVALID_OPERATION:
	  cerr << "invalid operation";
	  break;
	case GL_STACK_OVERFLOW:
	  cerr << "stack overflow";
	  break;
	case GL_STACK_UNDERFLOW:
	  cerr << "stack underflow";
	  break;
	case GL_OUT_OF_MEMORY:
	  cerr << "out of memory";
	  break;
	default:
	  cerr << "unknown error";
	  break;
	}
	cerr << endl;
	exit (1);
      }
    }

    virtual void initializeGL ()
    {
      using namespace std;

      std::cout << "initializeGL()" << std::endl;
      initializeGLFunctions ();

      glClearColor (0.1, 0, 0.1, 0);

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
      fshader = glCreateShader (GL_FRAGMENT_SHADER);
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

      vshader = glCreateShader (GL_VERTEX_SHADER);
      if (vshader == 0) {
	cerr << "could not create a vertex shader" << endl;
	exit (1);
      }

      program = glCreateProgram ();
      if (program == 0) {
	cerr << "could not create an OpenGL program" << endl;
	exit (1);
      }

      glAttachShader (program, fshader);
    }

    void resizeGL (int w, int h)
    {
      std::cout << "resizeGL(" << w << ", " << h << ")" << std::endl;
      gl_width = w;
      gl_height = h;
      update_shaders ();

      glClear (GL_COLOR_BUFFER_BIT);
    }

    void paintGL ()
    {
      std::cout << "paintGL()" << std::endl;

      glClear (GL_COLOR_BUFFER_BIT);
      glDrawArrays (GL_TRIANGLES, 0, 6);
      swapBuffers ();
    }
  };
}

#endif // __PLSTIM_GLWIDGET_H
