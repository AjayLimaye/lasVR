#ifndef GLOBAL_H
#define GLOBAL_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QList>
#include <QVector3D>

class Global
{
 public :
  static GLuint flagSpriteTexture();
  static void removeFlagSpriteTexture();

  static GLuint infoSpriteTexture();
  static void removeInfoSpriteTexture();

  static GLuint homeSpriteTexture();
  static void removeHomeSpriteTexture();

 private :
  static GLuint m_flagSpriteTexture;
  static GLuint m_infoSpriteTexture;
  static GLuint m_homeSpriteTexture;

};

#endif
