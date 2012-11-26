#ifndef __PLSTIM_GLWIDGET_H
#define __PLSTIM_GLWIDGET_H

#include <stdlib.h>

#include <iomanip>
#include <iostream>
#include <map>
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

    std::vector<GLuint> unamed_frames;
    std::map<QString,GLuint> named_frames;

    GLint texloc;

  protected:
    bool first_shader_update;
    float hratio;
    float vratio;

    GLfloat vertices[12];
    GLuint current_frame;

  signals:
    void gl_initialised ();
    void gl_resized (int w, int h);
    void normal_screen_restored ();
    void key_press_event (QKeyEvent* evt);

  public:
    MyGLWidget (const QGLFormat& format, QWidget* parent)
      : QGLWidget (format, parent),
	nframes (3),
	gl_width (0), gl_height (0),
	tex_width (0), tex_height (0),
	fshader (0), vshader (0), program (0),
	vshader_attached (false),
	first_shader_update (true),
	current_frame (0)
    {
      setFocusPolicy (Qt::StrongFocus);
    }

  protected:
    virtual void keyPressEvent (QKeyEvent* evt) {
      //cout << "key press" << endl;
      if (evt->key () == Qt::Key_Escape)
	normal_screen ();
      else
	emit key_press_event (evt);
    }

  public:

    bool add_frame (const QImage& img) {

      unamed_frames.push_back (bindTexture (img));
      cout << "adding unamed frame, count: " << unamed_frames.size () << endl;
      return true;
    }

    void delete_unamed_frames () {

      for (auto f : unamed_frames)
	deleteTexture (f);

      unamed_frames.clear ();
    }

    void delete_named_frames () {

      for (auto f : named_frames)
	deleteTexture (f.second);

      named_frames.clear ();
    }

    bool add_frame (const QString& name, const QImage& img) {

      // Delete existing texture
      if (named_frames.find (name) != named_frames.end ())
	deleteTexture (named_frames[name]);

      named_frames[name] = bindTexture (img);
      return true;
    }

    void empty_frame () {
      cout << "displaying an empty frame" << endl;

      glClear (GL_COLOR_BUFFER_BIT);
      swapBuffers ();
    }

    void show_frame (const QString& name) {
      current_frame = named_frames[name];

      qDebug () << "show frame" << name << ", at" << current_frame;

      glClear (GL_COLOR_BUFFER_BIT);

      glBindTexture (GL_TEXTURE_2D, current_frame);
      assert_gl_error ("binding a texture");

      glUniform1i (texloc, 0);
      assert_gl_error ("specifying an uniform");

      glDrawArrays (GL_TRIANGLES, 0, 6);
      swapBuffers ();
    }

    void show_frames () {

      cout << "show multiple frames" << endl;

      for (auto f : unamed_frames) {
	current_frame = f;

	glClear (GL_COLOR_BUFFER_BIT);

	glBindTexture (GL_TEXTURE_2D, current_frame);
	glUniform1i (texloc, 0);

	glDrawArrays (GL_TRIANGLES, 0, 6);
	swapBuffers ();
      }
    }

    void full_screen () {
      setParent (NULL, Qt::Dialog|Qt::FramelessWindowHint);
      setCursor (QCursor (Qt::BlankCursor));
      showFullScreen ();
      //emit gl_resized (width (), height ());

      //paintGL ();
    }

    void normal_screen () {
      unsetCursor ();
      emit normal_screen_restored ();
    }

    void update_texture_size (int twidth, int theight) {
      // No change necessary
      if (twidth == tex_width && theight == tex_height)
	return;

      qDebug () << "setting tex dims to" << twidth << theight;

      tex_width = twidth;
      tex_height = theight;

      update_shaders ();
    }

    void update_shaders () {
      // Not yet ready
      if (tex_width == 0 || tex_height == 0
	  || gl_width == 0 || gl_height == 0)
	return;

      cout << "update_shaders ()" << endl;
      cout << "    " << isValid () << endl;

      if (first_shader_update) {
	hratio = (float) tex_width / gl_width;
	vratio = (float) tex_height / gl_height;
	cout << "Ratio at first shader update: " << hratio << " & " << vratio << endl;
	first_shader_update = false;
      }

      //qDebug () << "tex:" << tex_width << tex_height << "gl:" << gl_width << gl_height;

      // Create an identity vertex shader
      glViewport (0, 0, (GLint) gl_width, (GLint) gl_height);

      // Compute the offset to center the stimulus
      GLfloat ofx = tex_width > gl_width ? 0.0f : 1.0f - (GLfloat) tex_width / gl_width;
      GLfloat ofy = tex_height > gl_height ? 0.0f : 1.0f - (GLfloat) tex_height / gl_height;

      GLfloat txm = ofx;
      GLfloat txM = ofx + 2.0f * (GLfloat) tex_width/gl_width;
      GLfloat tym = ofy;
      GLfloat tyM = ofy + 2.0f * (GLfloat) tex_height/gl_height;

      GLfloat tgw2_ratio = 2.0f * (GLfloat) tex_width/gl_width;
      GLfloat tgh2_ratio = 2.0f * (GLfloat) tex_height/gl_height;

      stringstream ss;
      ss << fixed << setprecision(12)
	// This projection matrix maps OpenGL coordinates
	// to device coordinates (pixels)
	<< "const mat4 proj_matrix = mat4("
	//<< (2.0/gl_width) << ", 0.0, 0.0, -1.0," << endl
	//<< "0.0, " << -(2.0/gl_height) << ", 0.0, 1.0," << endl
	<< (2.0/gl_width) << ", 0.0, 0.0, -1.0," << endl
	<< "0.0, " << -(2.0/gl_height) << ", 0.0, 1.0," << endl
	<< "0.0, 0.0, -1.0, 0.0," << endl
	<< "0.0, 0.0, 0.0, 1.0);" << endl
	<< "attribute vec2 ppos;" << endl
	<< "varying vec2 tex_coord;" << endl
	<< "void main () {" << endl
	//<< "  gl_Position = vec4(ppos.x, ppos.y, 0.0, 1.0) * proj_matrix;" << endl
	//<< "  gl_Position = vec4(ppos.x*" << hratio << ", ppos.y*" << vratio << ", 0.0, 1.0);" << endl
	<< "  gl_Position = vec4(ppos.x-1.0, ppos.y-1.0, 0.0, 1.0);" << endl
	<< "  tex_coord = vec2((ppos.x-" << ofx << ")/" << tgw2_ratio
	<< ", (ppos.y-"<<ofy<<")/" << tgh2_ratio << ");" << endl
	//<< "  tex_coord = vec2((ppos.x-" << offx << ")/" << (float) tex_width 
	//<< ", (ppos.y-" << offy << ")/" << (float) tex_height << " );" << endl
	<< "}" << endl;
      auto vshader_str = ss.str();
      //cout << "vertex shader:" << endl << vshader_str << endl;


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

      cout << "program linked and used" << endl;

      glActiveTexture (GL_TEXTURE0);
      texloc = glGetUniformLocation (program, "texture");
      if (texloc == -1) {
	cerr << "could not get location of ‘texture’" << endl;
	//exit (1);
      }
      qDebug () << "texture location:" << texloc;
      assert_gl_error ("get location of uniform ‘texture’");


      int ppos = glGetAttribLocation (program, "ppos");
      if (ppos == -1) {
	fprintf (stderr, "could not get attribute ‘ppos’\n");
	exit (1);
      }
      //qDebug () << "ppos: " << ppos;

      //qDebug () << "tw/th:" << tex_width << tex_height;
      //qDebug () << "gw/gh:" << gl_width << gl_height;

      //qDebug () << "txM/tyM:" << txM << tyM;

      // Triangle covering half the texture
      vertices[0] = txm;
      vertices[1] = tym;
      vertices[2] = txM;
      vertices[3] = tym;
      vertices[4] = txm;
      vertices[5] = tyM;

      // Other half triangle
      vertices[6] = txm;
      vertices[7] = tyM;
      vertices[8] = txM;
      vertices[9] = tym;
      vertices[10] = txM;
      vertices[11] = tyM;

      glEnableVertexAttribArray (ppos);
      glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, vertices);
      glClear (GL_COLOR_BUFFER_BIT);

      paintGL ();
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

      std::cout << "initializeGL() " << hex << (long) this << dec << std::endl;
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
	"varying vec2 tex_coord;\n"
	"uniform sampler2D texture;\n"
	"void main() {\n"
	"  gl_FragColor = texture2D(texture, tex_coord);\n"
	//"  gl_FragColor = vec4(0.9, 0.8, 0.7, 1.0);\n"
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

      glEnable (GL_TEXTURE_2D);

      emit gl_initialised ();
    }

    void resizeGL (int w, int h)
    {
      std::cout << "resizeGL(" << w << ", " << h << ") "
                << hex << (long) this << dec << std::endl;
      gl_width = w;
      gl_height = h;
      update_shaders ();

      //glClear (GL_COLOR_BUFFER_BIT);

      emit gl_resized (w, h);
    }

    void paintGL ()
    {
      if (current_frame == 0) {
	cout << "paintGL() with no effect" << endl;
	glClear (GL_COLOR_BUFFER_BIT);
	swapBuffers ();
	return;
      }

      std::cout << "paintGL(" << current_frame << ")" << std::endl;

      glClear (GL_COLOR_BUFFER_BIT);

      glBindTexture (GL_TEXTURE_2D, current_frame);
      glUniform1i (texloc, 0);
      glDrawArrays (GL_TRIANGLES, 0, 6);

      swapBuffers ();
    }
  };
}

#endif // __PLSTIM_GLWIDGET_H
