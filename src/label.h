#ifndef LABEL_H
#define LABEL_H

#include <QGLViewer/vec.h>
#include <QGLViewer/camera.h>
using namespace qglviewer;

#include <QMatrix4x4>
#include <QVector3D>

class Label
{
 public :
  Label();
  ~Label();

  void setCaption(QString cap) { m_caption = cap; }
  void setPosition(Vec pos) { m_position = pos; }
  void setProximity(float p) { m_proximity = p; }
  void setColor(Vec col) { m_color = col; }
  void setFontSize(float fs) { m_fontSize = fs; }
  void setLink(QString lnk) { m_linkData = lnk; }

  void setTreeInfo(QList<float>);

  void drawLabel(Camera*);
  void drawLabel(QVector3D,
		 QVector3D, QVector3D, QVector3D,
		 QMatrix4x4, QMatrix4x4,
		 QMatrix4x4, QMatrix4x4,
		 float, QVector3D,
		 bool);

  bool checkLink(Camera*, QPoint);
  QString linkData() { return m_linkData; }

  void setGlobalMinMax(Vec, Vec);

  void stickToGround();
  float checkHit(QMatrix4x4, QMatrix4x4,
		float, QVector3D);

  GLuint glTexture() { return m_glTexture; }
  QSize textureSize() { return QSize(m_texWd, m_texHt); }

 private :

  QList<float> m_treeInfo;

  QString m_caption;
  Vec m_position;
  float m_proximity;
  Vec m_color;
  float m_fontSize;
  QString m_linkData;

  int m_texWd, m_texHt;

  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;

  float *m_vertData;
  float *m_boxData;

  int m_hitDur;
  Vec m_hitC;

  void genVertData();

  void showTreeInfoPosition(QMatrix4x4, bool);

  void createBox();
  void drawBox(QMatrix4x4, QVector3D, bool, bool);
};

#endif
