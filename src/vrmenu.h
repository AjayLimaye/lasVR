#ifndef VRMENU_H
#define VRMENU_H

#include "glewinitialisation.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "map.h"
#include "menu01.h"
#include "menu02.h"

class VrMenu : public QObject
{
  Q_OBJECT

 public :
  VrMenu();
  ~VrMenu();

  void draw(QMatrix4x4, QMatrix4x4, bool);

  void setImage(QImage);

  void setCurrPos(QVector3D, QVector3D);

  void setTeleports(QList<QVector3D>);

  int checkTeleport(QMatrix4x4, QMatrix4x4);

  int checkOptions(QMatrix4x4, QMatrix4x4, int);

  QVector2D pinPoint2D();

  void setShowMap(bool);

  void setPlay(bool);

  QVector3D pinPoint();

  void setCurrentMenu(QString);

  QStringList menuList() { return m_menus.keys(); }

  void setTimeStep(QString);
  void setDataShown(QString);

  bool pointingToMenu();

  void setPlayMenu(bool);
  void setPlayButton(bool);

  void setAnnotationIcons(QStringList);
  QString currentAnnotationIcon();
  
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

  void toggle(QString, float);
  void toggle(QString, QString);

 private :
  QMap<QString, QObject*> m_menus;
  QString m_currMenu;

  bool m_showMap;

  QVector3D m_currPos;
  QVector3D m_currPosD;

  QList<QVector3D> m_teleports;
};

#endif
