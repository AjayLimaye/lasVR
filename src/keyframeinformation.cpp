#include "keyframeinformation.h"
#include "global.h"

#include <QBuffer>

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
KeyFrameInformation::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];

  m_title.clear();
  m_interpCameraPosition = 0;
  m_interpCameraOrientation = 0;
  //m_interpCameraPosition = Enums::KFIT_Linear;
  //m_interpCameraOrientation = Enums::KFIT_Linear;

  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "keyframeend") == 0)
	done = true;
      else if (strcmp(keyword, "framenumber") == 0)
	fin.read((char*)&m_frameNumber, sizeof(int));
      else if (strcmp(keyword, "currtime") == 0)
	fin.read((char*)&m_currTime, sizeof(int));
      else if (strcmp(keyword, "title") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  m_title = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "position") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  m_position = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "rotation") == 0)
	{
	  Vec axis;
	  float angle;
	  fin.read((char*)&f, 3*sizeof(float));
	  axis = Vec(f[0], f[1], f[2]);	
	  fin.read((char*)&angle, sizeof(float));
	  m_rotation.setAxisAngle(axis, angle);
	}
      else if (strcmp(keyword, "image") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  unsigned char *tmp = new unsigned char[n+1];
	  fin.read((char*)tmp, n);
	  m_image = QImage::fromData(tmp, n);	 
	  delete [] tmp;
	}
      else if (strcmp(keyword, "interpcamerapos") == 0)
	fin.read((char*)&m_interpCameraPosition, sizeof(int));
      else if (strcmp(keyword, "interpcamerarot") == 0)
	fin.read((char*)&m_interpCameraOrientation, sizeof(int));
    }
}

void
KeyFrameInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];
  int len;

  memset(keyword, 0, 100);
  sprintf(keyword, "keyframestart");
  fout.write((char*)keyword, strlen(keyword)+1);


  memset(keyword, 0, 100);
  sprintf(keyword, "title");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = m_title.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)m_title.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "framenumber");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_frameNumber, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "currtime");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_currTime, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = m_position.x;
  f[1] = m_position.y;
  f[2] = m_position.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "rotation");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec axis;
  qreal angle;
  float fangle;
  m_rotation.getAxisAngle(axis, angle);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fangle = angle;
  fout.write((char*)&f, 3*sizeof(float));
  fout.write((char*)&fangle, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "image");
  fout.write((char*)keyword, strlen(keyword)+1);
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  m_image.save(&buffer, "PNG");
  int n = bytes.size();
  fout.write((char*)&n, sizeof(int));
  fout.write((char*)bytes.data(), n);


  memset(keyword, 0, 100);
  sprintf(keyword, "interpcamerapos");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCameraPosition, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "interpcamerarot");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&m_interpCameraOrientation, sizeof(int));



  memset(keyword, 0, 100);
  sprintf(keyword, "keyframeend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

