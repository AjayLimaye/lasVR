#ifndef GLOBAL_H
#define GLOBAL_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QList>
#include <QVector3D>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QProgressDialog>

class Global
{
 public :
  static GLuint circleSpriteTexture();
  static void removeCircleSpriteTexture();

  static GLuint infoSpriteTexture();
  static void removeInfoSpriteTexture();

  static GLuint homeSpriteTexture();
  static void removeHomeSpriteTexture();

  static GLuint boxSpriteTexture();
  static void removeBoxSpriteTexture();

  static void setMenuCam(Camera);
  static Vec menuCamProjectedCoordinatesOf(Vec);
  static Vec menuCamUnprojectedCoordinatesOf(Vec);
  static QVector3D menuCamProjectedCoordinatesOf(QVector3D);
  static QVector3D menuCamUnprojectedCoordinatesOf(QVector3D);

  static void setScreenSize(int, int);
  static int screenWidth();
  static int screenHeight();

  static void setDepthBuffer(float*);
  static Vec stickToGround(Vec);
  static QVector3D stickToGround(QVector3D);

  static void setStatusBar(QStatusBar*);
  static QStatusBar* statusBar();

  static QProgressBar* progressBar();
  static void hideProgressBar();
  static void showProgressBar();


 private :
  static GLuint m_circleSpriteTexture;
  static GLuint m_infoSpriteTexture;
  static GLuint m_homeSpriteTexture;
  static GLuint m_boxSpriteTexture;

  static Camera m_menuCam;

  static int m_screenWidth, m_screenHeight;
  static float *m_depthBuffer;

  static QProgressBar *m_progressBar;
  static QStatusBar *m_statusBar;
};

#endif
