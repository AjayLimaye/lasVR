#ifndef ICONLIBRARY_H
#define ICONLIBRARY_H

#include <GL/glew.h>
#include <QImage>
#include <QRectF>
#include <QStringList>

class IconLibrary : public QObject
{
 public :
  static void init();
  static void clear();

  static void loadIcons(QString);

  static GLuint iconTexture() { return m_iconTex; }
  static QStringList iconList() { return m_iconList; }
  static int width() { return m_texWd; }
  static int height() { return m_texHt; }
  static QList<QRect> iconShape() { return m_iconShape; }
  static QList<QRectF> iconGeometry() { return m_iconGeom; }
  
  static QRect iconShape(int);
  static QRect iconShape(QString);
  
  static QRectF iconGeometry(int);
  static QRectF iconGeometry(QString);
  
 private :
  static GLuint m_iconTex;
  static QString m_iconDir;
  static int m_texWd, m_texHt;
  static QImage m_iconImage;
  static QStringList m_iconList;
  static QList<QRect> m_iconShape;
  static QList<QRectF> m_iconGeom;

};

#endif
