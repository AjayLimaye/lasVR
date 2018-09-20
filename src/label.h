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
  void setIcon(QString con) { m_icon = con; }
  void setPosition(Vec pos) { m_positionO = m_position = pos; }
  void setProximity(float p) { m_proximity = p; }
  void setColor(Vec col) { m_color = col; }
  void setFontSize(float fs) { m_fontSize = fs; }
  void setLink(QString lnk) { m_linkData = lnk; }

  Vec position() { return m_positionO; }
  QString icon() { return m_icon; }
  QString caption() { return m_caption; }
  
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

  void setXform(float, float,
		Vec, Vec,
		Quaternion, Vec);

  void stickToGround();
  float checkHit(QMatrix4x4, QMatrix4x4,
		float, QVector3D);

  GLuint glTexture() { return m_glTexture; }
  QSize textureSize() { return QSize(m_texWd, m_texHt); }

  void genVertData();

  void regenerate();
  
 private :

  QList<float> m_treeInfo;
  QImage m_captionImage;
  QString m_caption;
  QString m_icon;
  Vec m_positionO;
  Vec m_position;
  float m_proximity;
  Vec m_color;
  float m_fontSize;
  QString m_linkData;

  int m_texWd, m_texHt;
  float m_tx0, m_ty0, m_tx1, m_ty1;
  
  GLuint m_glTexture;
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;

  float *m_vertData;
  float *m_boxData;

  int m_hitDur;
  Vec m_hitC;

  Vec m_globalMin;
  Quaternion m_rotation;
  Vec m_shift, m_offset;
  float m_scale, m_scaleCloudJs;
  Vec m_xformCen;

  void createBox();
  void drawBox(QMatrix4x4, QVector3D, bool, bool);

  Vec xformPoint(Vec);
};

#endif
