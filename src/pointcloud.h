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

  Vec globalMin() { return m_gmin; };
  Vec globalMax() { return m_gmax; };

  Vec tightOctreeMinO() { return m_tightOctreeMinAllTiles; }
  Vec tightOctreeMaxO() { return m_tightOctreeMaxAllTiles; }

  Vec tightOctreeMin();
  Vec tightOctreeMax();

  Vec octreeMin();
  Vec octreeMax();

  QList<OctreeNode*> tiles() { return m_tiles; }
  QList<OctreeNode*> allNodes() { return m_allNodes; }
  QList< QList<uchar> > vData() { return m_vData; }

  //int maxTime();

  QString name() { return m_name; }
  void setName(QString name) { m_name = name; }

  int priority() { return m_priority; }
  int time() { return m_time; }
  void setTime(int t) { m_time = t; }

  void drawInactiveTiles();
  void drawActiveNodes();

  void setVisible(bool v) { m_visible = v; };
  bool visible() { return m_visible; }

  void updateVisibilityData();

  QList<uchar> maxLevelVisible();

  void loadLabelsJson(QString);
  void addLabel(Vec);
  void drawLabels(Camera*);
  void drawLabels(QVector3D,
		  QVector3D, QVector3D, QVector3D,
		  QMatrix4x4, QMatrix4x4,
		  QMatrix4x4, QMatrix4x4,
		  float, QVector3D);
  void stickLabelsToGround();
  bool findNearestLabelHit(QMatrix4x4, QMatrix4x4,
			   float, QVector3D);
  GLuint labelTexture();
  QSize labelTextureSize();



  QString checkLink(Camera*, QPoint);

  void setGlobalMinMax(Vec, Vec);

  void setShowMap(bool sm) { m_showMap = sm; }
  bool showMap() { return m_showMap; }

  void setGravity(bool sm) { m_gravity = sm; }
  bool gravity() { return m_gravity; }

  void setSkybox(bool sm) { m_skybox = sm; }
  bool skybox() { return m_skybox; }

  void setPlayButton(bool sm) { m_playButton = sm; }
  bool playButton() { return m_playButton; }

  void setGroundHeight(float gh) { m_groundHeight = gh; }
  float groundHeight() { return m_groundHeight; }

  void setTeleportScale(float gh) { m_teleportScale = gh; }
  float teleportScale() { return m_teleportScale; }

  void setShowSphere(bool s) { m_showSphere = s; }
  bool showSphere() { return m_showSphere; }

  void setPointType(bool s) { m_pointType = s; }
  bool pointType() { return m_pointType; }

  void setColorPresent(bool b) { m_colorPresent = b; }
  bool colorPresent() { return m_colorPresent; }

  void zBounds(float& bminz, float& bmaxz) { bminz = m_bminZ; bmaxz = m_bmaxZ; }
  void setZBounds(float, float);
  
  void setLoadAll(bool b) { m_loadAll = b; }  
  void loadAll();

  void reload();
  
  void translate(Vec);
  void setScale(float);
  float getScale() { return m_scale; }

  void setShift(Vec);
  Vec getShift() { return m_shift; }

  void setRotation(Quaternion);
  Quaternion getRotation() { return m_rotation; }

  void setXformCen(Vec);
  Vec getXformCen() { return m_xformCen; }

  void setXform(float, Vec, Quaternion, Vec);
  Vec xformPoint(Vec);

  Vec xformPointInverse(Vec);

  void undo();

  void saveModInfo(QString, bool);

  void setEditMode(bool);

 private :
  QStringList m_filenames;

  int m_dpv;

  bool m_visible;

  float m_spacing;
  Vec m_octreeMin, m_octreeMax;
  Vec m_tightOctreeMin, m_tightOctreeMax;
  Vec m_octreeMinO, m_octreeMaxO;
  Vec m_tightOctreeMinO, m_tightOctreeMaxO;
  Vec m_tightOctreeMinAllTiles, m_tightOctreeMaxAllTiles;
  Vec m_gmin, m_gmax;

  QString m_name;
  float m_scale;
  float m_scaleCloudJs;
  Vec m_shift;
  Quaternion m_rotation;
  float m_bminZ, m_bmaxZ;
  bool m_colorPresent;
  bool m_classPresent;
  int m_priority;
  int m_time;
  Vec m_xformCen;

  bool m_ignoreScaling;

  bool m_showMap;
  bool m_gravity;
  bool m_skybox;
  bool m_playButton;
  float m_groundHeight;
  float m_teleportScale;
  bool m_showSphere;
  bool m_pointType;
  bool m_loadAll;

  bool m_fileFormat;
  QStringList m_pointAttrib;
  int m_attribBytes;

  QList< QList<uchar> > m_vData;

  QList<OctreeNode*> m_tiles;
  QList<OctreeNode*> m_allNodes;

  QList<Label*> m_labels;

  QList<QVector4D> m_undo;

  int m_nearHitLabel;
  float m_nearHit;

  QString m_labelsJsonFile;
  
  qint64 getNumPointsInLASFile(QString);
  qint64 getNumPointsInBINFile(QString);

  void loadOctreeNodeFromJson(QString, OctreeNode*);
  void saveOctreeNodeToJson(QString, OctreeNode*);

  int setLevel(OctreeNode*, int);

  void loadModJson(QString);

  void loadCloudJson(QString);

  void loadLabelsCSV(QString);

  void loadLowerTiles(QStringList);

  void saveLabelToJson(Vec, QString);
};

#endif
