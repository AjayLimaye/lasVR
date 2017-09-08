#ifndef POINTCLOUD_H
#define POINTCLOUD_H

#include <QProgressDialog>
#include <QFile>
#include <QJsonDocument>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include "octreenode.h"
#include "label.h"

class PointCloud
{
 public :
  PointCloud();
  ~PointCloud();

  void reset();

  void loadPoTreeMultiDir(QString, int timestep=-1, bool ignoreScaling=false);

  bool loadMultipleTiles(QStringList);

  bool loadTileOctree(QString);

  void setLevelsBelow();

  Vec coordMin() { return m_coordMin; };
  Vec coordMax() { return m_coordMax; };

  Vec tightOctreeMin();
  Vec tightOctreeMax();

  Vec octreeMin();
  Vec octreeMax();

  QList<OctreeNode*> tiles() { return m_tiles; }
  QList<OctreeNode*> allNodes() { return m_allNodes; }
  QList< QList<uchar> > vData() { return m_vData; }

  int maxTime();

  int priority() { return m_priority; }
  int time() { return m_time; }

  void drawInactiveTiles();
  void drawActiveNodes();

  void setVisible(bool v) { m_visible = v; };
  bool visible() { return m_visible; }

  void updateVisibilityData();

  QList<uchar> maxLevelVisible();

  void drawLabels(Camera*);
  void drawLabels(QVector3D,
		  QVector3D, QVector3D, QVector3D,
		  QMatrix4x4, QMatrix4x4, QMatrix4x4);

  QString checkLink(Camera*, QPoint);

  void setGlobalMinMax(Vec, Vec);

  void setShowMap(bool sm) { m_showMap = sm; }
  bool showMap() { return m_showMap; }

  void setGravity(bool sm) { m_gravity = sm; }
  bool gravity() { return m_gravity; }

  void setGroundHeight(float gh) { m_groundHeight = gh; }
  float groundHeight() { return m_groundHeight; }

  void setTeleportScale(float gh) { m_teleportScale = gh; }
  float teleportScale() { return m_teleportScale; }

  void setShowSphere(bool s) { m_showSphere = s; }
  bool showSphere() { return m_showSphere; }

  void setPointType(bool s) { m_pointType = s; }
  bool pointType() { return m_pointType; }

  void setLoadAll(bool b) { m_loadAll = b; }  
  void loadAll();

 private :
  QStringList m_filenames;

  int m_dpv;

  bool m_visible;

  float m_spacing;
  Vec m_octreeMin, m_octreeMax;
  Vec m_tightOctreeMin, m_tightOctreeMax;

  float m_scale;
  Vec m_shift;
  float m_bminZ, m_bmaxZ;
  bool m_colorPresent;
  bool m_classPresent;
  bool m_xformPresent;
  float m_xform[16];
  int m_priority;
  int m_time;

  bool m_ignoreScaling;

  Vec m_coordMin;
  Vec m_coordMax;

  bool m_showMap;
  bool m_gravity;
  float m_groundHeight;
  float m_teleportScale;
  bool m_showSphere;
  bool m_pointType;
  bool m_loadAll;

  QList< QList<uchar> > m_vData;

  QList<OctreeNode*> m_tiles;
  QList<OctreeNode*> m_allNodes;

  QList<Label*> m_labels;

  qint64 getNumPointsInFile(QString);

  void setOctreeNodeFromFile(OctreeNode*, QString);

  void loadOctreeNodeFromJson(QString, OctreeNode*);
  void saveOctreeNodeToJson(QString, OctreeNode*);

  int setLevel(OctreeNode*, int);

  void loadModJson(QString);

  void loadCloudJson(QString);

  void loadLabelsJson(QString);
  void loadLabelsCSV(QString);
};

#endif
