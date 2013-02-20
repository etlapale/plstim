#include "stimwindow.h"
using namespace plstim;


StimWindow::StimWindow (QWindow* parent)
    : QWindow (parent), m_context (NULL)
{
    qDebug () << "creating a stimulus window";
    setSurfaceType (QWindow::OpenGLSurface);

    QSurfaceFormat format;
    format.setSwapBehavior (QSurfaceFormat::DoubleBuffer);
    format.setMajorVersion (3);
    format.setMinorVersion (0);
    setFormat (format);
}

StimWindow::~StimWindow ()
{
    qDebug () << "destroying a stimulus window";
}

void
StimWindow::addFixedFrame (const QString& name, const QImage& img)
{
    Q_UNUSED (name);
    Q_UNUSED (img);
    // TODO: delete existing texture

    //fixed_frames[name] = bindTexture (img);
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
    // Nothing to do if not exposed
    if (! isExposed ()) {
	qDebug () << "not yet exposed!";
	return;
    }

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
StimWindow::resizeEvent (QResizeEvent* evt)
{
    Q_UNUSED (evt);
    if (isExposed ())
	renderNow ();
}

void
StimWindow::initialize ()
{
    glClearColor (1, 0, 0, 0);
}

void
StimWindow::render ()
{
    glClear (GL_COLOR_BUFFER_BIT);
}
