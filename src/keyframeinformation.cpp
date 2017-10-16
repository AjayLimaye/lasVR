#include "keyframeinformation.h"
#include "global.h"

#include <QBuffer>
#include <QJsonObject>
#include <QMessageBox>

void KeyFrameInformation::setTitle(QString s) { m_title = s; }
void KeyFrameInformation::setFrameNumber(int fn) { m_frameNumber = fn; }
void KeyFrameInformation::setPosition(Vec pos) { m_position = pos; }
void KeyFrameInformation::setOrientation(Quaternion rot) { m_rotation = rot; }
void KeyFrameInformation::setImage(QImage pix) { m_image = pix; }
void KeyFrameInformation::setCurrTime(int ct) { m_currTime = ct; }


QString KeyFrameInformation::title() { return m_title; }
int KeyFrameInformation::frameNumber() { return m_frameNumber; }
Vec KeyFrameInformation::position() { return m_position; }
Quaternion KeyFrameInformation::orientation() { return m_rotation; }
QImage KeyFrameInformation::image() { return m_image; }
int KeyFrameInformation::currTime() { return m_currTime; }


// -- keyframe interpolation parameters
void KeyFrameInformation::setInterpCameraPosition(int i) {m_interpCameraPosition = i;}
void KeyFrameInformation::setInterpCameraOrientation(int i) {m_interpCameraOrientation = i;}

int KeyFrameInformation::interpCameraPosition() { return m_interpCameraPosition; }
int KeyFrameInformation::interpCameraOrientation() { return m_interpCameraOrientation; }


KeyFrameInformation::KeyFrameInformation()
{
  m_title.clear();
  m_frameNumber = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_currTime = -1;

  //m_interpCameraPosition = Enums::KFIT_Linear;
  //m_interpCameraOrientation = Enums::KFIT_Linear;
  m_interpCameraPosition = 0;
  m_interpCameraOrientation = 0;
}

void
KeyFrameInformation::clear()
{
  m_title.clear();
  m_frameNumber = 0;
  m_position = Vec(0,0,0);
  m_rotation = Quaternion(Vec(1,0,0), 0);
  m_image = QImage(100, 100, QImage::Format_RGB32);
  m_currTime = -1;

  m_interpCameraPosition = 0;
  m_interpCameraOrientation = 0;
  //m_interpCameraPosition = Enums::KFIT_Linear;
  //m_interpCameraOrientation = Enums::KFIT_Linear;
}

KeyFrameInformation::KeyFrameInformation(const KeyFrameInformation& kfi)
{
  m_title = kfi.m_title;
  m_frameNumber = kfi.m_frameNumber;
  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;
  m_image = kfi.m_image;
  m_currTime = kfi.m_currTime;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;
}

KeyFrameInformation::~KeyFrameInformation()
{
  m_title.clear();
}

KeyFrameInformation&
KeyFrameInformation::operator=(const KeyFrameInformation& kfi)
{
  m_title = kfi.m_title;
  m_frameNumber = kfi.m_frameNumber;
  m_position = kfi.m_position;
  m_rotation = kfi.m_rotation;
  m_image = kfi.m_image;
  m_currTime = kfi.m_currTime;
  m_interpCameraPosition = kfi.m_interpCameraPosition;
  m_interpCameraOrientation = kfi.m_interpCameraOrientation;

  return *this;
}

//--------------------------------
//---- load and save -------------
//--------------------------------

void
KeyFrameInformation::load(QJsonObject keyframeNode)
{
  m_title.clear();
  m_interpCameraPosition = 0;
  m_interpCameraOrientation = 0;

  m_title = keyframeNode["title"].toString();
  m_frameNumber = keyframeNode["framenumber"].toInt();
  m_currTime = keyframeNode["currtime"].toInt();

  QString str;
  QStringList words;

  str = keyframeNode["position"].toString();
  words = str.split(" ");
  m_position = Vec(words[0].toFloat(),
		   words[1].toFloat(),
		   words[2].toFloat());

  str = keyframeNode["rotation"].toString();
  words = str.split(" ");
  m_rotation = Quaternion(words[0].toFloat(),
			  words[1].toFloat(),
			  words[2].toFloat(),
			  words[3].toFloat());

  m_interpCameraPosition = keyframeNode["interpcamerapos"].toInt();
  m_interpCameraOrientation = keyframeNode["interpcamerarot"].toInt();

//  m_image = pixmapFrom(keyframeNode["image"]);
}

QJsonObject
KeyFrameInformation::save()
{
  QJsonObject keyframeNode;

  keyframeNode["title"] = m_title;
  keyframeNode["framenumber"] = m_frameNumber;
  keyframeNode["currtime"] = m_currTime;

  QString str;
  str = QString("%1 %2 %3").\
    arg(m_position.x).\
    arg(m_position.y).\
    arg(m_position.z);
  keyframeNode["position"] = str;

  str = QString("%1 %2 %3 %4").\
    arg(m_rotation[0]).\
    arg(m_rotation[1]).\
    arg(m_rotation[2]).\
    arg(m_rotation[3]);
  keyframeNode["rotation"] = str;
  
  keyframeNode["interpcamerapos"] = m_interpCameraPosition;
  keyframeNode["interpcamerarot"] = m_interpCameraOrientation;

//  auto val = jsonValFromPixmap(m_image);
//  keyframeNode["image"] = val;


  return keyframeNode;
}

QJsonValue
KeyFrameInformation::jsonValFromPixmap(QImage p) {
  QByteArray data;
  QBuffer buffer { &data };
  buffer.open(QIODevice::WriteOnly);
  p.save(&buffer, "PNG");
  auto encoded = buffer.data().toBase64();
  return QJsonValue(QString::fromLatin1(encoded));
}

QImage
KeyFrameInformation::pixmapFrom(QJsonValue val) {
  QByteArray encoded = val.toString().toLatin1();
  QImage p;
  p = QImage::fromData(QByteArray::fromBase64(encoded), "PNG");
  return p;
}
