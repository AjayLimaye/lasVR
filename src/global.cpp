#include "global.h"

GLuint Global::m_flagSpriteTexture = 0;
void Global::removeFlagSpriteTexture()
{
  if (m_flagSpriteTexture)
    glDeleteTextures( 1, &m_flagSpriteTexture );
  m_flagSpriteTexture = 0;
}
GLuint Global::flagSpriteTexture()
{
  if (m_flagSpriteTexture)
    return m_flagSpriteTexture;

  glGenTextures( 1, &m_flagSpriteTexture );

  QImage flag(":/images/flag.png");
  int texsize = flag.height();
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_flagSpriteTexture);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	       texsize, texsize, 0,
	       GL_BGRA, GL_UNSIGNED_BYTE,
	       flag.bits());
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  return m_flagSpriteTexture;
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
