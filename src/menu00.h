#ifndef MENU00_H
#define MENU00_H

#include "glewinitialisation.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Menu00 : public QObject
{
  Q_OBJECT

 public :
  Menu00();
  ~Menu00();

  void draw(QMatrix4x4, QMatrix4x4);

  int checkOptions(QMatrix4x4, QMatrix4x4);

  void setPlay(bool p) { m_play = p; }

  bool isVisible() { return m_visible; }
  void setVisible(bool v)
  {
    m_visible = v;
    if (!m_visible)
      m_pointingToMenu = false;
  }

  bool pointingToMenu() { return m_pointingToMenu; }

  void setTimeStep(QString);

 private :
  bool m_visible;
  bool m_pointingToMenu;

  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  int m_texHt, m_texWd;
  float *m_vertData;

  GLuint m_stepTexture;

  int m_selected;
  bool m_play;

  QList<QColor> m_colorList;
  QList<QString> m_menuList;
  QList<QRect> m_relGeom;
  QList<QRectF> m_optionsGeom;

  float m_mapScale;
  QQuaternion m_mapRot;

  void genVertData();

  QVector3D projectPin(QMatrix4x4, QMatrix4x4);
};

#endif
