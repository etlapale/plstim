#include <iomanip>
#include <sstream>

using namespace std;

#include "stimwindow.h"
using namespace plstim;

#ifdef WIN32
#include <windows.h>
#include "GL/wglext.h"
#endif

static const char *fshader_txt = 
    "varying vec2 tex_coord;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(texture, tex_coord);\n"
    "}\n";


StimWindow::StimWindow (QScreen* scr)
    : QWindow (scr), m_context (NULL), m_vshader (NULL),
      tex_width (0), tex_height (0),
      m_texloc (0), m_currentFrame (NULL)
{
    // Create a floatting window surface
    setFlags (Qt::Dialog);
    // Disable any cursor on the window
    setCursor (Qt::BlankCursor);

    // Enable OpenGL on the window
    setSurfaceType (QWindow::OpenGLSurface);
    // Define the surface format we want
    QSurfaceFormat fmt;
    // We want desktop OpenGL
    fmt.setRenderableType (QSurfaceFormat::OpenGL);
    // Activate double-buffering
    fmt.setSwapBehavior (QSurfaceFormat::DoubleBuffer);
    // We need at least OpenGL 3.0
    // TODO: investigate why we cannot get less than 3.2
    fmt.setMajorVersion (3);
    fmt.setMinorVersion (2);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat (fmt);

    // Create the window, allocating resources for it
    create ();
    qDebug () << "StimWindow screen currently on: " << screen ()->name ();
    setupOpenGL ();

    // Re-create an OpenGL context when screen is changed
    connect (this, &QWindow::screenChanged, [this] (QScreen* screen) {
              qDebug () << "StimWindow screen changed to: " << screen->name ();
              setupOpenGL ();
            });
}

StimWindow::~StimWindow ()
{
}

void
StimWindow::setupOpenGL ()
{
    qDebug () << "StimWindow::setupOpenGL ()";

    if (m_context != NULL) {
        qDebug () << "Deleting previous OpenGL context";
        delete m_context;
    }

    qDebug () << "Creating a new OpenGL context";
    m_context = new QOpenGLContext (this);
    m_context->setScreen (screen ());
    m_context->setFormat (format ());  // TODO: why not format()

    if (! m_context->create ()) {
      qCritical() << "error: could not create the OpenGL context";
      return;
    }
    if (! m_context->isValid ()) {
      qCritical() << "error: created OpenGL context is invalid";
      return;
    }

    auto fmt = m_context->format();
    qDebug() << "created context for OpenGL " << fmt.majorVersion() << "." << fmt.minorVersion() << "-" << fmt.profile();

    // Initialize the new OpenGL context
    if (! m_context->makeCurrent (this)) {
        qCritical() << "error: could not use the OpenGL context";
	return;
    }
    if (! initializeOpenGLFunctions ()) {
        qCritical() << "error: could not initialise the OpenGL functions";
	return;
    }

    // Enables V-Sync
#ifdef WIN32
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC) wglGetProcAddress ("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT == NULL)
        qCritical () << "error: could not get swap interval extension";
    else
        wglSwapIntervalEXT (1);
#endif

    // Create a shader program
    m_program = new QOpenGLShaderProgram (this);
    if (! m_program->addShaderFromSourceCode (QOpenGLShader::Fragment,
	    fshader_txt)) {
        qCritical () << "error: could not create the fragment shader:" << endl;
        return;
    }

    // Create a vertex array object (VAO)
    glGenVertexArrays (1, &m_vao);
    glBindVertexArray (m_vao);
    // Create the vertex buffer object (VBO) associated
    glGenBuffers (1,&m_vbo);
    glBindBuffer (GL_ARRAY_BUFFER,m_vbo);
    qDebug () << "glBVA" << glGetError ();

    // TODO: update shaders on stimwindow move?

    // Black as default background colour
    glClearColor (0, 0, 0, 0);

    m_context->doneCurrent ();

    m_opengl_initialized = true;
}

static QOpenGLTexture*
bindTexture (const QImage& img)
{
    // Convert pixel data from Qt to OpenGL format
    auto tex = new QOpenGLTexture (img.mirrored ());
    tex->setMinificationFilter (QOpenGLTexture::Linear);
    tex->setMagnificationFilter (QOpenGLTexture::Linear);
    tex->bind ();
    return tex;
}

void
StimWindow::addFixedFrame (const QString& name, const QImage& img)
{
    qDebug () << "StimWindow::addFixedFrame ()" << name;
    qDebug() << "gl ctx: " << m_context << m_opengl_initialized;
    
    if (! m_context->makeCurrent (this))
        qCritical () << "error: cannot use OpenGL context";

    // Delete existing texture
    if (m_fixedFrames.contains (name)) {
        qDebug () << "deleting existing homonymous texture";
        auto tex = m_fixedFrames[name];
        delete tex;
    }

    // Bind the freame to a texture
    m_fixedFrames[name] = bindTexture (img);

    m_context->doneCurrent ();
}

void
StimWindow::addAnimatedFrame (const QString& name, const QImage& img)
{
    //qDebug () << "adding animated frame" << name;
    if (! m_context->makeCurrent (this))
        qCritical () << "error: cannot use OpenGL context";

    auto& frames = m_animatedFrames[name];

    //img.save (QString("%1-%2.png").arg(name).arg(frames.size ()));

    frames.append (bindTexture (img));

    m_context->doneCurrent ();
}

void
StimWindow::deleteAnimatedFrames (const QString& name)
{
    if (m_animatedFrames.contains (name)) {
	m_context->makeCurrent (this);

	auto& frames = m_animatedFrames[name];
	for (auto tex : frames)
            delete tex;

	frames.clear ();
	m_animatedFrames.remove (name);

        m_context->doneCurrent ();
    }
}

void
StimWindow::clear ()
{
    qDebug () << "StimWindow::clear ()";

    if (m_context == nullptr) {
      qDebug() << "skipping clearing of uninitialized StimWindow";
      return;
    }

    m_context->makeCurrent (this);
    
    // Unset current frame
    m_currentFrame = 0;

    // Destroy fixed frame textures
    QMapIterator<QString,QOpenGLTexture*> it (m_fixedFrames);
    while (it.hasNext ()) {
	it.next ();
	auto tex = it.value ();
        delete tex;
    }
    m_fixedFrames.clear ();

    // Destroy animated frame textures
    QMapIterator<QString,QVector<QOpenGLTexture*>> jt (m_animatedFrames);
    while (jt.hasNext ()) {
	jt.next ();
	const QVector<QOpenGLTexture*>& vec = jt.value ();
	for (QOpenGLTexture* t : vec)
            delete t;
    }
    m_animatedFrames.clear ();

    m_context->doneCurrent ();
}

bool
StimWindow::event (QEvent* evt)
{
    switch (evt->type ()) {
    case QEvent::UpdateRequest:
	renderNow ();
	return true;
    default:
#ifdef HAVE_POWERMATE
	if (evt->type () == PowerMateEvent::Rotation) {
	    emit powerMateRotation (static_cast<PowerMateEvent*> (evt));
	    return true;
	}
	else if (evt->type () == PowerMateEvent::ButtonPress) {
	    qDebug () << "powermate button press in stim window";
	    emit powerMateButtonPressed (static_cast<PowerMateEvent*> (evt));
	    return true;
	}
	else if (evt->type () == PowerMateEvent::ButtonRelease) {
	    emit powerMateButtonReleased (static_cast<PowerMateEvent*> (evt));
	    return true;
	}
#endif
	return QWindow::event (evt);
    }
}

void StimWindow::exposeEvent(QExposeEvent*)
{
  qDebug() << "exposing the StimWindow" << endl;
  if (m_context == nullptr) {
    this->setupOpenGL();
  }
        
  emit exposed();
  
  //qDebug () << "stimwindow::expose";
  if (isExposed())
    renderNow();
}

void
StimWindow::keyPressEvent (QKeyEvent* evt)
{
    if (evt->key () == Qt::Key_L
            && (evt->modifiers () & Qt::ControlModifier)) {
        qDebug () << "Ctrl-L catched, listing screens:";
        QScreen* currentScreen = screen ();
        for (auto s : currentScreen->virtualSiblings ()) {
            qDebug () << " " << s->name () << s->virtualSize ()
                      << "at" << s->refreshRate () << "Hz"
                      << "availaible geometry:" << s->availableGeometry ()
                      << "avl. virtual geom.:" << s->availableVirtualGeometry ();
            if (s == currentScreen)
                qDebug () << "   ^ is current screen";
        }
        return;
    }
    else if (evt->key () == Qt::Key_O
            && (evt->modifiers () & Qt::ControlModifier)) {
        qDebug () << "Ctrl-O catched, movint to next screen";
        QScreen* currentScreen = screen ();
        for (auto s : currentScreen->virtualSiblings ()) {
            if (s != currentScreen) {
                qDebug () << "   moving to" << s->name ();
                setScreen (s);
                QWindow::showFullScreen ();
                return;
            }
        }
        return;
    }
    emit keyPressed (evt);
}

void
StimWindow::setTextureSize (int twidth, int theight)
{
  // No change necessary
  if (twidth == tex_width && theight == tex_height)
    return;

  qDebug () << "!!! setting tex dims to" << twidth << theight;

  tex_width = twidth;
  tex_height = theight;

  updateShaders ();
}

void
StimWindow::updateShaders ()
{
    //if (! isExposed () || tex_width == 0) {
    if (tex_width == 0 || width () == 0 || height () == 0) {
        qDebug () << "Not updating shaders!!! size is: " << width () << height ();
	return;
    }

    qDebug () << "*** updating the shaders";

    GLint gl_width = width ();
    GLint gl_height = height ();

    if (! m_context->makeCurrent (this))
        qCritical () << "error: could not use the OpenGL context";
    
    // Create an identity vertex shader
    glViewport (0, 0, gl_width, gl_height);

    // Compute the offset to center the stimulus
    GLfloat txw = static_cast<GLfloat> (tex_width) / gl_width;
    GLfloat txh = static_cast<GLfloat> (tex_height) / gl_height;

    GLfloat ofx = tex_width > gl_width ? 0.0f : 1.0f - txw;
    GLfloat ofy = tex_height > gl_height ? 0.0f : 1.0f - txh;

    GLfloat txm = ofx;
    GLfloat txM = ofx + 2.0f * txw;
    GLfloat tym = ofy;
    GLfloat tyM = ofy + 2.0f * txh;

    GLfloat tgw2_ratio = 2.0f * txw;
    GLfloat tgh2_ratio = 2.0f * txh;

    stringstream ss;
    ss << fixed << setprecision(12)
       << "const mat4 proj_matrix = mat4("
       << (2.0/gl_width) << ", 0.0, 0.0, -1.0," << endl
       << "0.0, " << -(2.0/gl_height) << ", 0.0, 1.0," << endl
       << "0.0, 0.0, -1.0, 0.0," << endl
       << "0.0, 0.0, 0.0, 1.0);" << endl
       << "attribute vec2 ppos;" << endl
       << "varying vec2 tex_coord;" << endl
       << "void main () {" << endl
       << "  gl_Position = vec4(ppos.x-1.0, ppos.y-1.0, 0.0, 1.0);" << endl
       << "  tex_coord = vec2((ppos.x-" << ofx << ")/" << tgw2_ratio
       << ", (ppos.y-"<<ofy<<")/" << tgh2_ratio << ");" << endl
       << "}" << endl;
    auto vshader_str = ss.str();
    const char* vshader_txt = vshader_str.c_str();
    //qDebug () << vshader_txt;
    
    // Remove previous vertex shader
    if (m_vshader != NULL) {
        qDebug () << "Removing previous vertex shader";
	m_program->removeShader (m_vshader);
	delete m_vshader;
    }

    // Create a new vertex shader
    m_vshader = new QOpenGLShader (QOpenGLShader::Vertex);
    if (! m_vshader->compileSourceCode (vshader_txt))
        qCritical () << "error: could not compile the vertex shader";
    if (! m_program->addShader (m_vshader))
        qCritical () << "error: could not add the vertex shader";
    if (! m_program->link ())
        qCritical () << "error: could not link the shader program";
    if (! m_program->bind ())
        qCritical () << "error: could not bind the shader program";

    // Get the texture location
    glActiveTexture (GL_TEXTURE0);
    qDebug () << "glActTex err:" << glGetError ();

    m_texloc = m_program->uniformLocation ("texture");
    qDebug () << "texture located at:" << m_texloc;
    int ppos = m_program->attributeLocation ("ppos");
    qDebug () << "ppos attribute located at:" << ppos;
    qDebug () << "pposattloc err:" << glGetError ();

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

    glBufferData (GL_ARRAY_BUFFER, 12*sizeof(GLfloat), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer (ppos, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray (ppos);
    qDebug () << "glEnVeAP err:" << glGetError ();

    m_context->doneCurrent ();
}

void
StimWindow::resizeEvent (QResizeEvent* evt)
{
    Q_UNUSED (evt);
    qDebug () << "StimWindow::resizeEvent()";
    updateShaders ();
}

void
StimWindow::showFixedFrame (const QString& name)
{
    if (! m_fixedFrames.contains (name)) {
	qCritical () << "??? unknown fixed frame" << name;
    }
    else {
        qDebug () << "showing fixed frame" << name;
        m_currentFrame = m_fixedFrames[name];
        renderNow ();
    }
}

void
StimWindow::showAnimatedFrames (const QString& name)
{
    qDebug () << "showing animated frames" << name;
    if (! m_animatedFrames.contains (name)) {
	qCritical () << "??? unknown animated frame" << name;
    }
    else {
        if (! m_context->makeCurrent (this)) {
            qDebug () << "Could not make" << this << "the current context";
            return;
        }

	auto& frames = m_animatedFrames[name];

        QElapsedTimer timer;
        timer.start ();
	for (auto frame : frames) {
	    m_currentFrame = frame;
	    render ();
	    m_context->swapBuffers (this);
            /*for (int i = 0; i < 10; i++) {
              render ();
              m_context->swapBuffers (this);
            }*/
	}
        qint64 nsecs = timer.nsecsElapsed ();
        qDebug () << "animated frames shown in" << (nsecs/1000000) << "ms";

        m_context->doneCurrent ();
    }
}

void
StimWindow::render ()
{
    //qDebug () << "StimWindow::render ()";
    glClear (GL_COLOR_BUFFER_BIT);
    //qDebug () << "glClear errors:" << glGetError ();

    if (m_currentFrame == NULL) {
	qDebug () << "render() with no effect (no frame)";
	return;
    }

    m_currentFrame->bind ();
    //qDebug () << "    current frame: " << m_currentFrame << ", texloc: " << m_texloc;
    //glBindTexture (GL_TEXTURE_2D, m_currentFrame);
    //qDebug () << "BindTexture errors:" << glGetError ();
    glUniform1i (m_texloc, 0);
    //qDebug () << "uniform1i errors: " << glGetError ();
    glDrawArrays (GL_TRIANGLES, 0, 6);

    //glDrawElements (GL_TRIANGLES, 6, 

    //qDebug () << glGetError ();
}

void
StimWindow::renderNow ()
{
  if (! m_opengl_initialized)
    return;
    //qDebug () << "stimwindow::rendernow";
    //if (! isExposed ())
	//return;

    // Render the frame, and swap the buffers
    //qDebug () << "rendering and swapping";
    m_context->makeCurrent (this);
    //qDebug () << "make current errors:" << glGetError ();
    render ();
    m_context->swapBuffers (this);
    m_context->doneCurrent ();
}

// vim: sw=4
