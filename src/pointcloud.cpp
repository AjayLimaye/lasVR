#include "global.h"
#include "staticfunctions.h"
#include "pointcloud.h"

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QInputDialog>
#include <QApplication>
#include <QDir>

#include "laszip_dll.h"

PointCloud::PointCloud()
{
  m_filenames.clear(); 

  m_name.clear();
  m_scale = 1.0;
  m_scaleCloudJs = 1.0;
  m_shift = Vec(0,0,0);
  m_rotation = Quaternion();
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_colorPresent = true;
  m_classPresent = false;
  m_priority = 0;
  m_time = -1;
  m_xformCen = Vec(0,0,0);

  m_spacing = 1.0;
  m_octreeMin = Vec(0,0,0);
  m_octreeMax = Vec(0,0,0);
  m_tightOctreeMin = Vec(0,0,0);
  m_tightOctreeMax = Vec(0,0,0);

  m_octreeMinO = Vec(0,0,0);
  m_octreeMaxO = Vec(0,0,0);
  m_tightOctreeMinO = Vec(0,0,0);
  m_tightOctreeMaxO = Vec(0,0,0);
  m_tightOctreeMinAllTiles = Vec(0,0,0);
  m_tightOctreeMaxAllTiles = Vec(0,0,0);

  //m_dpv = 3;
  //m_dpv = 4;
  m_dpv = 6;

  m_fileFormat = true; // LAS
  m_pointAttrib.clear();
  m_attribBytes = 0;

  m_tiles.clear();
  m_labels.clear();
  m_tempLabel = 0;

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

  m_undo.clear();

  m_labelsJsonFile.clear();
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
  m_rotation = Quaternion();
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_colorPresent = true;
  m_classPresent = false;
  m_priority = 0;
  m_time = -1;
  m_xformCen = Vec(0,0,0);

  m_spacing = 1.0;

  m_tiles.clear();
  m_labels.clear();

  if (m_tempLabel)
    delete m_tempLabel;
  m_tempLabel = 0;

  m_vData.clear();

  m_octreeMin = Vec(0,0,0);
  m_octreeMax = Vec(0,0,0);
  m_tightOctreeMin = Vec(0,0,0);
  m_tightOctreeMax = Vec(0,0,0);

  m_octreeMinO = Vec(0,0,0);
  m_octreeMaxO = Vec(0,0,0);
  m_tightOctreeMinO = Vec(0,0,0);
  m_tightOctreeMaxO = Vec(0,0,0);
  m_tightOctreeMinAllTiles = Vec(0,0,0);
  m_tightOctreeMaxAllTiles = Vec(0,0,0);


  m_fileFormat = true; // LAS
  m_pointAttrib.clear();
  m_attribBytes = 0;

  m_undo.clear();
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

void
PointCloud::loadPoTreeMultiDir(QString dirname, int timestep, bool ignoreScaling)
{
  m_scale = 1.0;
  m_scaleCloudJs = 1.0;
  m_shift = Vec(0,0,0);
  m_xformCen = Vec(0,0,0);
  m_rotation = Quaternion();
  m_bminZ = 1;
  m_bmaxZ = 0;
  m_classPresent = false;
  m_priority = 0;
  m_time = timestep;
  m_name = QFileInfo(dirname).fileName();

  m_labelsJsonFile = QDir(dirname).absoluteFilePath("labels.json");

  

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
}

void
PointCloud::setLevel(OctreeNode *node, int lvl)
{
  node->setLevel(lvl);
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
  Global::statusBar()->showMessage("Loading tiles", 2000);
  Global::progressBar()->show();

  int dcount = dirnames.count();
  for (int d=0; d<dcount; d++)
    {	
      Global::progressBar()->setValue(100*(float)d/(float)dcount);
      qApp->processEvents();
      
      //-----------------------
      // drill down till you hit directory containing cloud.js
      QString dnm = dirnames[d];
      if (!QDir(dnm).exists("cloud.js"))
	{
	  QStringList sd;
	  QStringList dlist;
	  dlist << dnm;
	  while(dlist.count() > 0)
	    {
	      for(int di=0; di<dlist.count(); di++)
		{
		  //-----
		  // load mod.json if present in the directory
		  // this will overwrite earlier values
		  if (QDir(dlist[di]).exists("mod.json"))
		    {
		      QString jsonfile = QDir(dlist[di]).absoluteFilePath("mod.json");
		      loadModJson(jsonfile);
		    }
		  //-----

		  //-----
		  // now parse the subdirectories
		  QStringList subdir = QDir(dlist[di]).entryList(QDir::AllDirs |
								 QDir::NoDotAndDotDot);
		  QStringList dnames;
		  for(int si=0; si<subdir.count(); si++)
		    {
		      QString dname = QDir(dnm).absoluteFilePath(subdir[si]);
		      if (QDir(dname).exists("cloud.js"))
			dnames << dname;
		      else
			sd << dname;
		    }

		  if (dnames.count() > 0) // load these PoTrees
		    loadLowerTiles(dnames);

		}
	      if (sd.count() > 0)
		dlist = sd;
	      else
		dlist.clear();		  
	    }
	}
      else
	{
	  QStringList dnames;
	  dnames << dnm;
	  loadLowerTiles(dnames);
	}
    }

  Global::progressBar()->hide();
  Global::statusBar()->showMessage("Start", 100);

  return true;
}

void
PointCloud::loadLowerTiles(QStringList dirnames)
{
  int dcount = dirnames.count();
  for (int d=0; d<dcount; d++)
    {	
      Global::progressBar()->setValue(100*(float)d/(float)dcount);
      qApp->processEvents();
      
      QFileInfo finfo(dirnames[d]);
      if (finfo.isDir())
	{
	  loadTileOctree(dirnames[d]);

	  if (d == 0)
	    {
	      m_tightOctreeMinAllTiles = m_tightOctreeMinO;
	      m_tightOctreeMaxAllTiles = m_tightOctreeMaxO;
	    }
	  else
	    {
	      double lx = m_tightOctreeMinAllTiles.x;
	      double ly = m_tightOctreeMinAllTiles.y;
	      double lz = m_tightOctreeMinAllTiles.z;
	      double ux = m_tightOctreeMaxAllTiles.x;
	      double uy = m_tightOctreeMaxAllTiles.y;
	      double uz = m_tightOctreeMaxAllTiles.z;
	      m_tightOctreeMinAllTiles = Vec(qMin(m_tightOctreeMinO.x,lx),
					     qMin(m_tightOctreeMinO.y,ly),
					     qMin(m_tightOctreeMinO.z,lz));
	      m_tightOctreeMaxAllTiles = Vec(qMax(m_tightOctreeMaxO.x,ux),
					     qMax(m_tightOctreeMaxO.y,uy),
					     qMax(m_tightOctreeMaxO.z,uz));
	    }
	}
    }
}

bool
PointCloud::loadTileOctree(QString dirnameO)
{  
  OctreeNode *oNode = new OctreeNode();
  oNode->setParent(0);

  m_tiles << oNode;

  m_allNodes << oNode;


  //-----------------------
  QString dirname = dirnameO;
  if (!QDir(dirname).exists("cloud.js"))
    {
      // drill down till you hit directory containing cloud.js
      QDirIterator topDirIter(dirname,
			      QDir::Dirs | QDir::Readable |
			      QDir::NoDotAndDotDot | QDir::NoSymLinks,
			      QDirIterator::Subdirectories);
      
      while(topDirIter.hasNext())
	{
	  QString dnm = topDirIter.next();	  
	  if (QDir(dnm).exists("cloud.js"))
	    {
	      dirname = dnm;
	      break;
	    }	  
	}
    }
  //-----------------------


  QDir jsondir(dirname);

  //-----------------------
  if (jsondir.exists("cloud.js"))
    loadCloudJson(dirname);
  else
    return false;
  //-----------------------


  //-----------------------
  // load mod.json if present in the directory
  // this will overwrite earlier values
  if (QDir(dirname).exists("mod.json"))
    {
      QString jsonfile = QDir(dirname).absoluteFilePath("mod.json");
      loadModJson(jsonfile);
    }
  //-----------------------


  //-----------------------
  // just make sure that height limits are in right order
  if (m_bminZ > m_bmaxZ)
    {
      m_bminZ = m_tightOctreeMinO.z;
      m_bmaxZ = m_tightOctreeMaxO.z;
    }
  //-----------------------

  
  //-----------------------
  // check existance of octree.json file
  if (jsondir.exists("octree.json"))
    {
      loadOctreeNodeFromJson(dirname, oNode);
      return true;
    }  
  //-----------------------


  QStringList namefilters;
  if (m_fileFormat) // LAS/LAZ
    namefilters << "*.las" << "*.laz";
  else
    namefilters << "*.bin";
  
  Global::statusBar()->showMessage("Enumerating Files in "+dirname);

  Global::progressBar()->show();


  QDirIterator topDirIter(dirname,
			  namefilters,
			  QDir::Files | QDir::Readable,
			  QDirIterator::Subdirectories);

  int nfl = 0;
  QJsonArray jsonOctreeData;
  while(topDirIter.hasNext())
    {
      topDirIter.next();
      QFileInfo finfo = topDirIter.fileInfo();
      QString basename = finfo.baseName();
      QString levelString = basename.mid(1);
      
      //--------------------------
      QJsonObject jsonOctreeNode;
      QJsonObject jsonInfo;
      jsonInfo["filename"] = jsondir.relativeFilePath(finfo.absoluteFilePath());  
      jsonInfo["level"] = levelString;
      jsonOctreeNode["node"] = jsonInfo;
      //--------------------------
      
      jsonOctreeData << jsonOctreeNode;
      nfl++;
      if (nfl%100 == 1)
	{
	  float pg = nfl%100;
	  Global::progressBar()->setValue(pg);
	  qApp->processEvents();
	}
    }
  
  loadOctreeNodeFromJsonArray(jsondir, oNode, jsonOctreeData);
  
  //------------
  QJsonDocument saveDoc(jsonOctreeData);  
  QString jsonfile = jsondir.absoluteFilePath("octree.json");  
  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());
  //------------

  return true;
}

void
PointCloud::loadCloudJson(QString dirname)
{
  m_filenames << dirname;

  QDir jsondir(dirname);
  QString jsonfile = jsondir.absoluteFilePath("cloud.js");

  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonObject jsonCloudData = jsonDoc.object();

  m_npoints = jsonCloudData["points"].toInt();
  m_hierarchyStepSize = jsonCloudData["hierarchyStepSize"].toInt();

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
    m_octreeMinO = Vec(lx,ly,lz);
    m_octreeMaxO = Vec(ux,uy,uz);
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

    m_tightOctreeMinO = m_tightOctreeMin;
    m_tightOctreeMaxO = m_tightOctreeMax;
    
    m_xformCen = (m_tightOctreeMinO+m_tightOctreeMaxO)*0.5;
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

  if (m_bminZ > m_bmaxZ)
    {
      m_bminZ = m_tightOctreeMinO.z;
      m_bmaxZ = m_tightOctreeMaxO.z;
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
  Quaternion rotate = m_rotation;
  float bminZ = m_bminZ;
  float bmaxZ = m_bmaxZ;
  int priority = m_priority;
  int time = m_time;
  bool colorPresent = m_colorPresent;
  bool classPresent = m_classPresent;

  
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

      if (jsonInfo.contains("rotation"))
	{
	  QString str = jsonInfo["rotation"].toString();
	  QStringList xyzw = str.split(" ", QString::SkipEmptyParts);
	  rotate = Quaternion(xyzw[0].toFloat(), 
			      xyzw[1].toFloat(), 
			      xyzw[2].toFloat(),
			      xyzw[4].toFloat());
	}

      if (jsonInfo.contains("scale"))
	scale = jsonInfo["scale"].toDouble();

      if (jsonInfo.contains("min_height"))
	bminZ = jsonInfo["min_height"].toDouble();

      if (jsonInfo.contains("max_height"))
	bmaxZ = jsonInfo["max_height"].toDouble();

      if (jsonInfo.contains("color"))
	{
	  if (jsonInfo["color"].isBool())
	    colorPresent = jsonInfo["color"].toBool();
	  else
	    colorPresent = (jsonInfo["color"].toInt() != 0);
	}

      if (jsonInfo.contains("classification"))
	{
	  if (jsonInfo["classification"].isBool())
	    classPresent = jsonInfo["classification"].toBool();
	  else
	    classPresent = (jsonInfo["classification"].toInt() != 0);
	}

      jstart = 1;
    }

  Global::statusBar()->showMessage(dirname, 2000);
  Global::progressBar()->show();

  int jend = jsonOctreeData.count();
  for (int i=jstart; i<jend; i++)
    {
      Global::progressBar()->setValue(100*(float)(i-jstart)/(float)(jend-jstart+1));
      qApp->processEvents();


      QJsonObject jsonOctreeNode = jsonOctreeData[i].toObject();
      QJsonObject jsonInfo = jsonOctreeNode["node"].toObject();

      QString str;
      QStringList xyz;

      QString flnm = jsonInfo["filename"].toString();
      flnm = jsondir.absoluteFilePath(flnm);
      
      //qint64 numpt = jsonInfo["numpoints"].toDouble();

      //int levelsBelow = jsonInfo["levelsbelow"].toInt();

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

      tnode->setFileName(flnm);
      tnode->setOffset(bmin);
      tnode->setBMin(bmin);
      tnode->setBMax(bmax);
      tnode->setTightMin(m_tightOctreeMinO);
      tnode->setTightMax(m_tightOctreeMaxO);
      //tnode->setNumPoints(numpt);
      tnode->setLevelString(lvlStr);
      tnode->setDataPerVertex(m_dpv);

      tnode->setPriority(priority);
      tnode->setScale(scale, m_scaleCloudJs);      
      tnode->setSpacing(m_spacing*scale);

      tnode->setPointAttributes(m_pointAttrib);
      tnode->setAttribBytes(m_attribBytes);

      tnode->setColorPresent(colorPresent);
      tnode->setClassPresent(classPresent);

      tnode->setZBounds(m_bminZ, m_bmaxZ);
    }

  setXform(m_scale, m_shift, m_rotation, m_xformCen);
}

void
PointCloud::loadOctreeNodeFromJsonArray(QDir jsondir,
					OctreeNode *oNode,
					QJsonArray jsonOctreeData)
{
  int jstart = 0;

  Vec shift = m_shift;
  float scale = m_scale;
  if (m_ignoreScaling)
    scale = 1.0;
  Quaternion rotate = m_rotation;
  float bminZ = m_bminZ;
  float bmaxZ = m_bmaxZ;
  int priority = m_priority;
  int time = m_time;
  bool colorPresent = m_colorPresent;
  bool classPresent = m_classPresent;

  
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

      if (jsonInfo.contains("rotation"))
	{
	  QString str = jsonInfo["rotation"].toString();
	  QStringList xyzw = str.split(" ", QString::SkipEmptyParts);
	  rotate = Quaternion(xyzw[0].toFloat(), 
			      xyzw[1].toFloat(), 
			      xyzw[2].toFloat(),
			      xyzw[4].toFloat());
	}

      if (jsonInfo.contains("scale"))
	scale = jsonInfo["scale"].toDouble();

      if (jsonInfo.contains("min_height"))
	bminZ = jsonInfo["min_height"].toDouble();

      if (jsonInfo.contains("max_height"))
	bmaxZ = jsonInfo["max_height"].toDouble();

      if (jsonInfo.contains("color"))
	{
	  if (jsonInfo["color"].isBool())
	    colorPresent = jsonInfo["color"].toBool();
	  else
	    colorPresent = (jsonInfo["color"].toInt() != 0);
	}

      if (jsonInfo.contains("classification"))
	{
	  if (jsonInfo["classification"].isBool())
	    classPresent = jsonInfo["classification"].toBool();
	  else
	    classPresent = (jsonInfo["classification"].toInt() != 0);
	}

      jstart = 1;
    }

  Global::statusBar()->showMessage(jsondir.dirName(), 2000);
  Global::progressBar()->show();

  int jend = jsonOctreeData.count();
  for (int i=jstart; i<jend; i++)
    {
      Global::progressBar()->setValue(100*(float)(i-jstart)/(float)(jend-jstart+1));
      qApp->processEvents();


      QJsonObject jsonOctreeNode = jsonOctreeData[i].toObject();
      QJsonObject jsonInfo = jsonOctreeNode["node"].toObject();

      QString str;
      QStringList xyz;

      QString flnm = jsonInfo["filename"].toString();
      flnm = jsondir.absoluteFilePath(flnm);

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

      tnode->setFileName(flnm);
      tnode->setOffset(bmin);
      tnode->setBMin(bmin);
      tnode->setBMax(bmax);
      tnode->setTightMin(m_tightOctreeMinO);
      tnode->setTightMax(m_tightOctreeMaxO);
      tnode->setLevelString(lvlStr);
      tnode->setDataPerVertex(m_dpv);

      tnode->setPriority(priority);
      tnode->setScale(scale, m_scaleCloudJs);      
      tnode->setSpacing(m_spacing*scale);

      tnode->setPointAttributes(m_pointAttrib);
      tnode->setAttribBytes(m_attribBytes);

      tnode->setColorPresent(colorPresent);
      tnode->setClassPresent(classPresent);

      tnode->setZBounds(m_bminZ, m_bmaxZ);
    }

  setXform(m_scale, m_shift, m_rotation, m_xformCen);
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

      if (jsonInfo.contains("name"))
	m_name = jsonInfo["name"].toString();

      if (jsonInfo.contains("shift"))
	{
	  QString str = jsonInfo["shift"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  m_shift = Vec(xyz[0].toFloat(),
			xyz[1].toFloat(),
			xyz[2].toFloat());
	}

      if (jsonInfo.contains("xformcen"))
	{
	  QString str = jsonInfo["xformcen"].toString();
	  QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	  m_xformCen = Vec(xyz[0].toFloat(),
			   xyz[1].toFloat(),
			   xyz[2].toFloat());
	}

      if (jsonInfo.contains("rotation"))
	{
	  QString str = jsonInfo["rotation"].toString();
	  QStringList xyzw = str.split(" ", QString::SkipEmptyParts);
	  m_rotation = Quaternion(xyzw[0].toFloat(), 
				  xyzw[1].toFloat(), 
				  xyzw[2].toFloat(),
				  xyzw[3].toFloat());
	  m_rotation.normalize();
	}

      if (jsonInfo.contains("scale"))
	m_scale = jsonInfo["scale"].toDouble();

      if (jsonInfo.contains("min_height"))
	m_bminZ = jsonInfo["min_height"].toDouble();

      if (jsonInfo.contains("max_height"))
	m_bmaxZ = jsonInfo["max_height"].toDouble();

      if (jsonInfo.contains("color"))
	{
	  if (jsonInfo["color"].isBool())
	    m_colorPresent = jsonInfo["color"].toBool();
	  else
	    m_colorPresent = (jsonInfo["color"].toInt() != 0);
	}

      if (jsonInfo.contains("classification"))
	m_classPresent = (jsonInfo["classification"].toInt() != 0);
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
	  //int ol = node->levelsBelow();

	  glColor3f(1,0,0);
//	  if (ol == 1) glColor3f(1,1,0);
//	  if (ol == 2) glColor3f(0,1,1);
//	  if (ol == 3) glColor3f(1,0,1);
//	  if (ol == 4) glColor3f(0,1,0);
//	  if (ol == 5) glColor3f(0,0,1);
//	  if (ol == 6) glColor3f(0,0.5,1);
//	  if (ol == 7) glColor3f(1,0.5,0);
//	  if (ol == 8) glColor3f(0,1,0.5);

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

  for(int i=0; i<m_labels.count(); i++)
    delete m_labels[i];
  m_labels.clear();

  if (m_tempLabel)
    delete m_tempLabel;
  m_tempLabel = 0;

  
  QString icondir = qApp->applicationDirPath() +       \
                        QDir::separator() + "assets" + \
                        QDir::separator() + "annotation_icons";
  QDir idir(icondir);

  for(int i=0; i<jsonLabelsData.count(); i++)
    {
      QJsonObject jsonLabelNode = jsonLabelsData[i].toObject();
      QJsonObject jsonInfo = jsonLabelNode["label"].toObject();

      QString caption;
      QString icon;
      
      if (jsonInfo.contains("caption"))
	caption = jsonInfo["caption"].toString();
      
      if (jsonInfo.contains("icon"))
	icon = jsonInfo["icon"].toString();

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
      if (icon.isEmpty())
	lbl->setCaption(caption);
      else
	lbl->setIcon(icon);
      lbl->setPosition(position);
      lbl->setProximity(proximity);
      lbl->setColor(color);
      lbl->setFontSize(fontSize);
      if (!linkData.isEmpty())
	lbl->setLink(linkData);
      
      lbl->setXform(m_scale, m_scaleCloudJs,
		    m_shift, m_octreeMin,
		    m_rotation, m_xformCen);
      lbl->setGlobalMinMax(m_gmin, m_gmax);
      lbl->genVertData();

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
	  x = words[2].toFloat();
	  y = words[3].toFloat();
	  z = words[7].toFloat();
//	  x = words[2].toFloat() * 0.01;
//	  y = words[3].toFloat() * 0.01;
//	  z = words[7].toFloat() * 0.01;
//	  z -= 0.3; // reduce height in order to match the trees

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

	  tInfo << Vec(x,y,z);
	  tInfo << Vec(r,g,b);
	  tInfo << Vec(treeHeight, treeArea, treePointCount);
	  
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
      lbl->setProximity(100*m_scale);
      lbl->setColor(rgb);
      lbl->setFontSize(20);
      lbl->setTreeInfo(treeInfo);
	  
      m_labels << lbl;
    }
}

void
PointCloud::setTempLabel(Vec v, QString icon)
{
  Vec pos;
  pos = xformPointInverse(v);

  QString caption = "Annotation";
  
  Label *lbl = new Label();
  if (icon.isEmpty())
    lbl->setCaption(caption);
  else
    lbl->setIcon(icon);
  lbl->setPosition(pos);
  lbl->setProximity(200*m_scale);
  lbl->setColor(Vec(0.7, 0.85, 1.0));
  lbl->setFontSize(20);
  
  lbl->setXform(m_scale, m_scaleCloudJs,
		m_shift, m_octreeMin,
		m_rotation, m_xformCen);
  lbl->setGlobalMinMax(m_gmin, m_gmax);
  lbl->genVertData();

  if (m_tempLabel)
    delete m_tempLabel;  
  m_tempLabel = lbl;
}
void
PointCloud::moveTempLabel(Vec v)
{
  Vec pos;
  pos = xformPointInverse(v);
  m_tempLabel->setPosition(pos);
  m_tempLabel->setGlobalMinMax(m_gmin, m_gmax);
}
void
PointCloud::addLabel(Vec v, QString icon)
{
  if (m_tempLabel)
    delete m_tempLabel;
  m_tempLabel = 0;
  
  Vec pos;
  pos = xformPointInverse(v);

  QString caption = "Annotation";
  
  
  Label *lbl = new Label();
  if (icon.isEmpty())
    lbl->setCaption(caption);
  else
    lbl->setIcon(icon);
  lbl->setPosition(pos);
  lbl->setProximity(200*m_scale);
  lbl->setColor(Vec(0.7, 0.85, 1.0));
  lbl->setFontSize(20);
  
  lbl->setXform(m_scale, m_scaleCloudJs,
		m_shift, m_octreeMin,
		m_rotation, m_xformCen);
  lbl->setGlobalMinMax(m_gmin, m_gmax);
  lbl->genVertData();
  m_labels << lbl;


  saveLabelToJson(pos, caption, icon);
}
void
PointCloud::saveLabelToJson(Vec pos, QString caption, QString icon)
{
  QJsonArray jsonLabelsData;  

  if (QFile::exists(m_labelsJsonFile))
    {
      QFile loadFile(m_labelsJsonFile);
      loadFile.open(QIODevice::ReadOnly);
      QByteArray data = loadFile.readAll();
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      jsonLabelsData = jsonDoc.array();
      loadFile.close();
    }

  QJsonObject jsonInfo;
  if (icon.isEmpty())
    jsonInfo["caption"] = caption;
  else
    jsonInfo["icon"] = icon;
  jsonInfo["position"] = QString("%1 %2 %3").arg(pos.x).arg(pos.y).arg(pos.z);
  jsonInfo["proximity"] = 200;
  jsonInfo["color"] = "0.7 0.85 1.0";
  jsonInfo["font size"] = 20;
  
  QJsonObject jsonLabelNode;
  jsonLabelNode["label"] = jsonInfo;

  jsonLabelsData << jsonLabelNode;

  
  QJsonDocument saveDoc(jsonLabelsData);  
  QFile saveFile(m_labelsJsonFile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());
  saveFile.flush();
  saveFile.close();

}

void
PointCloud::drawLabels(Camera* cam)
{
  // do not draw labels if this point cloud is not visible
  if (!m_visible)
    return;
  
  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->drawLabel(cam);

  if (m_tempLabel)
    m_tempLabel->drawLabel(cam);
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
      return (m_nearHitLabel >= 0);    

//      if (m_nearHit < 1)
//	return (m_nearHitLabel >= 0);    
//      else
//	{
//	  return false;
//	  //m_nearHitLabel = -1;
//	}
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
  if (!m_visible || (m_labels.count()==0 && !m_tempLabel))
    return;


  if (m_tempLabel)
    m_tempLabel->drawLabel(cpos, vDir, uDir, rDir,
			   mat, matR,
			   finalxform, finalxformInv,
			   deadRadius, deadPoint,
			   false);

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
  return m_tightOctreeMin;
}
Vec
PointCloud::tightOctreeMax()
{
  return m_tightOctreeMax;
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

void
PointCloud::updateVisibilityData()
{
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

}

Vec
PointCloud::xformPointInverse(Vec v)
{
  Vec ov = v + m_gmin;
  ov -= m_shift;
  ov -= m_xformCen;
  ov /= m_scale;
  ov = m_rotation.inverseRotate(ov);
  ov += m_xformCen;

  return ov;
}

Vec
PointCloud::xformPoint(Vec v)
{
  Vec ov = v-m_xformCen;

  ov = m_rotation.rotate(ov);

  ov *= m_scale;

  ov += m_xformCen;
  
  ov += m_shift;

  ov -= m_gmin;

  return ov;
}

void
PointCloud::setZBounds(float bminz, float bmaxz)
{
  m_bminZ = bminz;
  m_bmaxZ = bmaxz;

  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->setZBounds(m_bminZ, m_bmaxZ);
    }
}

void
PointCloud::setGlobalMinMax(Vec gmin, Vec gmax)
{
  m_gmin = gmin;
  m_gmax = gmax;

  m_octreeMin = xformPoint(m_octreeMinO);
  m_octreeMax = xformPoint(m_octreeMaxO);

  m_tightOctreeMin = xformPoint(m_tightOctreeMinAllTiles);
  m_tightOctreeMax = xformPoint(m_tightOctreeMaxAllTiles);

  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->setXform(m_scale, m_scaleCloudJs,
			  m_shift, m_octreeMin,
			  m_rotation, m_xformCen);

  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->setGlobalMinMax(m_gmin, m_gmax);
}

void
PointCloud::translate(Vec trans)
{
  setShift(m_shift+trans);
}

void
PointCloud::setScale(float scl)
{
  m_scale = scl;

  m_octreeMin = xformPoint(m_octreeMinO);
  m_octreeMax = xformPoint(m_octreeMaxO);
  m_tightOctreeMin = xformPoint(m_tightOctreeMinAllTiles);
  m_tightOctreeMax = xformPoint(m_tightOctreeMaxAllTiles);
  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->unloadData();
      node->setScale(m_scale, m_scaleCloudJs);
    } 
}

void
PointCloud::setRotation(Quaternion rotate)
{
  m_rotation = rotate;
  setShift(m_shift);
}
void
PointCloud::setXformCen(Vec xformcen)
{
  m_xformCen = xformcen;
  setShift(m_shift);
}

void
PointCloud::setShift(Vec shift)
{
  m_shift = shift;

  m_octreeMin = xformPoint(m_octreeMinO);
  m_octreeMax = xformPoint(m_octreeMaxO);
  m_tightOctreeMin = xformPoint(m_tightOctreeMinAllTiles);
  m_tightOctreeMax = xformPoint(m_tightOctreeMaxAllTiles);

  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->unloadData();
      node->setShift(m_shift);
    } 
}

void
PointCloud::setXform(float scale, Vec shift, Quaternion rotate, Vec xformCen)
{
  m_undo << QVector4D(m_shift.x, m_shift.y, m_shift.z, m_scale);
  m_undo << QVector4D(m_rotation[0], m_rotation[1], m_rotation[2], m_rotation[3]);
  m_undo << QVector4D(m_xformCen.x, m_xformCen.y, m_xformCen.z, 1);

  m_scale = scale;
  m_shift = shift;
  m_rotation = rotate.normalized();
  m_xformCen = xformCen;

  m_octreeMin = xformPoint(m_octreeMinO);
  m_octreeMax = xformPoint(m_octreeMaxO);
  m_tightOctreeMin = xformPoint(m_tightOctreeMinAllTiles);
  m_tightOctreeMax = xformPoint(m_tightOctreeMaxAllTiles);
  
  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->setXform(m_scale, m_shift, m_rotation, m_xformCen);
      node->reloadData();
    } 
}

void
PointCloud::undo()
{
  if (m_undo.count() == 0)
    return;

  QVector4D xc = m_undo.takeLast();
  QVector4D rot = m_undo.takeLast();
  QVector4D u = m_undo.takeLast();

  //-------
  // always keep the first one
  if (m_undo.count() == 0)
    {
      m_undo << u;
      m_undo << rot;
      m_undo << xc;
    }
  //-------

  m_rotation = Quaternion(rot[0],rot[1],rot[2],rot[3]);
  m_shift = Vec(u.x(), u.y(), u.z());
  m_scale = u.w();
  m_xformCen = Vec(xc.x(), xc.y(), xc.z());

  m_octreeMin = xformPoint(m_octreeMinO);
  m_octreeMax = xformPoint(m_octreeMaxO);
  m_tightOctreeMin = xformPoint(m_tightOctreeMinAllTiles);
  m_tightOctreeMax = xformPoint(m_tightOctreeMaxAllTiles);
  
  for(int d=0; d<m_allNodes.count(); d++)
    {
      OctreeNode *node = m_allNodes[d];
      node->setXform(m_scale, m_shift, m_rotation, m_xformCen);
      node->reloadData();
    } 
  
}

void
PointCloud::saveModInfo(QString askString, bool askTime)
{
  QJsonObject jsonMod;

  QJsonObject jsonInfo;

//  if (askTime || m_time == -1)
//    {
//      bool ok;
//      int tm = QInputDialog::getInt(0,
//				    "Pointcloud time step",
//				    askString,
//				    m_time, -1, 10000, 1, &ok);
//      if (ok)
//	m_time = tm;
//    }

  
  jsonInfo["name"] = m_name;

  jsonInfo["time"] = m_time;

  jsonInfo["color"] = m_colorPresent;

  QString str = QString("%1  %2  %3").\
    arg(m_shift.x, 12, 'f', 2).\
    arg(m_shift.y, 12, 'f', 2).\
    arg(m_shift.z, 12, 'f', 2); 
  jsonInfo["shift"] = str.simplified();

  str = QString("%1  %2  %3").\
    arg(m_xformCen.x, 12, 'f', 2).\
    arg(m_xformCen.y, 12, 'f', 2).\
    arg(m_xformCen.z, 12, 'f', 2); 
  jsonInfo["xformcen"] = str.simplified();

  jsonInfo["scale"] = m_scale;

  str = QString("%1  %2  %3  %4"). \
    arg(m_rotation[0], 12, 'f', 8).\
    arg(m_rotation[1], 12, 'f', 8).\
    arg(m_rotation[2], 12, 'f', 8).\
    arg(m_rotation[3], 12, 'f', 8); 
  jsonInfo["rotation"] = str.simplified();

  jsonInfo["min_height"] = m_bminZ;
  jsonInfo["max_height"] = m_bmaxZ;
  
  jsonMod["mod"] = jsonInfo;

  QJsonDocument saveDoc(jsonMod);

  QString jsonfile = QDir(m_filenames[0]).absoluteFilePath("mod.json");

  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());

  if (askTime) // modified point cloud
    QMessageBox::information(0, "", "Information saved to\n"+jsonfile);
}

void
PointCloud::setEditMode(bool b)
{
  for(int d=0; d<m_allNodes.count(); d++)
    {
      m_allNodes[d]->setEditMode(b);
      m_allNodes[d]->reloadData();
    }
}

void
PointCloud::reload()
{
  for(int d=0; d<m_allNodes.count(); d++)
    {
      m_allNodes[d]->setColorPresent(m_colorPresent);
      m_allNodes[d]->setZBounds(m_bminZ, m_bmaxZ);
      m_allNodes[d]->reloadData();
    }
}

void
PointCloud::regenerateLabels()
{
  for(int i=0; i<m_labels.count(); i++)
    m_labels[i]->regenerate();
}
