#ifndef MAP_H
#define MAP_H

#include "glewinitialisation.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Map : public QObject
{
  Q_OBJECT

 public :
  Map();
  ~Map();

  void draw(QMatrix4x4, QMatrix4x4);

  void setImage(QImage);

  void setCurrPos(QVector3D v, QVector3D vd)
  {
    m_currPos = v;
    m_currPosD = vd;
  }

  void setTeleports(QList<QVector3D>);

  int checkTeleport(QMatrix4x4, QMatrix4x4);

  QVector2D pinPoint2D() { return m_pinPt2D; }
  QVector3D pinPoint() { return m_pinPt; }

  bool isVisible() { return m_visible; }
  void setVisible(bool v) { m_visible = v; }

 private :
  bool m_visible;

  QImage m_image;

  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  int m_texHt, m_texWd;
  float *m_vertData;

  QVector3D m_currPos;
  QVector3D m_currPosD;

  QVector3D m_pinPt;
  QVector2D m_pinPt2D;

  GLuint m_glVA;
  GLuint m_glVB;
  int m_VBpoints;

  int m_teleportTouched;
  QList<QVector3D> m_teleports;

  int m_bc;

  float m_mapScale;
  QQuaternion m_mapRot;

  void genVertData();
  void genOtherVertData();

  QVector3D projectPin(QMatrix4x4, QMatrix4x4);
};

#endif
