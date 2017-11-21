#ifndef VOLUME_H
#define VOLUME_H

#include <QThread>

#include <QProgressDialog>
#include <QFile>
#include <QJsonDocument>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include "triset.h"
#include "pointcloud.h"
#include "volumeloaderthread.h"

class Volume : public QObject
{
 Q_OBJECT

 public :
  Volume();
  ~Volume();

  void reset();

  void setShiftToZero(bool b) { m_zeroShift = b; }
  bool shiftToZero() { return m_zeroShift; }

  bool loadOnTop(QString);
  bool loadDir(QString);

  bool loadTiles(QStringList);

  QStringList filenames() { return m_filenames; }

  Vec coordMin() { return m_coordMin; };
  Vec coordMax() { return m_coordMax; };

  qint64 numPoints() { return m_npoints; };
  float *coordPtr() { return m_coord; };
  uchar *colorPtr() { return m_color; };

  QList<OctreeNode*> tiles() { return m_tiles; }
  QList<PointCloud*> pointClouds() { return m_pointClouds; }

  QList<Triset*> trisets() { return m_trisets; }

  void setNewLoad(bool nl) { m_newLoad = nl; }
  void setLoadingNodes(QList<OctreeNode*> lon) { m_newLoad = true; m_loadingNodes = lon;}
  QList<OctreeNode*> getLoadingNodes() { m_newLoad = false; return m_loadingNodes; }

  bool newLoad() { return m_newLoad; }
  void resetNewLoad() { m_newLoad = false; }
  
  void postLoad(bool);


  int dataPerVertex() { return m_dpv; }
  int maxTime();
  bool timeseries() { return m_timeseries; }

  bool validCamera() { return m_validCamera; }
  void setCamera(Camera*);
  Vec getCamPosition() { return m_camPosition; }
  Quaternion getCamOrientation() { return m_camOrientation; }
  Vec getCamSceneCenter() { return m_camSceneCenter; }
  Vec getCamPivot() { return m_camPivot; }

  int xformNodeId() { return m_xformNodeId; }
  int xformTileId() { return m_xformTileId; }

 signals :
  void startLoading();

 private :
  QThread *m_thread;
  VolumeLoaderThread *m_lt;

  QStringList m_filenames;

  bool m_zeroShift;
  int m_xformNodeId;
  int m_xformTileId;

  qint64 m_npoints;
  float *m_coord;
  uchar *m_color;

  int m_dpv;

  float m_teleportScale;
  float m_scale;
  Vec m_shift;
  float m_bminZ, m_bmaxZ;
  bool m_xformPresent;
  float m_xform[16];
  int m_priority;
  int m_time;
  bool m_colorPresent;

  Vec m_coordMin;
  Vec m_coordMax;

  QList<Triset*> m_trisets;
  QList<PointCloud*> m_pointClouds;
  QList<OctreeNode*> m_tiles;
  QList<OctreeNode*> m_loadingNodes;
  bool m_newLoad;

  bool m_showMap;
  bool m_gravity;
  bool m_skybox;
  bool m_playbutton;
  bool m_showSphere;
  float m_groundHeight;
  bool m_pointType;
  bool m_loadall;

  bool m_validCamera;
  Vec m_camPosition;
  Quaternion m_camOrientation;
  Vec m_camSceneCenter;
  Vec m_camPivot;

  bool m_timeseries;
  bool m_ignoreScaling;


  void pruneOctreeNodesBasedOnPriority();

  bool checkBoxXY(QList<OctreeNode*>, Vec, Vec);
  QList<OctreeNode*> getAllLeaves(OctreeNode*);
  QList<OctreeNode*> getNodesAtLevel(OctreeNode*, int);
  QList<OctreeNode*> getNodesAtLevelBelow(OctreeNode*, int);

  int cullNodes(OctreeNode*, QList<OctreeNode*>);
  QList<OctreeNode*> getNodesWithOccupiedLeaves(OctreeNode*, QList<int>);

  void loadTopJson(QString);

  void loadAllPLY(QString);
};

#endif
