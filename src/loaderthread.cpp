#include "loaderthread.h"
#include <QMessageBox>

void
LoaderThread::start(QGLFormat format, QWidget *parent, QGLWidget *sw)
{
  m_gl = new GLHiddenWidget(format, parent, sw);
  m_gl->setVisible(false);

  connect(m_gl, SIGNAL(vboLoaded(int, qint64)),
	  this, SIGNAL(vboLoaded(int, qint64)));
  connect(m_gl, SIGNAL(vboLoadedAll(int, qint64)),
	  this, SIGNAL(vboLoadedAll(int, qint64)));
  connect(m_gl, SIGNAL(message(QString)),
	  this, SIGNAL(message(QString)));
}

void
LoaderThread::stopLoading()
{
  m_gl->stopLoading();
}

void
LoaderThread::loadPointsToVBO()
{
  m_gl->makeCurrent();
  m_gl->loadPointsToVBO();
  m_gl->doneCurrent();
}

void
LoaderThread::updateView()
{
  m_gl->makeCurrent();
  m_gl->updateView();
  m_gl->doneCurrent();
}
