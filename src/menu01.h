#ifndef MENU01_H
#define MENU01_H

#include "glewinitialisation.h"
#include "button.h"

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

  int checkOptions(QMatrix4x4, QMatrix4x4, int);

  bool isVisible() { return m_visible; }
  void setVisible(bool v)
  {
    m_visible = v;
    if (!m_visible)
      m_pointingToMenu = false;
  }

  bool pointingToMenu() { return m_pointingToMenu; }

  void setPlayMenu(bool b) { m_playMenu = b; }
  void setPlayButton(bool b) { m_playButton = b; }
  void setPlay(bool b) { m_play = b; }

  void setShowMap(bool);

  void setTimeStep(QString);
  void setDataShown(QString);

 signals :
  void resetModel();
  void updateMap();
  void updatePointSize(int);
  void updateScale(int);
  void updateSoftShadows(bool);
  void updateEdges(bool);
  void updateSpheres(bool);
  void gotoFirstStep();
  void gotoPreviousStep();
  void gotoNextStep();
  void playPressed(bool);

 private :
  bool m_visible;
  bool m_pointingToMenu;
  QVector3D m_pinPt;

  int m_texHt, m_texWd;
  QList<QRect> m_menuDim;
  QImage m_image;

  GLuint m_stepTexture;
  GLuint m_dataNameTexture;
  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  float *m_vertData;

  int m_selected;
  bool m_softShadows;
  bool m_edges;
  bool m_spheres;
  bool m_play;
  bool m_playMenu;
  bool m_playButton;
  bool m_showMap;

  QList<QString> m_menuList;
  QList<QRect> m_relGeom;
  QList<QRectF> m_optionsGeom;

  QList<Button> m_buttons;

  QList<QRectF> m_dataGeom;

  float m_mapScale;
  QQuaternion m_mapRot;

  void genVertData();

  QVector3D projectPin(QMatrix4x4, QMatrix4x4);

  void showText(GLuint, QRectF,
		QVector3D, QVector3D,
		QVector3D, QVector3D,
		float,float,float,float,
		Vec);
};

#endif
