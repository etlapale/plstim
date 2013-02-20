#include <iomanip>
#include <sstream>

using namespace std;

#include <QGLWidget>

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
    qDebug () << "creating a stimulus window";
    setSurfaceType (QWindow::OpenGLSurface);

    QSurfaceFormat fmt;
    fmt.setSwapBehavior (QSurfaceFormat::DoubleBuffer);
    fmt.setMajorVersion (3);
    fmt.setMinorVersion (0);
    setFormat (fmt);
}

StimWindow::~StimWindow ()
{
    qDebug () << "destroying a stimulus window";
}

void
StimWindow::addFixedFrame (const QString& name, const QImage& img)
{
    if (isExposed ()) {
	qDebug () << "binding texture" << name;
	
	GLuint texid;

	// Delete existing texture
	if (m_fixedFrames.contains (name)) {
	    texid = m_fixedFrames[name];
	    glDeleteTextures (1, &texid);
	}

	m_context->makeCurrent (this);

	glGenTextures (1, &texid);
	glBindTexture (GL_TEXTURE_2D, texid);
	QImage glimg = QGLWidget::convertToGLFormat (img);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA,
		glimg.width (), glimg.height (), 0, GL_RGBA,
		GL_UNSIGNED_BYTE, glimg.bits ());
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_fixedFrames[name] = texid;
    }
    // Mark the texture for later binding
    else {
	m_tobind[name] = img;
    }

}

bool
StimWindow::event (QEvent* evt)
{
    switch (evt->type ()) {
    case QEvent::UpdateRequest:
	renderNow ();
	return true;
    default:
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

    m_context->makeCurrent (this);

    if (needInit) {
	initializeOpenGLFunctions ();
	initialize ();
    }

    // Render the frame, and swap the buffers
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

    glClearColor (1, 0, 0, 0);

    updateShaders ();

    // Show deferred frame
    if (! m_toshow.isEmpty ()) {
	showFixedFrame (m_toshow);
	m_toshow.clear ();
    }
}

void
StimWindow::showFixedFrame (const QString& name)
{
    if (! m_fixedFrames.contains (name)
	    && ! m_tobind.contains (name)) {
	qDebug () << "??? unknown fixed frame" << name;
	return;
    }

    if (isExposed ()) {
	qDebug () << "@@@ showing fixed frame" << name;

	//m_context->makeCurrent (this);

	m_currentFrame = m_fixedFrames[name];

	renderNow ();
    }
    else {
	qDebug () << "!!! deferring fixed frame" << name;
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

    qDebug () << "effective rendering";

    glBindTexture (GL_TEXTURE_2D, m_currentFrame);
    glUniform1i (m_texloc, 0);
    glDrawArrays (GL_TRIANGLES, 0, 6);
}
