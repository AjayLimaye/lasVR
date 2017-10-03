#include "staticfunctions.h"
#include "pointcloud.h"

#include <QtGui>
#include <QMessageBox>
#include <QProgressDialog>
#include <QFileDialog>
#include <QTextStream>

#include "laszip_dll.h"

PointCloud::PointCloud()
{
  m_filenames.clear(); 

  m_scale = 1.0;
  m_scaleCloudJs = 1.0;
  m_shift = Vec(0,0,0);
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_colorPresent = true;
  m_classPresent = false;
  m_xformPresent = false;
  m_priority = 0;
  m_time = -1;

  m_spacing = 1.0;
  m_octreeMin = Vec(0,0,0);
  m_octreeMax = Vec(0,0,0);
  m_tightOctreeMin = Vec(0,0,0);
  m_tightOctreeMax = Vec(0,0,0);

  //m_dpv = 3;
  //m_dpv = 4;
  m_dpv = 6;

  m_fileFormat = true; // LAS
  m_pointAttrib.clear();
  m_attribBytes = 0;

  m_tiles.clear();
  m_labels.clear();

  m_vData.clear();

  m_showMap = false;
  m_gravity = false;
  m_skybox = false;
  m_playButton = true;
  m_groundHeight = 0.16;
  m_teleportScale = 1.0;
  m_showSphere = false;
  m_pointType = true;
  m_loadAll = false;

  m_nearHitLabel = -1;
  m_nearHit = -1000;

  m_ignoreScaling = false;
}

PointCloud::~PointCloud()
{
  reset();
}

void
PointCloud::reset()
{
  m_filenames.clear();

  m_scale = 1.0;
  m_scaleCloudJs = 1.0;
  m_shift = Vec(0,0,0);
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_colorPresent = true;
  m_classPresent = false;
  m_xformPresent = false;
  m_priority = 0;
  m_time = -1;

  m_spacing = 1.0;
  m_octreeMin = m_octreeMax = Vec(0,0,0);

  m_tiles.clear();
  m_labels.clear();

  m_vData.clear();

  m_octreeMin = m_octreeMax = Vec(0,0,0);
  m_octreeMin = Vec(0,0,0);
  m_octreeMax = Vec(0,0,0);
  m_tightOctreeMin = Vec(0,0,0);
  m_tightOctreeMax = Vec(0,0,0);

  m_fileFormat = true; // LAS
  m_pointAttrib.clear();
  m_attribBytes = 0;
}

void
PointCloud::loadAll()
{
  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->loadData();
    }
}

int
PointCloud::maxTime()
{
  int maxTime = 0;

  for(int d=0; d<m_tiles.count(); d++)
    maxTime = qMax(maxTime, m_tiles[d]->time());

  return maxTime;
}

void
PointCloud::loadPoTreeMultiDir(QString dirname, int timestep, bool ignoreScaling)
{
  m_scale = 1.0;
  m_scaleCloudJs = 1.0;
  m_shift = Vec(0,0,0);
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_colorPresent = true;
  m_classPresent = false;
  m_xformPresent = false;
  m_priority = 0;
  m_time = timestep;

  m_ignoreScaling = ignoreScaling;
  
  if (QDir(dirname).exists("mod.json"))
    {
      QString jsonfile = QDir(dirname).absoluteFilePath("mod.json");
      loadModJson(jsonfile);
    }

  if (QDir(dirname).exists("labels.json"))
    {
      QString jsonfile = QDir(dirname).absoluteFilePath("labels.json");
      loadLabelsJson(jsonfile);
    }
  
  if (QDir(dirname).exists("labels.csv"))
    {
      QString csvfile = QDir(dirname).absoluteFilePath("labels.csv");
      loadLabelsCSV(csvfile);
    }
  
  QStringList dirnames;

  if (QDir(dirname).exists("cloud.js"))
    dirnames << dirname;
  else
    {
      QDirIterator topDirIter(dirname,
			      QDir::Dirs | QDir::Readable |
			      QDir::NoDotAndDotDot | QDir::NoSymLinks,
			      QDirIterator::NoIteratorFlags);
      
      while(topDirIter.hasNext())
	dirnames << topDirIter.next();
    }
  
  loadMultipleTiles(dirnames);


//  m_labels[0]->setPosition(m_tiles[0]->bmin());
//  m_labels[1]->setPosition(m_tiles[0]->bmax());
//  Vec bmin = m_tiles[0]->bmin();
//  Vec bmax = m_tiles[0]->bmax();
//  QMessageBox::information(0, "", QString("%1 %2 %3\n %4 %5 %6").	\
//			   arg(bmin.x).arg(bmin.y).arg(bmin.z).	\
//			   arg(bmax.x).arg(bmax.y).arg(bmax.z));
}

int
PointCloud::setLevel(OctreeNode *node, int lvl)
{
  node->setLevel(lvl);

  int levelsBelow = 0;
  for (int k=0; k<8; k++)
    {
      OctreeNode *cnode = node->getChild(k);
      if (cnode)
	levelsBelow = qMax(levelsBelow, setLevel(cnode, lvl+1)+1);
    }

  node->setLevelsBelow(levelsBelow);
  return levelsBelow;
}

void
PointCloud::setLevelsBelow()
{
  for(int d=0; d<m_tiles.count(); d++)
    {
      OctreeNode *oNode = m_tiles[d];
      setLevel(oNode, 0);      
    }
}

bool
PointCloud::loadMultipleTiles(QStringList dirnames)
{
  QProgressDialog progress("Loading tiles",
			   "",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int dcount = dirnames.count();
  for (int d=0; d<dcount; d++)
    {
      progress.setValue(100*(float)d/(float)dcount);
      qApp->processEvents();
	  
      QFileInfo finfo(dirnames[d]);
      if (finfo.isDir())
	loadTileOctree(dirnames[d]);
    }

  progress.setValue(100);

  return true;
}

bool
PointCloud::loadTileOctree(QString dirname)
{  
  OctreeNode *oNode = new OctreeNode();
  oNode->setParent(0);

  m_tiles << oNode;

  m_allNodes << oNode;


  QDir jsondir(dirname);

  //-----------------------
  if (jsondir.exists("cloud.js"))
    loadCloudJson(dirname);
  //-----------------------


  //-----------------------
  // check existance of octree.json file
  if (jsondir.exists("octree.json"))
    {
      loadOctreeNodeFromJson(dirname, oNode);
      return true;
    }  
  //-----------------------


  //------------------------------------------------
  // octree.json not found so continue collecting
  // information withing the directory
  //------------------------------------------------

  QStringList namefilters;
  if (m_fileFormat) // LAS/LAZ
    namefilters << "*.las" << "*.laz";
  else
    namefilters << "*.bin";
  
  QProgressDialog progress("Enumerating Files in " + dirname,
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  QDirIterator topDirIter(dirname,
			  namefilters,
			  QDir::Files | QDir::Readable,
			  QDirIterator::Subdirectories);

  QFileInfoList finfolist;
  int nfl = 0;
  while(topDirIter.hasNext())
    {
      topDirIter.next();
      finfolist << topDirIter.fileInfo();
      nfl++;
      if (nfl%100 == 1)
	{
	  float pg = (nfl/100)%100;
	  progress.setValue(pg);
	  qApp->processEvents();
	}
    }


  qint64 totalPoints = 0;
  int maxOct = 0;
  int minNameSize = 10000;
  int maxNameSize = 0;
  for (int i=0; i<finfolist.count(); i++)
    {
      progress.setValue(100*(float)i/(float)finfolist.count());
      qApp->processEvents();

      QString basename = finfolist[i].baseName();
      minNameSize = qMin(minNameSize, basename.count());
      maxNameSize = qMax(maxNameSize, basename.count());

      if (m_fileFormat)
	totalPoints += getNumPointsInLASFile(finfolist[i].absoluteFilePath());
      else
	totalPoints += getNumPointsInBINFile(finfolist[i].absoluteFilePath());
    }

  maxOct = maxNameSize - minNameSize;
  
  //-----------------------------
  qint64 npts;
  Vec shift = m_shift;
  float scale = m_scale;
  if (m_ignoreScaling)
    scale = 1.0;
  float bminZ = m_bminZ;
  float bmaxZ = m_bmaxZ;
  int priority = m_priority;
  int time = m_time;
  bool colorPresent = m_colorPresent;
  bool classPresent = m_classPresent;
  bool xformPresent = m_xformPresent;
  float xform[16];
  if (m_xformPresent)
    memcpy(xform, m_xform, 16*sizeof(float));
  //-----------------------------


  for(int l=0; l<=maxOct; l++)
    {
      progress.setValue(100*(float)l/(float)(maxOct+1));

      QStringList flist;
      for (int i=0; i<finfolist.count(); i++)
	{
	  QString basename = finfolist[i].baseName();
	  if (basename.count() == minNameSize + l)
	    flist << finfolist[i].absoluteFilePath();
	}

      if (l==0)
	{
	  if (m_fileFormat)
	    npts = getNumPointsInLASFile(flist[0]);
	  else
	    npts = getNumPointsInBINFile(flist[0]);	  

	  oNode->setFileName(flist[0]);
	  oNode->setNumPoints(npts);
	  oNode->setLevelsBelow(maxOct);
	  oNode->setLevelString("");
	  oNode->setDataPerVertex(m_dpv);
	  oNode->setOffset(m_octreeMin);
	  oNode->setBMin(m_octreeMin);
	  oNode->setBMax(m_octreeMax);
	  oNode->setTightMin(m_tightOctreeMin);
	  oNode->setTightMax(m_tightOctreeMax);
	  oNode->setPriority(priority);
	  oNode->setTime(time);
	  oNode->setShift(shift);
	  oNode->setScale(scale, m_scaleCloudJs);
	  oNode->setSpacing(m_spacing*scale);
	  oNode->setPointAttributes(m_pointAttrib);
	  oNode->setAttribBytes(m_attribBytes);
	  oNode->setColorPresent(colorPresent);
	  oNode->setClassPresent(classPresent);
	  if (xformPresent)
	    oNode->setXform(xform);
	}
      else
	{
	  for(int fl=0; fl<flist.count(); fl++)
	    {
	      QString flnm = flist[fl];

	      if (m_fileFormat)
		npts = getNumPointsInLASFile(flnm);
	      else
		npts = getNumPointsInBINFile(flnm);	  

	      QString basename = QFileInfo(flnm).baseName();
	      //QMessageBox::information(0, QString("level %1").arg(l), basename);

	      QString levelString = basename.mid(minNameSize, l);

	      // now put the node in octree
	      QList<int> ll;
	      for(int vl=0; vl<l; vl++)
		{
//		  QMessageBox::information(0, "", QString("%1 : %2").\
//					   arg(vl).arg(basename[minNameSize+vl]));
		  ll << basename[minNameSize+vl].digitValue();
		}
	      
	      OctreeNode* tnode = oNode;
	      if (ll.count() > 0)
		{
		  for(int vl=0; vl<ll.count(); vl++)
		    tnode = tnode->childAt(ll[vl]);

		  m_allNodes << tnode;
		}

	      //-----------------	      
	      Vec bmin = m_octreeMin;
	      Vec bmax = m_octreeMax;
	      Vec bsize = m_octreeMax-m_octreeMin;
	      if (ll.count() > 0)
		{	  
		  for(int vl=0; vl<ll.count(); vl++)
		    {
		      //spacing /= 2;
		      bsize /= 2;
		      
		      if (ll[vl]%2 > 0) bmin.z += bsize.z;
		      if (ll[vl]%4 > 1) bmin.y += bsize.y;
		      if (ll[vl]   > 3) bmin.x += bsize.x;
		    }
		  
		  bmax = bmin + bsize;
		}
	      //-----------------
	      
	      tnode->setFileName(flnm);
	      tnode->setNumPoints(npts);
	      tnode->setLevelsBelow(maxOct-l);
	      tnode->setLevelString(levelString);
	      tnode->setDataPerVertex(m_dpv);
	      tnode->setOffset(bmin);
	      tnode->setBMin(bmin);
	      tnode->setBMax(bmax);
	      tnode->setTightMin(m_tightOctreeMin);
	      tnode->setTightMax(m_tightOctreeMax);
	      tnode->setPriority(priority);
	      tnode->setTime(time);
	      tnode->setShift(shift);
	      tnode->setScale(scale, m_scaleCloudJs);
	      tnode->setSpacing(m_spacing*scale);
	      tnode->setPointAttributes(m_pointAttrib);
	      tnode->setAttribBytes(m_attribBytes);
	      tnode->setColorPresent(colorPresent);
	      tnode->setClassPresent(classPresent);
	      if (xformPresent)
		tnode->setXform(xform);
	    }
	}
    }


  saveOctreeNodeToJson(dirname, oNode);

  progress.setValue(100);

  return true;
}

void
PointCloud::loadCloudJson(QString dirname)
{
  QDir jsondir(dirname);
  QString jsonfile = jsondir.absoluteFilePath("cloud.js");

  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonObject jsonCloudData = jsonDoc.object();

  m_spacing = jsonCloudData["spacing"].toDouble();
  m_scaleCloudJs = jsonCloudData["scale"].toDouble();

  {
    QJsonObject jsonInfo = jsonCloudData["boundingBox"].toObject();
    double lx = jsonInfo["lx"].toDouble();
    double ly = jsonInfo["ly"].toDouble();
    double lz = jsonInfo["lz"].toDouble();
    double ux = jsonInfo["ux"].toDouble();
    double uy = jsonInfo["uy"].toDouble();
    double uz = jsonInfo["uz"].toDouble();

    m_octreeMin = Vec(lx,ly,lz);
    m_octreeMax = Vec(ux,uy,uz);
  }

  {
    QJsonObject jsonInfo = jsonCloudData["tightBoundingBox"].toObject();
    double lx = jsonInfo["lx"].toDouble();
    double ly = jsonInfo["ly"].toDouble();
    double lz = jsonInfo["lz"].toDouble();
    double ux = jsonInfo["ux"].toDouble();
    double uy = jsonInfo["uy"].toDouble();
    double uz = jsonInfo["uz"].toDouble();

    m_tightOctreeMin = Vec(qMax(m_octreeMin.x,lx),
			   qMax(m_octreeMin.y,ly),
			   qMax(m_octreeMin.z,lz));
    m_tightOctreeMax = Vec(qMin(m_octreeMax.x,ux),
			   qMin(m_octreeMax.y,uy),
			   qMin(m_octreeMax.z,uz));
  }

  if (jsonCloudData.contains("pointAttributes"))
    {
      if (jsonCloudData["pointAttributes"].isArray())
	{
	  m_fileFormat = 0; // BINARY
	  QJsonArray jsonArray = jsonCloudData["pointAttributes"].toArray();
	  int jc = jsonArray.count();
	  m_pointAttrib.clear();
	  m_attribBytes = 0;
	  for(int ijc=0; ijc<jc; ijc++)
	    {
	      QString name = jsonArray[ijc].toString();
	      m_pointAttrib << name;

	      if(name == "POSITION_CARTESIAN"){
		m_attribBytes += 12;
	      }else if(name == "COLOR_PACKED"){
		m_attribBytes += 4;
	      }else if(name == "INTENSITY"){
		m_attribBytes += 2;
	      }else if(name == "CLASSIFICATION"){
		m_attribBytes += 1;
	      }else if(name == "NORMAL_SPHEREMAPPED"){
		m_attribBytes += 2;
	      }else if(name == "NORMAL_OCT16"){
		m_attribBytes += 2;
	      }else if(name == "NORMAL"){
		m_attribBytes += 12;		
	      }
	    }
	}
      else
	{
	  m_fileFormat = true; // LAS/LAZ
	  m_pointAttrib.clear();
	  m_attribBytes = 0;
	}
    }

}

void
PointCloud::loadOctreeNodeFromJson(QString dirname, OctreeNode *oNode)
{
  QDir jsondir(dirname);
  QString jsonfile = jsondir.absoluteFilePath("octree.json");

  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonArray jsonOctreeData = jsonDoc.array();


  int jstart = 0;

  Vec shift = m_shift;
  float scale = m_scale;
  if (m_ignoreScaling)
    scale = 1.0;
  float bminZ = m_bminZ;
  float bmaxZ = m_bmaxZ;
  int priority = m_priority;
  int time = m_time;
  bool colorPresent = m_colorPresent;
  bool classPresent = m_classPresent;
  bool xformPresent = m_xformPresent;
  float xform[16];
  if (m_xformPresent)
    memcpy(xform, m_xform, 16*sizeof(float));

  
  if ((jsonOctreeData[0].toObject()).contains("mod"))
    {
      QJsonObject jsonOctreeNode = jsonOctreeData[0].toObject();
      QJsonObject jsonInfo = jsonOctreeNode["mod"].toObject();

      if (jsonInfo.contains("priority"))
	priority = jsonInfo["priority"].toDouble();

      if (jsonInfo.contains("time"))
	time = jsonInfo["time"].toDouble();

      if (jsonInfo.contains("shift"))
	{
	  QString str = jsonInfo["shift"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  shift = Vec(xyz[0].toFloat(), 
		      xyz[1].toFloat(), 
		      xyz[2].toFloat());
	}

      if (jsonInfo.contains("scale"))
	scale = jsonInfo["scale"].toDouble();

      if (jsonInfo.contains("bmin.z"))
	bminZ = jsonInfo["bmin.z"].toDouble();

      if (jsonInfo.contains("bmax.z"))
	bmaxZ = jsonInfo["bmax.z"].toDouble();

      if (jsonInfo.contains("color"))
	colorPresent = (jsonInfo["color"].toInt() != 0);

      if (jsonInfo.contains("classification"))
	classPresent = (jsonInfo["classification"].toInt() != 0);

      if (jsonInfo.contains("xform"))
	{
	  QString str = jsonInfo["xform"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  if (xyz.count() == 16)
	    {
	      xformPresent = true;
	      for(int t=0; t<16; t++)
		xform[t] = xyz[t].toFloat();
	    }
	}

      jstart = 1;
    }

  
  for (int i=jstart; i<jsonOctreeData.count(); i++)
    {
      QJsonObject jsonOctreeNode = jsonOctreeData[i].toObject();
      QJsonObject jsonInfo = jsonOctreeNode["node"].toObject();

      QString str;
      QStringList xyz;

      QString flnm = jsonInfo["filename"].toString();
      flnm = jsondir.absoluteFilePath(flnm);
      
//      str = jsonInfo["bmin"].toString();
//      xyz = str.split(" ", QString::SkipEmptyParts);
//      Vec bmin = Vec(xyz[0].toFloat(),
//		     xyz[1].toFloat(),
//		     xyz[2].toFloat());
//
//      str = jsonInfo["bmax"].toString();
//      xyz = str.split(" ", QString::SkipEmptyParts);
//      Vec bmax = Vec(xyz[0].toFloat(),
//		     xyz[1].toFloat(),
//		     xyz[2].toFloat());

      qint64 numpt = jsonInfo["numpoints"].toDouble();

      int levelsBelow = jsonInfo["levelsbelow"].toInt();

      QString lvlStr = jsonInfo["level"].toString();
      
      QList<int> ll;
      for(int vl=0; vl<lvlStr.count(); vl++)
	ll << lvlStr[vl].digitValue();

      //-----------------
      //float spacing = m_spacing;
      Vec bmin = m_octreeMin;
      Vec bmax = m_octreeMax;
      Vec bsize = m_octreeMax-m_octreeMin;
      if (ll.count() > 0)
	{	  
	  for(int vl=0; vl<ll.count(); vl++)
	    {
	      //spacing /= 2;
	      bsize /= 2;

	      if (ll[vl]%2 > 0) bmin.z += bsize.z;
	      if (ll[vl]%4 > 1) bmin.y += bsize.y;
	      if (ll[vl]   > 3) bmin.x += bsize.x;
	    }

	  bmax = bmin + bsize;
	}
      //-----------------


      OctreeNode* tnode = oNode;
      if (ll.count() > 0)
	{
	  for(int vl=0; vl<ll.count(); vl++)
	    tnode = tnode->childAt(ll[vl]);

	  m_allNodes << tnode;
	}
      

//      //---------------------------
//      // modify box height
//      if (bmaxZ > bminZ)
//	{
//	  bmax.z = qMin(bmax.z, (double)bmaxZ/m_scale);
//	  bmin.z = qMax(bmin.z, (double)bminZ/m_scale);
//	  if (bmax.z-bmin.z < 0.1)
//	    {
//	      bmin.z -= 0.05;
//	      bmax.z += 0.05;
//	    }
//	}
//      //---------------------------

      tnode->setFileName(flnm);
      tnode->setOffset(bmin);
      tnode->setBMin(bmin);
      tnode->setBMax(bmax);
      tnode->setTightMin(m_tightOctreeMin);
      tnode->setTightMax(m_tightOctreeMax);
      tnode->setNumPoints(numpt);
      tnode->setLevelsBelow(levelsBelow);
      tnode->setLevelString(lvlStr);
      tnode->setDataPerVertex(m_dpv);

      tnode->setPriority(priority);
      tnode->setTime(time);
      tnode->setShift(shift);
      tnode->setScale(scale, m_scaleCloudJs);
      //tnode->setSpacing(spacing*scale);
      tnode->setSpacing(m_spacing*scale);

      tnode->setPointAttributes(m_pointAttrib);
      tnode->setAttribBytes(m_attribBytes);

      tnode->setColorPresent(colorPresent);
      tnode->setClassPresent(classPresent);
      if (xformPresent)
	tnode->setXform(xform);
    }
  
  m_octreeMin *= m_scale;
  m_octreeMax *= m_scale;
  m_octreeMin += m_shift;
  m_octreeMax += m_shift;

  m_tightOctreeMin *= m_scale;
  m_tightOctreeMax *= m_scale;
  m_tightOctreeMin += m_shift;
  m_tightOctreeMax += m_shift;
}

void
PointCloud::saveOctreeNodeToJson(QString dirname, OctreeNode *oNode)
{
  QDir jsondir(dirname);

  QString str, lvlStr;
  Vec bmin, bmax;

  QJsonArray jsonOctreeData;
  QJsonObject jsonOctreeNode;
  QJsonObject jsonInfo;


  //--------------------------
  // toplevel node
  jsonInfo["filename"] = jsondir.relativeFilePath(oNode->filename());
  
  jsonInfo["numpoints"] = oNode->numpoints();
  jsonInfo["levelsbelow"] = oNode->levelsBelow();

  jsonInfo["level"] = "";

  jsonOctreeNode["node"] = jsonInfo;
  //--------------------------


  jsonOctreeData << jsonOctreeNode;


  QList<OctreeNode*> onl0;
  onl0 << oNode;

  while(onl0.count() > 0)
    {
      QList<OctreeNode*> onl1;
      onl1.clear();
      for(int i=0; i<onl0.count(); i++)
	{
	  OctreeNode *node = onl0[i];
	  if (node->levelsBelow() >= 0)
	    {
	      for (int k=0; k<8; k++)
		{
		  OctreeNode *cnode = node->getChild(k);
		  if (cnode)
		    {
		      onl1 << cnode;

		      QJsonObject jsonOctreeNode;
		      QJsonObject jsonInfo;
		      jsonInfo["filename"] = jsondir.relativeFilePath(cnode->filename());
		      
		      jsonInfo["numpoints"] = cnode->numpoints();
		      jsonInfo["levelsbelow"] = cnode->levelsBelow();
		      
		      lvlStr = cnode->levelString();
		      jsonInfo["level"] = lvlStr;

		      jsonOctreeNode["node"] = jsonInfo;
		      
		      
		      jsonOctreeData << jsonOctreeNode;
		    }
		}
	    }
	}

      onl0 = onl1;
    }

  QJsonDocument saveDoc(jsonOctreeData);
  
  QString jsonfile = jsondir.absoluteFilePath("octree.json");
  //QMessageBox::information(0, "", jsonfile);
  
  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());

  return;
}

qint64
PointCloud::getNumPointsInBINFile(QString flnm)
{
  QFileInfo finfo(flnm);
  qint64 fsz = finfo.size();
  return fsz/m_attribBytes;
}

qint64
PointCloud::getNumPointsInLASFile(QString flnm)
{
  laszip_POINTER laszip_reader;

  laszip_create(&laszip_reader);

  laszip_BOOL is_compressed = flnm.endsWith(".laz");
  if (laszip_open_reader(laszip_reader, flnm.toLatin1().data(), &is_compressed))
    {
      QMessageBox::information(0, "", "Error opening file" + flnm);
    }
  
  laszip_header* header;
  laszip_get_header_pointer(laszip_reader, &header);
  
  laszip_I64 npts = (header->number_of_point_records ? header->number_of_point_records : header->extended_number_of_point_records);

  laszip_close_reader(laszip_reader);

  qint64 numpts = npts;
  return numpts;
}

void
PointCloud::loadModJson(QString jsonfile)
{
  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonObject jsonMod = jsonDoc.object();

  if (jsonMod.contains("mod"))
    {
      QJsonObject jsonInfo = jsonMod["mod"].toObject();
      
      if (jsonInfo.contains("priority"))
	m_priority = jsonInfo["priority"].toDouble();

      if (jsonInfo.contains("time"))
	m_time = jsonInfo["time"].toDouble();

      if (jsonInfo.contains("shift"))
	{
	  QString str = jsonInfo["shift"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  m_shift = Vec(xyz[0].toFloat(),
			xyz[1].toFloat(),
			xyz[2].toFloat());
	}

      if (jsonInfo.contains("scale"))
	m_scale = jsonInfo["scale"].toDouble();

      if (jsonInfo.contains("bmin.z"))
	m_bminZ = jsonInfo["bmin.z"].toDouble();

      if (jsonInfo.contains("bmax.z"))
	m_bmaxZ = jsonInfo["bmax.z"].toDouble();

      if (jsonInfo.contains("color"))
	m_colorPresent = (jsonInfo["color"].toInt() != 0);

      if (jsonInfo.contains("classification"))
	m_classPresent = (jsonInfo["classification"].toInt() != 0);

      if (jsonInfo.contains("xform"))
	{
	  QString str = jsonInfo["xform"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  if (xyz.count() == 16)
	    {
	      m_xformPresent = true;
	      for(int t=0; t<16; t++)
		m_xform[t] = xyz[t].toFloat();
	    }
	}
    }
}

void
PointCloud::drawInactiveTiles()
{
  glColor3f(1,1,1);
  for(int d=0; d<m_tiles.count(); d++)
    {
      OctreeNode *node = m_tiles[d];
      if (!node->isActive())
	{
	  Vec bmin = node->bmin();
	  Vec bmax = node->bmax();
	  StaticFunctions::drawBox(bmin, bmax);
	}
    }
}

void
PointCloud::drawActiveNodes()
{
  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      if (node->isActive())
	{
	  int ol = node->levelsBelow();

	  glColor3f(1,0,0);
	  if (ol == 1) glColor3f(1,1,0);
	  if (ol == 2) glColor3f(0,1,1);
	  if (ol == 3) glColor3f(1,0,1);
	  if (ol == 4) glColor3f(0,1,0);
	  if (ol == 5) glColor3f(0,0,1);
	  if (ol == 6) glColor3f(0,0.5,1);
	  if (ol == 7) glColor3f(1,0.5,0);
	  if (ol == 8) glColor3f(0,1,0.5);

	  Vec bmin = node->bmin();
	  Vec bmax = node->bmax();
	  StaticFunctions::drawBox(bmin, bmax);
	}
    }
}

void
PointCloud::loadLabelsJson(QString jsonfile)
{
  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonArray jsonLabelsData = jsonDoc.array();

  m_labels.clear();

  for(int i=0; i<jsonLabelsData.count(); i++)
    {
      QJsonObject jsonLabelNode = jsonLabelsData[i].toObject();
      QJsonObject jsonInfo = jsonLabelNode["label"].toObject();

      QString caption = jsonInfo["caption"].toString();

      QString str;
      QStringList xyz;

      str = jsonInfo["position"].toString();
      xyz = str.split(" ", QString::SkipEmptyParts);
      Vec position = Vec(xyz[0].toFloat(),
			 xyz[1].toFloat(),
			 xyz[2].toFloat());

      float proximity = jsonInfo["proximity"].toDouble();


      str = jsonInfo["color"].toString();
      xyz = str.split(" ", QString::SkipEmptyParts);
      Vec color = Vec(xyz[0].toFloat(),
		      xyz[1].toFloat(),
		      xyz[2].toFloat());

      float fontSize = jsonInfo["font size"].toDouble();

//      QMessageBox::information(0, "", QString("%1\n%2 %3 %4\n%5").\
//			       arg(caption).\
//			       arg(position.x).arg(position.y).arg(position.z).\
//			       arg(proximity));

      QString linkData;
      if (jsonInfo.contains("link"))
	{
	  linkData = jsonInfo["link"].toString();
	  QFileInfo finfo(jsonfile);
	  QString jpath = finfo.absolutePath();
	  jpath = jpath + QDir::separator() + linkData;

	  linkData = QDir(jpath).absolutePath();
	}

      proximity *= m_scale;

      Label *lbl = new Label();
      lbl->setCaption(caption);
      lbl->setPosition(position);
      lbl->setProximity(proximity);
      lbl->setColor(color);
      lbl->setFontSize(fontSize);
      if (!linkData.isEmpty())
	lbl->setLink(linkData);
      
      m_labels << lbl;
    }

}

void
PointCloud::loadLabelsCSV(QString csvfile)
{
  QFile loadFile(csvfile);
  loadFile.open(QIODevice::ReadOnly);
  
  QTextStream stream(&loadFile);
  QString line;
  line = stream.readLine(); // ignore first line

//  // colour coded treePointCount
//  float tpcMin, tpcMax;
//  tpcMin = 1000000000;
//  tpcMax = -1;

  QList<Vec> tInfo;
  do
    {
      line = stream.readLine(); // ignore first line
      QStringList words = line.split(",", QString::SkipEmptyParts);
      if (words.count() == 12)
	{
	  float x, y, z;
	  float treeHeight, treeArea, treePointCount;
	  float r,g,b;
	  x = words[2].toFloat() * 0.01;
	  y = words[3].toFloat() * 0.01;
	  z = words[7].toFloat() * 0.01;
	  z -= 0.3; // reduce height in order to match the trees

	  treeHeight = words[5].toFloat();
	  treeArea = words[6].toFloat();
	  treePointCount = words[8].toFloat();

	  r = words[9].toFloat()/255.0f;
	  g = words[10].toFloat()/255.0f;
	  b = words[11].toFloat()/255.0f;

	  QList<float> treeInfo;
	  treeInfo << treeHeight;
	  treeInfo << treeArea;
	  treeInfo << treePointCount;

//	  tpcMin = qMin(tpcMin, treePointCount);
//	  tpcMax = qMax(tpcMax, treePointCount);

	  tInfo << Vec(x,y,z);
	  tInfo << Vec(r,g,b);
	  tInfo << Vec(treeHeight, treeArea, treePointCount);

//	  Label *lbl = new Label();
//	  lbl->setPosition(Vec(x,y,z));
//	  lbl->setProximity(10*m_scale);
//	  lbl->setColor(Vec(r,g,b));
//	  lbl->setFontSize(20);
//	  lbl->setTreeInfo(treeInfo);
//	  
//	  m_labels << lbl;
	  
	}    
    } while (!line.isNull());

  for(int i=0; i<tInfo.count()/3; i++)
    {
      Vec xyz = tInfo[3*i+0];
      Vec rgb = tInfo[3*i+1];
      QList<float> treeInfo;
      treeInfo << tInfo[3*i+2].x;
      treeInfo << tInfo[3*i+2].y;
      treeInfo << tInfo[3*i+2].z;

      Label *lbl = new Label();
      lbl->setPosition(xyz);
      lbl->setProximity(10*m_scale);
      lbl->setColor(rgb);
      lbl->setFontSize(20);
      lbl->setTreeInfo(treeInfo);
	  
      m_labels << lbl;
    }
}

void
PointCloud::drawLabels(Camera* cam)
{
  // do not draw labels if this point cloud is not visible
  if (!m_visible)
    return;

  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->drawLabel(cam);
}

void
PointCloud::stickLabelsToGround()
{
  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->stickToGround();
}
bool
PointCloud::findNearestLabelHit(QMatrix4x4 matR,
				QMatrix4x4 finalxformInv,
				float deadRadius,
				QVector3D deadPoint)
{
  m_nearHitLabel = -1;
  m_nearHit = 100000000;

  if (!m_visible)
    return false;
  
//  if (deadRadius <= 0)
//    return false;
  
  if (m_labels.count() == 0)
    return false;

  for(int i=0; i<m_labels.count(); i++)
    {
      float hit = m_labels[i]->checkHit(matR, finalxformInv,
					deadRadius, deadPoint);
      if (hit >= 0)
	{
	  if (m_nearHit > hit)
	    {
	      m_nearHit = hit;
	      m_nearHitLabel = i;
	    }
	}
    }

  if (deadRadius > 0)
    return (m_nearHitLabel >= 0);    
  else
    {
      if (m_nearHit < 1)
	return (m_nearHitLabel >= 0);    
      else
	{
	  return false;
	  m_nearHitLabel = -1;
	}
    }
}
GLuint
PointCloud::labelTexture()
{
  if (m_nearHitLabel < 0)
    return -1;

  return m_labels[m_nearHitLabel]->glTexture();  
}
QSize
PointCloud::labelTextureSize()
{
  if (m_nearHitLabel < 0)
    return QSize(0,0);
		 
  return m_labels[m_nearHitLabel]->textureSize();  
}

void
PointCloud::drawLabels(QVector3D cpos,
		       QVector3D vDir,
		       QVector3D uDir,
		       QVector3D rDir,
		       QMatrix4x4 mat,
		       QMatrix4x4 matR,
		       QMatrix4x4 finalxform,
		       QMatrix4x4 finalxformInv,
		       float deadRadius,
		       QVector3D deadPoint)
{
  // do not draw labels if this point cloud is not visible
  if (!m_visible)
    return;

  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->drawLabel(cpos, vDir, uDir, rDir,
			   mat, matR,
			   finalxform, finalxformInv,
			   deadRadius, deadPoint,
			   (m_nearHitLabel == i));
}


QString
PointCloud::checkLink(Camera *cam, QPoint pos)
{
  for(int i=0; i<m_labels.count(); i++)
    {
      if ( m_labels[i]->checkLink(cam, pos))
	return m_labels[i]->linkData();
    }

  return QString();
}


Vec
PointCloud::tightOctreeMin()
{
  Vec tm = m_tiles[0]->tightOctreeMin();
  for(int d=1; d<m_tiles.count(); d++)
    {
      Vec tom = m_tiles[d]->tightOctreeMin();
      tm.x = qMin(tom.x,tm.x);
      tm.y = qMin(tom.y,tm.y);
      tm.z = qMin(tom.z,tm.z);
    }
  return tm;
}
Vec
PointCloud::tightOctreeMax()
{
  Vec tm = m_tiles[0]->tightOctreeMax();
  for(int d=1; d<m_tiles.count(); d++)
    {
      Vec tom = m_tiles[d]->tightOctreeMax();
      tm.x = qMax(tom.x,tm.x);
      tm.y = qMax(tom.y,tm.y);
      tm.z = qMax(tom.z,tm.z);
    }
  return tm;
}

Vec
PointCloud::octreeMin()
{
  Vec tm = m_tiles[0]->bmin();
  for(int d=1; d<m_tiles.count(); d++)
    {
      Vec tom = m_tiles[d]->bmin();
      tm.x = qMin(tom.x,tm.x);
      tm.y = qMin(tom.y,tm.y);
      tm.z = qMin(tom.z,tm.z);
    }
  return tm;
}
Vec
PointCloud::octreeMax()
{
  Vec tm = m_tiles[0]->bmax();
  for(int d=1; d<m_tiles.count(); d++)
    {
      Vec tom = m_tiles[d]->bmax();
      tm.x = qMax(tom.x,tm.x);
      tm.y = qMax(tom.y,tm.y);
      tm.z = qMax(tom.z,tm.z);
    }
  return tm;
}

QList<uchar>
PointCloud::maxLevelVisible()
{
  QList<uchar> vS;
  for(int d=0; d<m_tiles.count(); d++)
    {
      int maxlevel = 0;

      OctreeNode *node = m_tiles[d];
      if (node->isActive())
	{	  
	  QList<OctreeNode*> vlist;
	  vlist << node;

	  int vi = 0;
	  
	  bool done = false;
	  while (!done)
	    {
	      int oid = vi;
	      OctreeNode *oNode = vlist[vi];
	      vi++;

	      maxlevel = qMax(maxlevel, oNode->level());	  
	      
	      for (int k=0; k<8; k++)
		{
		  OctreeNode *cnode = oNode->getChild(k);
		  if (cnode && cnode->isActive())
		    vlist << cnode;
		}

	      if (vi >= vlist.count())
		done = true;
	    }
	}
      vS << maxlevel;
    }

  return vS;
}

void
PointCloud::updateVisibilityData()
{
  QList<uchar> mvl = maxLevelVisible();

  m_vData.clear();

  for(int d=0; d<m_tiles.count(); d++)
    {
      m_tiles[d]->setMaxVisibleLevel();

      QList<uchar> vS;

      OctreeNode *node = m_tiles[d];
      if (node->isActive())
	{	  
	  QList<OctreeNode*> vlist;
	  vlist << node;
	  
	  int vi = 0;
	  
	  bool done = false;
	  while (!done)
	    {
	      int oid = vi;
	      OctreeNode *oNode = vlist[vi];
	      vi++;
	      
	      uchar vchildren = 0;
	      uchar jump = 0;
	      
	      bool firstChild = true;
	      for (int k=0; k<8; k++)
		{
		  OctreeNode *cnode = oNode->getChild(k);
		  if (cnode && cnode->isActive())
		    {
		      vchildren += qPow(2, k);
		      vlist << cnode;
		      if (firstChild)
			{
			  firstChild = false;
			  jump = vlist.count()-oid-1;
			}
		    }
		}
	      
	      uchar maxVisLevel = oNode->maxVisibleLevel();
	      if (oNode->isLeaf())
		{
		  OctreeNode *parent = oNode->parent();
		  if (parent)
		    {
		      for (int k=0; k<8; k++)
			{
			  OctreeNode *cnode = parent->getChild(k);
			  if (cnode && cnode->isActive())
			    maxVisLevel = qMax(maxVisLevel, cnode->maxVisibleLevel());
			}
		    }
		}
	      
	      vS << vchildren;
	      vS << jump;
	      vS << maxVisLevel;	      
	      vS << (oNode->isLeaf() ? 1 : 0);
	      
	      if (vi >= vlist.count())
		done = true;
	    }
	}

      m_vData << vS;
    }

//  QString mesg;
//  for(int j=0; j<m_vData.count(); j++)
//    {
//      mesg += QString("Tile : %1\n").arg(j);
//      QList<uchar> vS = m_vData[j];
//      if (vS.count() > 0)
//	{
//	  for(int k=0; k<vS.count()/2; k++)
//	    mesg += QString("%1 %2\n").arg(vS[2*k]).arg(vS[2*k+1]);
//	}
//      else
//	mesg += "....\n";
//    }
//
//  QMessageBox::information(0, "", mesg);

}

void
PointCloud::setGlobalMinMax(Vec gmin, Vec gmax)
{
  m_octreeMin -= gmin;
  m_octreeMax -= gmin;

  m_tightOctreeMin -= gmin;
  m_tightOctreeMax -= gmin;

  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->setGlobalMinMax(gmin, gmax);

//  Vec gbox = gmax-gmin;
//  float gboxmax = qMax(gbox.x, qMax(gbox.y, gbox.z));
//  m_octreeMin /= gboxmax;
//  m_octreeMax /= gboxmax;
//  m_tightOctreeMin /= gboxmax;
//  m_tightOctreeMax /= gboxmax;
}
