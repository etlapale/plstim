#include <iomanip>
#include <sstream>

using namespace std;

#include "stimwindow.h"
using namespace plstim;

static const char *fshader_txt = 
    "varying vec2 tex_coord;\n"
    "uniform sampler2D texture;\n"
    "void main() {\n"
    "  gl_FragColor = texture2D(texture, tex_coord);\n"
    "}\n";


StimWindow::StimWindow (QWindow* prnt)
    : QWindow (prnt), m_context (NULL), m_vshader (NULL),
      tex_width (0), tex_height (0),
      m_texloc (0), m_currentFrame (0)
{
    setSurfaceType (QWindow::OpenGLSurface);

    QSurfaceFormat fmt;
    fmt.setSwapBehavior (QSurfaceFormat::DoubleBuffer);
    fmt.setMajorVersion (3);
    fmt.setMinorVersion (0);
    setFormat (fmt);

    setCursor (Qt::BlankCursor);
}

StimWindow::~StimWindow ()
{
}

static GLuint
bindTexture (const QImage& img)
{
    // Convert pixel data from Qt to OpenGL format
    QImage glimg = img.rgbSwapped ().mirrored ();

    GLuint texid;

    glGenTextures (1, &texid);
    glBindTexture (GL_TEXTURE_2D, texid);

    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
	    glimg.width (), glimg.height (), 0, GL_RGBA,
	    GL_UNSIGNED_BYTE, glimg.bits ());
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texid;
}

void
StimWindow::addFixedFrame (const QString& name, const QImage& img)
{
    if (isExposed ()) {
	//qDebug () << "binding fixed texture" << name;
	
	GLuint texid;

	// Delete existing texture
	if (m_fixedFrames.contains (name)) {
	    texid = m_fixedFrames[name];
	    glDeleteTextures (1, &texid);
	}

	m_context->makeCurrent (this);

	texid = bindTexture (img);
	m_fixedFrames[name] = texid;
    }
    // Mark the texture for later binding
    else {
	m_tobind[name] = img;
    }

}

void
StimWindow::addAnimatedFrame (const QString& name, const QImage& img)
{
    //qDebug () << "adding animated frame" << name;
    if (isExposed ()) {
	m_context->makeCurrent (this);

	auto& frames = m_animatedFrames[name];
	GLuint texid = bindTexture (img);
	frames.append (texid);
    }
    else {
	m_toabind[name].append (img);
    }
}

void
StimWindow::deleteAnimatedFrames (const QString& name)
{
    if (m_animatedFrames.contains (name)) {
	m_context->makeCurrent (this);

	auto& frames = m_animatedFrames[name];
	for (auto texid : frames)
	    glDeleteTextures (1, &texid);

	frames.clear ();
	m_animatedFrames.remove (name);
    }
    if (m_toabind.contains (name)) {
	auto& frames = m_toabind[name];
	frames.clear ();
	m_toabind.remove (name);
    }
}

void
StimWindow::clear ()
{
    // Clear buffered commands
    m_tobind.clear ();
    m_toabind.clear ();
    m_toshow.clear ();

    // Unset current frame
    m_currentFrame = 0;

    GLuint texid;

    // Destroy fixed frame textures
    QMapIterator<QString,GLuint> it (m_fixedFrames);
    while (it.hasNext ()) {
	it.next ();
	texid = it.value ();
	glDeleteTextures (1, &texid);
    }
    m_fixedFrames.clear ();

    // Destroy animated frame textures
    QMapIterator<QString,QVector<GLuint>> jt (m_animatedFrames);
    while (jt.hasNext ()) {
	jt.next ();
	const QVector<GLuint>& vec = jt.value ();
	for (GLuint t : vec)
	    glDeleteTextures (1, &t);
    }
    m_animatedFrames.clear ();

    // Empty current display
    renderNow ();
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

void
StimWindow::exposeEvent (QExposeEvent* evt)
{
    Q_UNUSED (evt);
    if (isExposed ())
	renderNow ();
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
StimWindow::showFullScreen ()
{
    // Search for a secondary screen
    QScreen* scr = screen ();
    for (auto s : scr->virtualSiblings ()) {
        if (s->availableGeometry ().x () != 0
                || s->availableGeometry ().y () != 0) {
            scr = s;
            break;
        }
    }
    setScreen (scr);

    // Show in full screen
    QWindow::showFullScreen ();
}

void
StimWindow::renderNow ()
{
    if (! isExposed ())
	return;

    bool needInit = false;

    // Create an OpenGL context if none existing
    if (! m_context) {
	m_context = new QOpenGLContext (this);
	m_context->setFormat (requestedFormat ());
	m_context->create ();
	needInit = true;
    }


    if (needInit) {
	m_context->makeCurrent (this);
	initializeOpenGLFunctions ();
	initialize ();
    }

    // Render the frame, and swap the buffers
    qDebug () << "rendering and swapping";
    m_context->makeCurrent (this);
    render ();
    m_context->swapBuffers (this);
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
    if (! isExposed () || tex_width == 0)
	return;

    qDebug () << "*** updating the shaders";

    GLint gl_width = width ();
    GLint gl_height = height ();

    m_context->makeCurrent (this);
    
    // Create an identity vertex shader
    glViewport (0, 0, gl_width, gl_height);

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
    
    // Remove previous vertex shader
    if (m_vshader != NULL) {
	m_program->removeShader (m_vshader);
	delete m_vshader;
    }

    // Create a new vertex shader
    m_vshader = new QOpenGLShader (QOpenGLShader::Vertex);
    m_vshader->compileSourceCode (vshader_txt);
    m_program->addShader (m_vshader);
    m_program->link ();
    m_program->bind ();

    // Get the texture location
    glActiveTexture (GL_TEXTURE0);
    m_texloc = m_program->uniformLocation ("texture");
    qDebug () << "texture located at:" << m_texloc;
    int ppos = m_program->attributeLocation ("ppos");
    qDebug () << "ppos attribute located at:" << ppos;

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

    // TODO: not sure if really necessary here
    renderNow ();
}

void
StimWindow::resizeEvent (QResizeEvent* evt)
{
    Q_UNUSED (evt);
    if (isExposed ()) {
	//renderNow ();
	updateShaders ();
    }
}

void
StimWindow::initialize ()
{
    // Create the shader program
    m_program = new QOpenGLShaderProgram (this);
    m_program->addShaderFromSourceCode (QOpenGLShader::Fragment,
	    fshader_txt);

    // Bind the deferred textures
    QMapIterator<QString,QImage> it (m_tobind);
    while (it.hasNext ()) {
	it.next ();
	addFixedFrame (it.key (), it.value ());
    }
    m_tobind.clear ();

    // Bind deferred animations
    QMapIterator<QString,QVector<QImage>> ait (m_toabind);
    while (ait.hasNext ()) {
	ait.next ();
	const QVector<QImage>& frames = ait.value ();
	for (auto& frame : frames)
	    addAnimatedFrame (ait.key (), frame);
    }
    m_toabind.clear ();

    glClearColor (0, 0, 0, 0);

    updateShaders ();

    // Show deferred frame
    if (! m_toshow.isEmpty ()) {
	if (m_fixedFrames.contains (m_toshow))
	    showFixedFrame (m_toshow);
	else
	    showAnimatedFrames (m_toshow);
	m_toshow.clear ();
    }
}

void
StimWindow::showFixedFrame (const QString& name)
{
    if (! m_fixedFrames.contains (name)
	    && ! m_tobind.contains (name)) {
	qDebug () << "??? unknown fixed frame" << name;
    }
    else if (isExposed ()) {
	qDebug () << "showing fixed frame" << name;
	m_currentFrame = m_fixedFrames[name];
	renderNow ();
    }
    else {
	m_toshow = name;
    }
}

void
StimWindow::showAnimatedFrames (const QString& name)
{
    if (! m_animatedFrames.contains (name)
	    && ! m_toabind.contains (name)) {
	qDebug () << "??? unknown animated frame" << name;
    }
    else if (isExposed ()) {

	auto& frames = m_animatedFrames[name];
	for (auto frame : frames) {
	    m_currentFrame = frame;
	    m_context->makeCurrent (this);
	    render ();
	    m_context->swapBuffers (this);
	}
    }
    else {
	m_toshow = name;
    }
}

void
StimWindow::render ()
{
    glClear (GL_COLOR_BUFFER_BIT);

    if (m_currentFrame == 0) {
	qDebug () << "render() with no effect (no frame)";
	return;
    }

    glBindTexture (GL_TEXTURE_2D, m_currentFrame);
    glUniform1i (m_texloc, 0);
    glDrawArrays (GL_TRIANGLES, 0, 6);
}
