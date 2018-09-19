#ifndef STATICFUNCTIONS_H
#define STATICFUNCTIONS_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QtMath>
#include <QQuaternion>

class StaticFunctions
{
 public :

  static QWidget* visibleWidget();

  static void drawBox(Vec, Vec);
  static void drawBox(QList<Vec>);

  static float projectionSize(Vec, Vec, Vec, float);

  static void renderText(int, int,
			 QString, QFont,
			 QColor, QColor,
			 bool left = false);
  static QImage renderText(QString, QFont,
			   QColor, QColor,
			   bool border = true);
  static void renderText(int, int,
			 QStringList, QFont,
			 QColor, QColor,
			 bool left = false);
  static QImage renderText(QStringList, QFont,
			   QColor, QColor,
			   bool border = true);

  static void renderRotatedText(int, int,
				QString, QFont,
				QColor, QColor,
				float, bool);

  static QSize getImageSize(int, int);


  static void convertFromGLImage(QImage&);

  static void pushOrthoView(float, float, float, float);
  static void popOrthoView();
  
  static QQuaternion powQuat(QQuaternion, float);
  static QQuaternion expQuat(QQuaternion);
  static QQuaternion lnQuat(QQuaternion);
  static QQuaternion scaleQuat(QQuaternion, float);

  static Quaternion getRotationBetweenVectors(Vec, Vec);
  static QQuaternion getRotationBetweenVectors(QVector3D, QVector3D);
  static float getAngleBetweenVectors(QVector3D, QVector3D);

  static float easeIn(float);
  static float easeOut(float);
  static float smoothstep(float);
  static float smoothstep(float, float, float);

  static bool intersectRayPlane(QVector3D, QVector3D,
				QVector3D, QVector3D,
				QVector3D, QVector3D,
				float&, float&);

  static float intersectRayBox(QVector3D, QVector3D,
			       QVector3D, QVector3D);

  static bool checkExtension(QString, const char*);

  static Vec clampVec(Vec, Vec, Vec);
  static Vec maxVec(Vec, Vec);
  static Vec minVec(Vec, Vec);
};

#endif
