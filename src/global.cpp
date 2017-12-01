#include "global.h"

bool Global::m_playFrames = false;
bool Global::playFrames() { return m_playFrames; }
void Global::setPlayFrames(bool pf) { m_playFrames = pf; }

int Global::m_keyFrameNumber = -1;
void Global::setCurrentKeyFrame(int k) { m_keyFrameNumber = k; }
int Global::currentKeyFrame() { return m_keyFrameNumber; }

QString Global::m_previousDirectory = "";
QString Global::previousDirectory() {return m_previousDirectory;}
void Global::setPreviousDirectory(QString d) {m_previousDirectory = d;}

GLuint Global::m_circleSpriteTexture = 0;
void Global::removeCircleSpriteTexture()
{
  if (m_circleSpriteTexture)
    glDeleteTextures( 1, &m_circleSpriteTexture );
  m_circleSpriteTexture = 0;
}
GLuint Global::circleSpriteTexture()
{
  if (m_circleSpriteTexture)
    return m_circleSpriteTexture;

  glGenTextures( 1, &m_circleSpriteTexture );

  QImage circle(":/images/circle.png");
  int texsize = circle.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_circleSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       circle.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_circleSpriteTexture;
}

GLuint Global::m_infoSpriteTexture = 0;
void Global::removeInfoSpriteTexture()
{
  if (m_infoSpriteTexture)
    glDeleteTextures( 1, &m_infoSpriteTexture );
  m_infoSpriteTexture = 0;
}
GLuint Global::infoSpriteTexture()
{
  if (m_infoSpriteTexture)
    return m_infoSpriteTexture;

  glGenTextures( 1, &m_infoSpriteTexture );

  QImage info(":/images/info.png");
  int texsize = info.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_infoSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       info.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_infoSpriteTexture;
}

GLuint Global::m_homeSpriteTexture = 0;
void Global::removeHomeSpriteTexture()
{
  if (m_homeSpriteTexture)
    glDeleteTextures( 1, &m_homeSpriteTexture );
  m_homeSpriteTexture = 0;
}
GLuint Global::homeSpriteTexture()
{
  if (m_homeSpriteTexture)
    return m_homeSpriteTexture;

  glGenTextures( 1, &m_homeSpriteTexture );

  QImage home(":/images/home.png");
  int texsize = home.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_homeSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       home.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_homeSpriteTexture;
}

GLuint Global::m_boxSpriteTexture = 0;
void Global::removeBoxSpriteTexture()
{
  if (m_boxSpriteTexture)
    glDeleteTextures( 1, &m_boxSpriteTexture );
  m_boxSpriteTexture = 0;
}
GLuint Global::boxSpriteTexture()
{
  if (m_boxSpriteTexture)
    return m_boxSpriteTexture;

  glGenTextures( 1, &m_boxSpriteTexture );

  QImage box(":/images/box.png");
  int texsize = box.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_boxSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       box.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_boxSpriteTexture;
}

Camera Global::m_menuCam = Camera();
void Global::setMenuCam(Camera cam) { m_menuCam = cam; }
Vec Global::menuCamProjectedCoordinatesOf(Vec p)
{
  return m_menuCam.projectedCoordinatesOf(p);
}
QVector3D Global::menuCamProjectedCoordinatesOf(QVector3D p)
{
  Vec pp = m_menuCam.projectedCoordinatesOf(Vec(p.x(),p.y(),p.z()));
  return QVector3D(pp.x, pp.y, pp.z);
}
Vec Global::menuCamUnprojectedCoordinatesOf(Vec p)
{
  return m_menuCam.unprojectedCoordinatesOf(p);
}
QVector3D Global::menuCamUnprojectedCoordinatesOf(QVector3D p)
{
  Vec pp = m_menuCam.unprojectedCoordinatesOf(Vec(p.x(),p.y(),p.z()));
  return QVector3D(pp.x, pp.y, pp.z);
}

int Global::m_screenWidth = 0;
int Global::m_screenHeight = 0;
void
Global::setScreenSize(int wd, int ht)
{
  m_screenWidth = wd;
  m_screenHeight = ht;
}
int Global::screenWidth() { return m_screenWidth; }
int Global::screenHeight() { return m_screenHeight; }

float* Global::m_depthBuffer = 0;
void Global::setDepthBuffer(float *db) { m_depthBuffer = db; }


Vec
Global::stickToGround(Vec pt)
{
  Vec ptGround = pt;
  Vec ptProjected = menuCamProjectedCoordinatesOf(pt);
  int x = ptProjected.x;
  int y = ptProjected.y;
  if (x > 0 && x < m_screenWidth-1 &&
      y > 0 && y < m_screenHeight-1)
    {
      float z = m_depthBuffer[(m_screenHeight-1-y)*m_screenWidth + x];      
      if (z > 0 && z < 1.0)
	ptGround = menuCamUnprojectedCoordinatesOf(Vec(x,y,z));
    }

  return ptGround;
}
QVector3D
Global::stickToGround(QVector3D pt)
{
  QVector3D ptGround = pt;
  QVector3D ptProjected = menuCamProjectedCoordinatesOf(pt);
  int x = ptProjected.x();
  int y = ptProjected.y();
  if (x > 0 && x < m_screenWidth-1 &&
      y > 0 && y < m_screenHeight-1)
    {
      float z = m_depthBuffer[(m_screenHeight-1-y)*m_screenWidth + x];      
      if (z > 0 && z < 1.0)
	ptGround = menuCamUnprojectedCoordinatesOf(QVector3D(x,y,z));
    }

  return ptGround;
}

QStatusBar* Global::m_statusBar = 0;
QStatusBar* Global::statusBar() { return m_statusBar; }
void Global::setStatusBar(QStatusBar *sb) { m_statusBar = sb; }

QProgressBar* Global::m_progressBar = 0;
QProgressBar* Global::progressBar()
{
  if (!m_progressBar)
    m_progressBar = new QProgressBar();
  return m_progressBar;
}
void Global::hideProgressBar() { m_statusBar->hide(); }
void Global::showProgressBar() { m_statusBar->show(); }


QList<Vec> Global::m_colorMap;
void Global::setColorMap(QList<Vec> cm) { m_colorMap = cm; }
QList<Vec> Global::getColorMap() { return m_colorMap; }


QList<GLuint> Global::m_trisetVBOs;
void
Global::setTrisetVBOs(QList<GLuint> vbo)
{
  clearTrisetVBOs();
  m_trisetVBOs = vbo;
}
void
Global::clearTrisetVBOs()
{
  for(int i=0; i<m_trisetVBOs.count()/3; i++)
    {
      glDeleteVertexArrays(1, &m_trisetVBOs[3*i+0]);
      glDeleteBuffers(1, &m_trisetVBOs[3*i+1]);
      glDeleteBuffers(1, &m_trisetVBOs[3*i+2]);
    }
  
  m_trisetVBOs.clear();
}
GLuint
Global::trisetVBO(int i, int j)
{
  if (3*i+j < m_trisetVBOs.count())
    return m_trisetVBOs[3*i+j];
  else
    return 0;
}
