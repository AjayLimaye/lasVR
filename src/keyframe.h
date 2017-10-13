#ifndef KEYFRAME_H
#define KEYFRAME_H

#include "keyframeinformation.h"

class KeyFrame : public QObject
{
  Q_OBJECT

 public :
  KeyFrame();
  ~KeyFrame();

  void clear();

  void load(fstream&);

  void save(fstream&);

  void draw(float);

  KeyFrameInformation interpolate(int);

  KeyFrameInformation keyFrameInfo(int);

  int searchCaption(QStringList);

  void saveProject(Vec, Quaternion, QImage, int);

  void interpolateAt(int, float,
		     Vec&, Quaternion&,
		     int&,
		     KeyFrameInformation&,
		     float&);

  int numberOfKeyFrames();

 public slots :
  void playFrameNumber(int);
  void updateKeyFrameInterpolator();
  void removeKeyFrame(int);
  void removeKeyFrames(int, int);
  void setKeyFrameNumber(int, int);
  void setKeyFrameNumbers(QList<int>);
  void reorder(QList<int>);
  void copyFrame(int);
  void pasteFrame(int);
  void pasteFrameOnTop(int);
  void pasteFrameOnTop(int, int);
  void editFrameInterpolation(int);
  void replaceKeyFrameImage(int, QImage);
  void playSavedKeyFrame();
  void checkKeyFrameNumbers();
  void setKeyFrame(Vec, Quaternion, int, QImage, int);

 signals :
  void loadKeyframes(QList<int>, QList<QImage>);
  void updateLookFrom(Vec, Quaternion, int);
  void updateGL();
  void setImage(int, QImage);
  void currentFrameChanged(int);
  void replaceKeyFrameImage(int);
  void addKeyFrameNumbers(QList<int>);

 private :
  QList<KeyFrameInformation*> m_keyFrameInfo;

  KeyFrameInformation m_savedKeyFrame;
  KeyFrameInformation m_copyKeyFrame;

  QList<Vec> m_tgP;
  QList<Quaternion> m_tgQ;
  void computeTangents();
  Vec interpolatePosition(int, int, float);
  Quaternion interpolateOrientation(int, int, float);

  QMap<QString, QPair<QVariant, bool> > copyProperties(QString);
};

#endif
