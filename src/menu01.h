#ifndef MENU01_H
#define MENU01_H

#include "glewinitialisation.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Menu01 : public QObject
{
  Q_OBJECT

 public :
  Menu01();
  ~Menu01();

  void draw(QMatrix4x4, QMatrix4x4, bool);

  QVector3D pinPoint() { return m_pinPt; }

  int checkOptions(QMatrix4x4, QMatrix4x4, bool);

  bool isVisible() { return m_visible; }
  void setVisible(bool v)
  {
    m_visible = v;
    if (!m_visible)
      m_pointingToMenu = false;
  }

  bool pointingToMenu() { return m_pointingToMenu; }

 signals :
  void resetModel();
  void updatePointSize(int);
  void updateScale(int);
  void updateSoftShadows(bool);
  void updateEdges(bool);

 private :
  bool m_visible;
  bool m_pointingToMenu;
  QVector3D m_pinPt;

  QImage m_image;

  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  int m_texHt, m_texWd;
  float *m_vertData;

  int m_selected;
  bool m_softShadows;
  bool m_edges;

  QList<QString> m_menuList;
  QList<QRect> m_relGeom;
  QList<QRectF> m_optionsGeom;

  float m_mapScale;
  QQuaternion m_mapRot;

  void genVertData();

  QVector3D projectPin(QMatrix4x4, QMatrix4x4);
};

#endif
