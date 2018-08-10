#ifndef MENU02_H
#define MENU02_H

#include "glewinitialisation.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector2D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include "button.h"

class Menu02 : public QObject
{
  Q_OBJECT

 public :
  Menu02();
  ~Menu02();

  void draw(QMatrix4x4, QMatrix4x4, bool);

  void setIcons(QStringList);
  
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

  void setValue(QString, float);
  float value(QString);

  QString currentIcon();
  
 signals :
  void resetModel();

  void toggle(QString, float);

 private :
  bool m_visible;
  bool m_pointingToMenu;
  QVector3D m_pinPt;

  int m_texHt, m_texWd;
  QList<QRect> m_menuDim;
  QImage m_image;

  GLuint m_stepTexture;
  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;
  float *m_vertData;
  float *m_textData;

  int m_selected;

  QVector3D m_up, m_vleft, m_vright, m_vfrontActual;
    
  QList<QString> m_menuList;
  QList<QRect> m_relGeom;
  QList<QRectF> m_optionsGeom;

  QRectF m_stepGeom;

  QList<Button> m_buttons;
  QStringList m_icons;
  
  float m_menuScale;
  float m_menuDist;

  QMatrix4x4 m_menuMat;
  
  void genVertData();

  QVector3D projectPin(QMatrix4x4, QMatrix4x4);

  void showText(GLuint, QRectF,
		QVector3D, QVector3D,
		QVector3D, QVector3D,
		float,float,float,float,
		Vec);
};

#endif
