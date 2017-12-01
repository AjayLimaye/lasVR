#ifndef LOADERTHREAD_H
#define LOADERTHREAD_H

#include "glhiddenwidget.h"

class LoaderThread : public QObject
{
  Q_OBJECT

 public :
  LoaderThread() {};

  GLHiddenWidget* gl() { return m_gl; };

 public slots :
   void start(QGLFormat, QWidget*, QGLWidget*);
   void loadPointsToVBO();
   void stopLoading();
   void updateView();

 signals :
   void vboLoaded(int, qint64);
   void vboLoadedAll(int, qint64);
   void message(QString);
       
   void meshLoadedAll();
   
 private :
  GLHiddenWidget *m_gl;
};


#endif
