#ifndef KEYFRAMEINFORMATION_H
#define KEYFRAMEINFORMATION_H

#include <GL/glew.h>
#include <QObject>
#include <QImage>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class KeyFrameInformation
{
 public :
  KeyFrameInformation();
  KeyFrameInformation(const KeyFrameInformation&);
  ~KeyFrameInformation();

  KeyFrameInformation& operator=(const KeyFrameInformation&);

  void clear();

  void load(fstream&);
  void save(fstream&);

  void setTitle(QString);
  void setFrameNumber(int);
  void setPosition(Vec);
  void setOrientation(Quaternion);
  void setImage(QImage);
  void setCurrTime(int);
  
  QString title();
  int frameNumber();
  Vec position();
  Quaternion orientation();
  QImage image();
  int currTime();

  // -- keyframe interpolation parameters
  void setInterpCameraPosition(int);
  void setInterpCameraOrientation(int);

  int interpCameraPosition();
  int interpCameraOrientation();

 private :
  QString m_title;
  int m_frameNumber;
  Vec m_position;
  Quaternion m_rotation;
  QImage m_image;
  int m_currTime;

  //-- keyframe interpolation parameters
  int m_interpCameraPosition;
  int m_interpCameraOrientation;
};

#endif
