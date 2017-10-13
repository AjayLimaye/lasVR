#include "keyframe.h"
#include "global.h"
//#include "propertyeditor.h"
#include "staticfunctions.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>

int KeyFrame::numberOfKeyFrames() { return m_keyFrameInfo.count(); }

KeyFrame::KeyFrame()
{
  m_keyFrameInfo.clear();
  m_tgP.clear();
  m_tgQ.clear();
}

KeyFrame::~KeyFrame()
{
  for(int i=0; i<m_keyFrameInfo.size(); i++)
    delete m_keyFrameInfo[i];
  m_keyFrameInfo.clear();

  m_tgP.clear();
  m_tgQ.clear();
}

void
KeyFrame::clear()
{
  for(int i=0; i<m_keyFrameInfo.size(); i++)
    delete m_keyFrameInfo[i];
  m_keyFrameInfo.clear();

  m_tgP.clear();
  m_tgQ.clear();

  m_savedKeyFrame.clear();
  m_copyKeyFrame.clear();
}

KeyFrameInformation
KeyFrame::keyFrameInfo(int i)
{
  if (i < m_keyFrameInfo.size())
    return *m_keyFrameInfo[i];
  else
    return KeyFrameInformation();
}

void
KeyFrame::setKeyFrameNumbers(QList<int> fno)
{
  for(int i=0; i<fno.size(); i++)
    m_keyFrameInfo[i]->setFrameNumber(fno[i]);
}

void
KeyFrame::setKeyFrameNumber(int selected, int frameNumber)
{
  m_keyFrameInfo[selected]->setFrameNumber(frameNumber);
}

void
KeyFrame::checkKeyFrameNumbers()
{
  // check overlapping keyframes
  QString mesg;
  for(int i=0; i<m_keyFrameInfo.count()-1; i++)
    for(int j=i+1; j<m_keyFrameInfo.count(); j++)
      {
	if (m_keyFrameInfo[i]->frameNumber() ==
	    m_keyFrameInfo[j]->frameNumber())
	  mesg += QString("Clash of keyframes at : %1").\
	          arg(m_keyFrameInfo[i]->frameNumber());
      }
  if (!mesg.isEmpty())
    QMessageBox::information(0, "Keyframe clash", mesg);
}

void
KeyFrame::reorder(QList<int> sorted)
{
  QList<KeyFrameInformation*> kfInfo(m_keyFrameInfo);
  for(int i=0; i<sorted.size(); i++)
    m_keyFrameInfo[i] = kfInfo[sorted[i]];

  kfInfo.clear();

  computeTangents();
}


void
KeyFrame::saveProject(Vec pos, Quaternion rot,
		      QImage image, int currTime)
{
  m_savedKeyFrame.clear();
  m_savedKeyFrame.setFrameNumber(-1);
  m_savedKeyFrame.setPosition(pos);
  m_savedKeyFrame.setOrientation(rot);
  m_savedKeyFrame.setImage(image);
  m_savedKeyFrame.setCurrTime(currTime);
}

void
KeyFrame::setKeyFrame(Vec pos, Quaternion rot,
		      int frameNumber,
		      QImage image,
		      int currTime)
{
  // -- save keyframe first into a m_savedKeyFrame
  saveProject(pos, rot, image, currTime);

  bool found = false;
  int kfn = -1;
  for(int i=0; i<m_keyFrameInfo.size(); i++)
    {
      if (m_keyFrameInfo[i]->frameNumber() == frameNumber)
	{
	  kfn= i;
	  found = true;
	  break;
	}
    }	

  KeyFrameInformation *kfi;
  QString title;
  title = QString("Keyframe %1").arg(frameNumber);
  if (found)
    title = m_keyFrameInfo[kfn]->title();

  if (!found)
    { // append a new node
      kfn = m_keyFrameInfo.count();
      kfi = new KeyFrameInformation();
      m_keyFrameInfo.append(kfi);
    }

  kfi = m_keyFrameInfo[kfn];
  kfi->setTitle(title);
  kfi->setFrameNumber(frameNumber);
  kfi->setPosition(pos);
  kfi->setOrientation(rot);
  kfi->setCurrTime(currTime);

  emit setImage(kfn, image);  
  
  updateKeyFrameInterpolator();
}

void
KeyFrame::updateKeyFrameInterpolator()
{
  computeTangents();

  emit updateGL();
  qApp->processEvents();
}

void
KeyFrame::removeKeyFrame(int fno)
{
  delete m_keyFrameInfo[fno];

  m_keyFrameInfo.removeAt(fno);

  updateKeyFrameInterpolator();
}

void
KeyFrame::removeKeyFrames(int f0, int f1)
{
  for (int i=f0; i<=f1; i++)
    {
      delete m_keyFrameInfo[f0];
      m_keyFrameInfo.removeAt(f0);
    }

  updateKeyFrameInterpolator();
}


void
KeyFrame::interpolateAt(int kf, float frc,
			Vec &pos, Quaternion &rot,
			int &ct,
			KeyFrameInformation &kfi,
			float &volInterp)
{
  volInterp = 0.0f;

  float rfrc;

  if (kf < numberOfKeyFrames()-1)
    {
      //rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCameraPosition(), frc);
      rfrc = frc;
      pos = interpolatePosition(kf, kf+1, rfrc);

      //rfrc = StaticFunctions::remapKeyframe(m_keyFrameInfo[kf]->interpCameraOrientation(), frc);
      rfrc = frc;
      rot = interpolateOrientation(kf, kf+1, rfrc);

      ct = (1-rfrc)*m_keyFrameInfo[kf]->currTime() + rfrc*m_keyFrameInfo[kf+1]->currTime();
    }
  else
    {
      pos = m_keyFrameInfo[kf]->position();
      rot = m_keyFrameInfo[kf]->orientation();
      ct =  m_keyFrameInfo[kf]->currTime();
    }  
}

void
KeyFrame::playSavedKeyFrame()
{
  Vec pos = m_savedKeyFrame.position();
  Quaternion rot = m_savedKeyFrame.orientation();
  int ct = m_savedKeyFrame.currTime();

  emit updateLookFrom(pos, rot, ct);

  //emit updateGL();
  qApp->processEvents();  
}

void
KeyFrame::playFrameNumber(int fno)
{
  if (numberOfKeyFrames() == 0)
    return;

  emit currentFrameChanged(fno);
  qApp->processEvents();

  int maxFrame = m_keyFrameInfo[numberOfKeyFrames()-1]->frameNumber();
  int minFrame = m_keyFrameInfo[0]->frameNumber();

  if (fno > maxFrame || fno < minFrame)
    return;

  
  for(int kf=0; kf<numberOfKeyFrames(); kf++)
    {
      if (fno == m_keyFrameInfo[kf]->frameNumber())
	{
	  Vec pos = m_keyFrameInfo[kf]->position();
	  Quaternion rot = m_keyFrameInfo[kf]->orientation();
	  int ct = m_keyFrameInfo[kf]->currTime();
	  emit updateLookFrom(pos, rot, ct);

	  //emit updateGL();
	  qApp->processEvents();
  
	  return;
	}
    }


  float frc = 0;
  int i = 0;
  for(int kf=1; kf<numberOfKeyFrames(); kf++)
    {
      if (fno <= m_keyFrameInfo[kf]->frameNumber())
	{
	  i = kf-1;

	  if ( m_keyFrameInfo[i+1]->frameNumber() >
	       m_keyFrameInfo[i]->frameNumber())
	    frc = ((float)(fno-m_keyFrameInfo[i]->frameNumber()) /
		   (float)(m_keyFrameInfo[i+1]->frameNumber() -
			   m_keyFrameInfo[i]->frameNumber()));
	  else
	    frc = 1;

	  break;
	}
    }

  Vec pos;
  Quaternion rot;
  int ct;
  KeyFrameInformation keyFrameInfo;
  float volInterp;
  interpolateAt(i, frc,
		pos, rot, ct,
		keyFrameInfo,
		volInterp);

  emit updateLookFrom(pos, rot, ct);

  //emit updateGL();
  qApp->processEvents();
}

//--------------------------------
//---- load and save -------------
//--------------------------------
void
KeyFrame::load(fstream &fin)
{
  char keyword[100];

  clear();

  int n;
  fin.read((char*)&n, sizeof(int));

  bool savedFirst = false;
  while (!fin.eof())
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "done") == 0)
	break;

      if (strcmp(keyword, "keyframestart") == 0)
	{
	  // the zeroeth keyframe should be moved to m_savedKeyFrame
	  if (!savedFirst)
	    {
	      m_savedKeyFrame.load(fin);
	      savedFirst = true;
	    }
	  else
	    {
	      KeyFrameInformation *kfi = new KeyFrameInformation();
	      kfi->load(fin);
	      m_keyFrameInfo.append(kfi);
	    }
	}
    }

  // --- build list for keyframe editor
  QList<int> framenumbers;
  QList<QImage> images;
  for (int i=0; i<m_keyFrameInfo.size(); i++)
    {
      framenumbers.append(m_keyFrameInfo[i]->frameNumber());
      images.append(m_keyFrameInfo[i]->image());
    }

  emit loadKeyframes(framenumbers, images);
  qApp->processEvents();


  //playSavedKeyFrame();
  // ----------------------------------
}

void
KeyFrame::save(fstream& fout)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "keyframes");
  fout.write((char*)keyword, strlen(keyword)+1);

  int n = numberOfKeyFrames();
  fout.write((char*)&n, sizeof(int));

  m_savedKeyFrame.save(fout);

  for(int kf=0; kf<numberOfKeyFrames(); kf++)
    m_keyFrameInfo[kf]->save(fout);

  sprintf(keyword, "done");
  fout.write((char*)keyword, strlen(keyword)+1);
}


void
KeyFrame::computeTangents()
{
  int nkf = numberOfKeyFrames();
  if (nkf <= 1)
    return;

  // -- flip orientation if necessary --
  Quaternion prevQ = m_keyFrameInfo[0]->orientation();
  for(int kf=1; kf<nkf; kf++)
    {
      Quaternion currQ = m_keyFrameInfo[kf]->orientation();
      if (Quaternion::dot(prevQ, currQ) < 0.0)
	{
	  currQ.negate();
	  m_keyFrameInfo[kf]->setOrientation(currQ);
	}
      prevQ = currQ;
    }
  // ------------------------------------

  m_tgP.clear();
  m_tgQ.clear();
  Vec prevP, nextP, currP;
  Quaternion nextQ, currQ;
  
  currP = m_keyFrameInfo[0]->position();
  currQ = m_keyFrameInfo[0]->orientation();
  prevP = currP;
  prevQ = currQ;
  for(int kf=0; kf<nkf; kf++)
    {
      if (kf < nkf-1)
	{
	  nextP = m_keyFrameInfo[kf+1]->position();
	  nextQ = m_keyFrameInfo[kf+1]->orientation();
	}
      Vec tgP = 0.5*(nextP - prevP);
      Quaternion tgQ = Quaternion::squadTangent(prevQ, currQ, nextQ);

      m_tgP.append(tgP);
      m_tgQ.append(tgQ);


      prevP = currP;
      prevQ = currQ;
      currP = nextP;
      currQ = nextQ;
    }
}

Vec
KeyFrame::interpolatePosition(int kf1, int kf2, float frc)
{
  Vec pos = m_keyFrameInfo[kf1]->position();

  Vec diff = m_keyFrameInfo[kf2]->position() -
	     m_keyFrameInfo[kf1]->position();

  float len = diff.squaredNorm();
  if (len > 0.1)
    {
//      if (Global::interpolationType(Global::CameraPositionInterpolation))
//	{ // spline interpolation of position
	  Vec v1 = 3*diff - 2*m_tgP[kf1] - m_tgP[kf2];
	  Vec v2 = -2*diff + m_tgP[kf1] + m_tgP[kf2];
	  
	  pos += frc*(m_tgP[kf1] + frc*(v1+frc*v2));
//	}
//      else // linear interpolation of position
//	pos += frc*diff;
    }

  return pos;
}

Quaternion
KeyFrame::interpolateOrientation(int kf1, int kf2, float frc)
{
  Quaternion q1 = m_keyFrameInfo[kf1]->orientation();
  Quaternion q2 = m_keyFrameInfo[kf2]->orientation();

  double a0 = q1[0];
  double a1 = q1[1];
  double a2 = q1[2];
  double a3 = q1[3];

  double b0 = q2[0];
  double b1 = q2[1];
  double b2 = q2[2];
  double b3 = q2[3];

  if (fabs(a0-b0) > 0.0001 ||
      fabs(a1-b1) > 0.0001 ||
      fabs(a2-b2) > 0.0001 ||
      fabs(a3-b3) > 0.0001)
    {
      Quaternion q;
      q = Quaternion::squad(q1,
			    m_tgQ[kf1],
			    m_tgQ[kf2],
			    q2,
			    frc);
      return q;
    }
  else
    return q1;
}

void
KeyFrame::copyFrame(int kfn)
{
  if (kfn < m_keyFrameInfo.count())
    m_copyKeyFrame = *m_keyFrameInfo[kfn];
  else
    QMessageBox::information(0, "Error",
			     QString("KeyFrame %1 not present for copying").arg(kfn));
}
void
KeyFrame::pasteFrame(int frameNumber)
{
  KeyFrameInformation *kfi;
  kfi = new KeyFrameInformation();
  *kfi = m_copyKeyFrame;
  kfi->setFrameNumber(frameNumber);
  m_keyFrameInfo.append(kfi);

  updateKeyFrameInterpolator();
}

void
KeyFrame::editFrameInterpolation(int kfn)
{
//  KeyFrameInformation *kfi = m_keyFrameInfo[kfn];
//
//  PropertyEditor propertyEditor;
//  QMap<QString, QVariantList> plist;
//  
//  QStringList keys;
//  keys << "camera position";
//  keys << "camera orientation";
//
//  for(int ik=0; ik<keys.count(); ik++)
//    {
//      if (keys[ik] != "separator")
//	{
//	  QVariantList vlist;  
//	  vlist.clear();
//	  if (keys[ik] == "title")
//	    {
//	      vlist << QVariant("string");
//	      vlist << kfi->title();
//	    }
//	  else
//	    {
//	      vlist << QVariant("combobox");
//
//	      if (keys[ik] == "camera position") vlist << kfi->interpCameraPosition();
//	      else if (keys[ik] == "camera orientation") vlist << kfi->interpCameraOrientation();
//	      
//	      vlist << QVariant("linear");
//	      vlist << QVariant("smoothstep");
//	      vlist << QVariant("easein");
//	      vlist << QVariant("easeout");
//	      vlist << QVariant("none");
//	    }
//
//	  plist[keys[ik]] = vlist;
//	}
//    }
//
//  propertyEditor.set("Keyframe Interpolation Parameters",
//		     plist, keys,
//		     false); // do not add reset buttons
//  propertyEditor.resize(300, 500);
//	      
//  QMap<QString, QPair<QVariant, bool> > vmap;
//
//  if (propertyEditor.exec() == QDialog::Accepted)
//    vmap = propertyEditor.get();
//  else
//    {
////      QMessageBox::information(0,
////			       "Keyframe Interpolation Parameters",
////			       "No interpolation parameter changed");
//      return;
//    }
//
//  keys = vmap.keys();
//  
//  for(int ik=0; ik<keys.count(); ik++)
//    {
//      QPair<QVariant, bool> pair = vmap.value(keys[ik]);
//      
//      
//      if (pair.second)
//	{
//	  if (keys[ik] == "title")
//	    {
//	      QString title = pair.first.toString();
//	      kfi->setTitle(title);
//	    }
//	  else
//	    {
//	      int flag = pair.first.toInt();
//	      if (keys[ik] == "camera position") kfi->setInterpCameraPosition(flag);
//	      else if (keys[ik] == "camera orientation") kfi->setInterpCameraOrientation(flag);
//	    }
//	}
//    }
//
////  QMessageBox::information(0, "Parameters", "Changed");
}

void
KeyFrame::pasteFrameOnTop(int keyFrameNumber)
{
//  if (keyFrameNumber < 0 ||
//      keyFrameNumber >= m_keyFrameInfo.count() ||
//      keyFrameNumber >= m_cameraList.count())
//    {
//      QMessageBox::information(0, "",
//	  QString("%1 keyframe does not exist").arg(keyFrameNumber));
//      return;
//    }
//      
//  QMap<QString, QPair<QVariant, bool> > vmap;
//  vmap = copyProperties(QString("Paste Parameters to keyframe on %1").\
//			arg(m_keyFrameInfo[keyFrameNumber]->frameNumber()));
//
//  QStringList keys = vmap.keys();
//  if (keys.count() == 0)
//    return;
//  
//	      
//  KeyFrameInformation *kfi = m_keyFrameInfo[keyFrameNumber];
//
//  for(int ik=0; ik<keys.count(); ik++)
//    {
//      QPair<QVariant, bool> pair = vmap.value(keys[ik]);      
//      
//      if (pair.second)
//	{
//	  if (keys[ik] == "camera position")
//	    kfi->setPosition(m_copyKeyFrame.position());
//	  else if (keys[ik] == "camera orientation")
//	    kfi->setOrientation(m_copyKeyFrame.orientation());  
//	}
//    }
//
//  updateKeyFrameInterpolator();
//
//  playFrameNumber(kfi->frameNumber());
//
//  emit replaceKeyFrameImage(keyFrameNumber);
}

void
KeyFrame::pasteFrameOnTop(int startKF, int endKF)
{
//  QMap<QString, QPair<QVariant, bool> > vmap;
//  vmap = copyProperties(QString("Paste Parameters to keyframes between %1 - %2").\
//			arg(m_keyFrameInfo[startKF]->frameNumber()).\
//			arg(m_keyFrameInfo[endKF]->frameNumber()));
//
//  QStringList keys = vmap.keys();
//  if (keys.count() == 0)
//    return;
//  
//
//  for(int keyFrameNumber=startKF; keyFrameNumber<=endKF; keyFrameNumber++)
//    {	      
//      KeyFrameInformation *kfi = m_keyFrameInfo[keyFrameNumber];
//
//      for(int ik=0; ik<keys.count(); ik++)
//	{
//	  QPair<QVariant, bool> pair = vmap.value(keys[ik]);
//            
//	  if (pair.second)
//	    {
//	      if (keys[ik] == "camera position")
//		kfi->setPosition(m_copyKeyFrame.position());
//	      else if (keys[ik] == "camera orientation")
//		kfi->setOrientation(m_copyKeyFrame.orientation());  
//	    }
//	}
//      
//      updateKeyFrameInterpolator();
//      
//      playFrameNumber(kfi->frameNumber());
//      
//      emit replaceKeyFrameImage(keyFrameNumber);
//      qApp->processEvents();
//    }
}

QMap<QString, QPair<QVariant, bool> >
KeyFrame::copyProperties(QString title)
{
//  PropertyEditor propertyEditor;
//  QMap<QString, QVariantList> plist;
//  
//  QVariantList vlist;
//
//  vlist.clear();
//  vlist << QVariant("checkbox");
//  vlist << QVariant(false);
//  plist["camera position"] = vlist;
//
//  vlist.clear();
//  vlist << QVariant("checkbox");
//  vlist << QVariant(false);
//  plist["camera orientation"] = vlist;
//
//
//  QStringList keys;
//  keys << "camera position";
//  keys << "camera orientation";
//
//
//  propertyEditor.set(title,
//		     plist, keys,
//		     false); // do not add reset buttons
//  propertyEditor.resize(300, 500);
//
//	      
  QMap<QString, QPair<QVariant, bool> > vmap;
//
//  if (propertyEditor.exec() == QDialog::Accepted)
//    vmap = propertyEditor.get();
//  else
//    {
//      QMessageBox::information(0,
//			       title,
//			       "No parameters pasted");
//    }

  return vmap;
}



void
KeyFrame::replaceKeyFrameImage(int kfn, QImage img)
{
  m_keyFrameInfo[kfn]->setImage(img);
}
