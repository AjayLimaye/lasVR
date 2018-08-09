#include "staticfunctions.h"
#include <QtMath>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>

QWidget*
StaticFunctions::visibleWidget()
{
  QWidget *pw = 0;
  QWidgetList wlist = QApplication::topLevelWidgets();
  for (int w=0; w<wlist.count(); w++)
    {
      if (wlist[w]->isVisible())
	{
	  pw = wlist[w];
	  break;
	}      
    }
  return pw;
}

void
StaticFunctions::drawBox(QList<Vec> v)
{
  glBegin(GL_QUADS);  
  for(int i=0; i<16; i++)
    glVertex3f(v[i].x, v[i].y, v[i].z);
  glEnd();
}

void
StaticFunctions::drawBox(Vec subvolmin, Vec subvolmax)
{
  QList<Vec> v;
  v << Vec(subvolmin.x, subvolmin.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmin.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmax.y, subvolmin.z);
  v << Vec(subvolmin.x, subvolmax.y, subvolmin.z);
  v << Vec(subvolmin.x, subvolmin.y, subvolmax.z);
  v << Vec(subvolmax.x, subvolmin.y, subvolmax.z);
  v << Vec(subvolmax.x, subvolmax.y, subvolmax.z);
  v << Vec(subvolmin.x, subvolmax.y, subvolmax.z);
  v << Vec(subvolmin.x, subvolmax.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmax.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmax.y, subvolmax.z);
  v << Vec(subvolmin.x, subvolmax.y, subvolmax.z);
  v << Vec(subvolmin.x, subvolmin.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmin.y, subvolmin.z);
  v << Vec(subvolmax.x, subvolmin.y, subvolmax.z);
  v << Vec(subvolmin.x, subvolmin.y, subvolmax.z);  
  drawBox(v);

////---------------------
//  glBegin(GL_QUADS);  
//  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
//  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
//  glEnd();
//  
//  // FRONT 
//  glBegin(GL_QUADS);  
//  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);
//  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
//  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
//  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
//  glEnd();
//  
//  // TOP
//  glBegin(GL_QUADS);  
//  glVertex3f(subvolmin.x, subvolmax.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmax.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmax.y, subvolmax.z);
//  glVertex3f(subvolmin.x, subvolmax.y, subvolmax.z);
//  glEnd();
//  
//  // BOTTOM
//  glBegin(GL_QUADS);  
//  glVertex3f(subvolmin.x, subvolmin.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmin.y, subvolmin.z);
//  glVertex3f(subvolmax.x, subvolmin.y, subvolmax.z);
//  glVertex3f(subvolmin.x, subvolmin.y, subvolmax.z);  
//  glEnd();  
}

float
StaticFunctions::projectionSize(Vec cpos, Vec bmin, Vec bmax, float projFactor)
{
  Vec bsize = (bmax-bmin)/2;
  Vec bcen = (bmax+bmin)/2;
  float dist = (cpos-bcen).norm();
  float rad = qMax(bsize.x, qMax(bsize.y, bsize.z));
  //float slope = qTan(fov/2);
  //float projFactor = (0.5*screenHeight)/slope;

  float screenPixRad = projFactor*rad/dist;

  //float screenPixRad = projFactor*rad/qSqrt(dist*dist-rad*rad);

  return screenPixRad;
}

void
StaticFunctions::renderText(int x, int y,
			    QString str, QFont font,
			    QColor bcolor, QColor color,
			    bool left)
{
  QFontMetrics metric(font);
  int ht = metric.height()+9;
  int wd = metric.width(str)+11;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(Qt::transparent);
  //  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::TextAntialiasing, true);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setFont(font);
  p.setBrush(bcolor);
  p.drawRoundedRect(1, 1, wd-2, ht-2, 5, 5);

  p.drawText(5, ht-metric.descent()-5, str);

  QImage mimg = img.mirrored();
  if (!left)
    glRasterPos2i(x-wd/2, y+ht/2);
  else
    glRasterPos2i(x, y);

  glDrawPixels(wd, ht, GL_RGBA, GL_UNSIGNED_BYTE, mimg.bits());
}

QImage
StaticFunctions::renderText(QString str, QFont font,
			    QColor bcolor, QColor color, bool border)
{
  QFontMetrics metric(font);
  int ht = metric.height()+9;
  int wd = metric.width(str)+11;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::TextAntialiasing, true);
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  p.setFont(font);

  if (border)
    p.drawRoundedRect(1, 1, wd-2, ht-2, 5, 5);
  
  p.drawText(5, ht-metric.descent()-5, str);

  QImage mimg = img.mirrored();

  return mimg;
}

void
StaticFunctions::renderRotatedText(int x, int y,
				   QString str, QFont font,
				   QColor bcolor, QColor color,
				   float angle, bool ydir)
{
  QFontMetrics metric(font);
  int ht = metric.height()+1;
  int wd = metric.width(str)+3;
  QImage img(wd, ht, QImage::Format_ARGB32);
  img.fill(bcolor);
  QPainter p(&img);
  p.setRenderHints(QPainter::Antialiasing |
		   QPainter::TextAntialiasing |
		   QPainter::SmoothPixmapTransform);
  p.setPen(color);
  p.setFont(font);
  p.drawText(1, ht-metric.descent()-1, str);
  QImage mimg = img.mirrored();
  QMatrix mat;
  mat.rotate(angle);
  QImage pimg = mimg.transformed(mat, Qt::SmoothTransformation);
  if (ydir) // (0,0) is bottom left
    glRasterPos2i(x-pimg.width()/2, y+pimg.height()/2);
  else // (0,0) is top left
    glRasterPos2i(x-pimg.width()/2, y-pimg.height()/2);
  glDrawPixels(pimg.width(), pimg.height(),
	       GL_RGBA, GL_UNSIGNED_BYTE,
	       pimg.bits());
}

QSize
StaticFunctions::getImageSize(int width, int height)
{
  QSize imgSize = QSize(width, height);

  QStringList items;
  items << QString("%1 %2 (Current)").arg(width).arg(height);
  items << "320 200 (CGA)";
  items << "320 240 (QVGA)";
  items << "640 480 (VGA)";
  items << "720 480 (NTSC)";
  items << "720 576 (PAL)";
  items << "800 480 (WVGA)";
  items << "800 600 (SVGA)";
  items << "854 480 (WVGA 16:9)";
  items << "1024 600 (WSVGA)";
  items << "1024 768 (XGA)";
  items << "1280 720 (720p 16:9)";
  items << "1280 768 (WXGA)";
  items << "1280 1024 (SXGA)";
  items << "1366 768 (16:9)";
  items << "1400 1050 (SXGA+)";
  items << "1600 1200 (UXGA)";
  items << "1920 1080 (1080p 16:9)";
  items << "1920 1200 (WUXGA)";

  bool ok;
  QString str;
  str = QInputDialog::getItem(0,
			      "Image size",
			      "Image Size",
			      items,
			      0,
			      true, // text is editable
			      &ok);
  
  if (ok)
    {
      QStringList strlist = str.split(" ", QString::SkipEmptyParts);
      int x=0, y=0;
      if (strlist.count() > 1)
	{
	  x = strlist[0].toInt(&ok);
	  y = strlist[1].toInt(&ok);
	}

      if (x > 0 && y > 0)
	imgSize = QSize(x, y);
      else
	QMessageBox::critical(0, "Image Size",
			      "Image Size improperly set : "+str);
    }
  
  return imgSize;
}

void // Qt version
StaticFunctions::convertFromGLImage(QImage &img)
{
  int w = img.width();
  int h = img.height();

  if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
    {
      // OpenGL gives RGBA; Qt wants ARGB
      uint *p = (uint*)img.bits();
      uint *end = p + w*h;
      while (p < end)
	{
	  uint a = *p << 24;
	  *p = (*p >> 8) | a;
	  p++;
	}
      // This is an old legacy fix for PowerPC based Macs, which
      // we shouldn't remove
      while (p < end)
	{
	  *p = 0xff000000 | (*p>>8);
	  ++p;
	}
    }
  else
    {
      // OpenGL gives ABGR (i.e. RGBA backwards); Qt wants ARGB
      QImage res = img.rgbSwapped();
      img = res;
    }
  img = img.mirrored();
}

void
StaticFunctions::pushOrthoView(float x, float y, float width, float height)
{
  glViewport(x,y, width, height);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0,width, 0,height, 0,1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

void
StaticFunctions::popOrthoView()
{
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
}

//------------------------
///
// Quaternion^n - for a rotation quaternion,
// this is equivalent to rotating this by itself n times.
// This should only work for unit quaternions.
//
QQuaternion
StaticFunctions::powQuat(QQuaternion q0, float n)
{
  QQuaternion q = q0;
  q.normalize();

  // q^n = exp(n*ln(q))

  q = lnQuat(q);
  q = scaleQuat(q, n);
  q = expQuat(q);

  return q;
}
QQuaternion
StaticFunctions::expQuat(QQuaternion q0)
{
  float x = q0.x();
  float y = q0.y();
  float z = q0.z();
  float w = q0.scalar();

  float r  = qSqrt(x*x+y*y+z*z);
  float et = qExp(w);
  float s  = r>=0.00001f ? et*qSin(r)/r : 0;
  
  w=et*qCos(r);
  x*=s;
  y*=s;
  z*=s;

  return QQuaternion(w, x, y, z);
}
QQuaternion
StaticFunctions::lnQuat(QQuaternion q0)
{
  float x = q0.x();
  float y = q0.y();
  float z = q0.z();
  float w = q0.scalar();
  
  float r  = qSqrt(x*x+y*y+z*z);
  float t  = r>0.00001f ? qAtan2(r,w)/r : 0;

  w=0.5f*qLn(w*w+x*x+y*y+z*z);
  x*=t;
  y*=t;
  z*=t;

  return QQuaternion(w, x, y, z);
}
QQuaternion
StaticFunctions::scaleQuat(QQuaternion q0, float scale)
{
  float x = q0.x();
  float y = q0.y();
  float z = q0.z();
  float w = q0.scalar();

  w*=scale;
  x*=scale;
  y*=scale;
  z*=scale;

  return QQuaternion(w, x, y, z);
}
//------------------------

Quaternion
StaticFunctions::getRotationBetweenVectors(Vec p, Vec q)
{
  Vec axis = p ^ q;
  float angle = 0;

  if (axis.squaredNorm() == 0) // parallel vectors
    {
      axis = p;
      angle = 0;
      return Quaternion(p,0);
    }

  axis.normalize();

  float cost = (p*q)/(p.norm()*q.norm());
  cost = qMax(-1.0f, qMin(1.0f, cost));
  angle = acos(cost);

  return Quaternion(axis, angle);
}

QQuaternion
StaticFunctions::getRotationBetweenVectors(QVector3D p, QVector3D q)
{
  QVector3D axis = QVector3D::normal(p, q);

  if (axis.lengthSquared() == 0) // parallel vectors
    return QQuaternion();

  float angle = getAngleBetweenVectors(p,q);
  //float cost = QVector3D::dotProduct(p,q)/(p.length()*q.length());
  //cost = qMax(-1.0f, qMin(1.0f, cost));
  //float angle = acos(cost);

  return QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle));
}

float
StaticFunctions::getAngleBetweenVectors(QVector3D p, QVector3D q)
{
  float cost = QVector3D::dotProduct(p,q)/(p.length()*q.length());
  cost = qMax(-1.0f, qMin(1.0f, cost));
  return acos(cost);
}

float
StaticFunctions::easeIn(float a)
{
  return a*a;
}

float
StaticFunctions::easeOut(float a)
{
  float b = (1-a);
  return 1.0f-b*b;
}

float
StaticFunctions::smoothstep(float a)
{
  return a*a*(3.0f-2.0f*a);
}

float
StaticFunctions::smoothstep(float min, float max, float v)
{
  if (v <= min) return 0.0f;
  if (v >= max) return 1.0f;
  float a = (v-min)/(max-min);
  return a*a*(3.0f-2.0f*a);
}


bool
StaticFunctions::intersectRayPlane(QVector3D v0, QVector3D vy,
				   QVector3D vx, QVector3D pn,
				   QVector3D rayO, QVector3D ray,
				   float &x, float &y)
{
  float deno = QVector3D::dotProduct(ray, pn);

//  if (deno > -0.00001) // point along same direction
//    return false;

  float t = -QVector3D::dotProduct(rayO - v0, pn) / deno;
  if (t >= 0)
    {
      QVector3D pt = rayO + t*ray;
      float lx = vx.length();
      float ly = vy.length();
      x = QVector3D::dotProduct(pt-v0, vx.normalized())/lx;
      y = QVector3D::dotProduct(pt-v0, vy.normalized())/ly;
      if (x >= 0 && x <= 1 &&
	  y >= 0 && y <= 1)
	return true;
    } 

  return false;
}

// https://tavianator.com/fast-branchless-raybounding-box-intersections-part-2-nans/
float
StaticFunctions::intersectRayBox(QVector3D bmin, QVector3D bmax,
				 QVector3D rayO, QVector3D rayD)
{
  float t1, t2, tmin, tmax;

  QVector3D rayDInv = QVector3D(1/rayD.x(),1/rayD.y(),1/rayD.z());
  QVector3D brMin = (bmin-rayO)*rayDInv;
  QVector3D brMax = (bmax-rayO)*rayDInv;

  tmin = qMin(brMin.x(), brMax.x());
  tmax = qMax(brMin.x(), brMax.x());
  tmin = qMax(tmin, qMin(brMin.y(), brMax.y()));
  tmax = qMin(tmax, qMax(brMin.y(), brMax.y()));
  tmin = qMax(tmin, qMin(brMin.z(), brMax.z()));
  tmax = qMin(tmax, qMax(brMin.z(), brMax.z()));
  
  //return tmax > qMax(tmin, 0.0f);
  if (tmax > qMax(tmin, 0.0f))
    return tmin;
  else
    return -100000;
}

bool
StaticFunctions::checkExtension(QString flnm, const char *ext)
{
  bool ok = true;
  int extlen = strlen(ext);

  QByteArray exten = flnm.toLatin1().right(extlen);
  if (exten != ext)
    ok = false;

  return ok;
}

Vec
StaticFunctions::clampVec(Vec minv, Vec maxv, Vec v)
{
  Vec cv;
  cv.x = qBound(minv.x, v.x, maxv.x);
  cv.y = qBound(minv.y, v.y, maxv.y);
  cv.z = qBound(minv.z, v.z, maxv.z);

  return cv;
}

Vec
StaticFunctions::maxVec(Vec a, Vec b)
{
  Vec cv;
  cv.x = qMax(a.x, b.x);
  cv.y = qMax(a.y, b.y);
  cv.z = qMax(a.z, b.z);

  return cv;
}

Vec
StaticFunctions::minVec(Vec a, Vec b)
{
  Vec cv;
  cv.x = qMin(a.x, b.x);
  cv.y = qMin(a.y, b.y);
  cv.z = qMin(a.z, b.z);

  return cv;
}

