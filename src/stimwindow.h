// src/stimwindow.h – Visual stimulus displayer
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#pragma once

#include <QtGui>
#include <QOpenGLFunctions_3_2_Core>

#ifdef HAVE_POWERMATE
#include "powermate.h"
#endif // HAVE_POWERMATE

#include "../lib/displayer.h"

namespace plstim
{
class StimWindow : public QWindow, public Displayer,
		   protected QOpenGLFunctions_3_2_Core
{
  Q_OBJECT
public:
  explicit StimWindow (QScreen* scr=nullptr);

  // Overrides from Displayer
  virtual void addFixedFrame (const QString& name, const QImage& img) override;
  virtual void showFixedFrame (const QString& name) override;
  virtual void addAnimatedFrame (const QString& name, const QImage& img) override;
  virtual void showAnimatedFrames (const QString& name) override;
  virtual void deleteAnimatedFrames (const QString& name) override;
  virtual void setTextureSize (int twidth, int theight) override;
  virtual void clear () override;
  virtual void begin() override;
  virtual void beginInline() override;
  virtual void end() override;
  virtual QScreen* displayScreen() override;
signals:
  void exposed() override;
  void keyPressed (QKeyEvent* evt) override;
  
public:
  void render ();
public slots:
  void renderNow ();
  void updateShaders ();
signals:
#ifdef HAVE_POWERMATE
  void powerMateRotation (PowerMateEvent* evt);
  void powerMateButtonPressed (PowerMateEvent* evt);
  void powerMateButtonReleased (PowerMateEvent* evt);
#endif // HAVE_POWERMATE
protected:
  virtual bool event (QEvent* evt) override;
  virtual void exposeEvent (QExposeEvent* evt) override;
  virtual void resizeEvent (QResizeEvent* evt) override;
  virtual void keyPressEvent (QKeyEvent* evt) override;

  void setupOpenGL ();
private:
  QOpenGLContext* m_context;
  QMap<QString,QOpenGLTexture*> m_fixedFrames;
  QMap<QString,QVector<QOpenGLTexture*>> m_animatedFrames;
  QOpenGLShaderProgram* m_program;
  QOpenGLShader* m_vshader;
  int tex_width;
  int tex_height;
  GLfloat vertices[12];
  int m_texloc;
  QOpenGLTexture* m_currentFrame;
  GLuint m_vao;
  GLuint m_vbo;
  bool m_opengl_initialized = false;
};
} // namespace plstim

// Local Variables:
// mode: c++
// End:
