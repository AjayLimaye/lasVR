#ifndef OCTREENODE_H
#define OCTREENODE_H

#include <QGLViewer/quaternion.h>
using namespace qglviewer;

#include <QList>

class OctreeNode
{
 public :
  OctreeNode();
  ~OctreeNode();

  QString fileName() { return m_fileName; }

  bool inBox(Vec);
  bool inBoxXY(Vec);

  bool isLeaf();


  void setParent(OctreeNode *p) { m_parent = p; }
  void setPriority(int i) { m_priority = i; }
  void setTime(int i) { m_time = i; }
  void setId(int);
  void setUId(int i) { m_uid = i; }
  void setScale(float, float);
  void setShift(Vec);
  void setRotation(Quaternion);
  void setXform(float, Vec, Quaternion, Vec);
  void setActive(bool a) { m_active = a; }
  void setFileName(QString flnm) { m_fileName = flnm; }
  void setLevelString(QString l) { m_levelString = l; }
  void setOffset(Vec off) { m_offset = off; }
  void setBMin(Vec bmin) { m_bmin = m_bminO = bmin; }
  void setBMax(Vec bmax) { m_bmax = m_bmaxO = bmax; }
  void setTightMin(Vec bmin) { m_tightMin = m_tightMinO = bmin; }
  void setTightMax(Vec bmax) { m_tightMax = m_tightMaxO = bmax; }
  void setNumPoints(qint64 n) { m_numpoints = n; }

  void setPointAttributes(QStringList s) { m_pointAttrib = s; }
  void setAttribBytes(int a) { m_attribBytes = a; }

  void addChild(int i, OctreeNode* o) { m_child[i] = o; }
  void setLevel(int l) { m_level = l; }
  void setLevelsBelow(int l) { m_levelsBelow = l; }
  void setDataPerVertex(int d) { m_dpv = d; }
  void setColorPresent(bool b) { m_colorPresent = b; }
  void setClassPresent(bool b) { m_classPresent = b; }
  void setColorMap(QList<Vec> cm) { m_colorMap = cm; }
  void setGlobalMinMax(Vec, Vec);
  void setPointSize(float ps) { m_pointSize = ps; }
  void setPointSizeFactor(float ps) { m_pointSize *= ps; }
  void setSpacing(float s) { m_spacing = s; }


  void loadData();
  void unloadData();
  void reloadData();


  OctreeNode* parent() { return m_parent; }
  int priority() { return m_priority; }
  int time() { return m_time; }
  int id() { return m_id; }
  int uid() { return m_uid; }
  bool isActive() { return m_active; }
  QString filename() { return m_fileName; }
  QString levelString() { return m_levelString; }
  Vec bmin() { return m_bmin; }
  Vec bmax() { return m_bmax; }
  qint64 numpoints() { return m_numpoints; }
  uchar* coords() { return m_coord; }
  OctreeNode* getChild(int i) { return m_child[i]; }
  OctreeNode* childAt(int); // will create child if not present
  int level() { return m_level; }
  int levelsBelow() { return m_levelsBelow; }
  int dataPerVertex() { return m_dpv; }
  Vec shift() { return m_shift; }
  float pointSize() { return m_pointSize; }
  float spacing() { return m_spacing; }
  uchar maxVisibleLevel() { return m_maxVisLevel; }

  void computeBB();

  Vec tightOctreeMin() { return m_tightMin; }
  Vec tightOctreeMax() { return m_tightMax; }

  uchar setMaxVisibleLevel();

  void markForDeletion();
  bool markedForDeletion() { return m_removalFlag; }

  QList<OctreeNode*> allActiveNodes();
  int setPointSizeForActiveNodes(float);

  void setEditMode(bool b) { m_editMode = b; }

  static void pruneDeletedNodes();

 private :
  OctreeNode* m_parent;

  int m_priority;
  int m_time;
  int m_id, m_uid;

  Quaternion m_rotation;
  Vec m_shift;
  float m_scale;
  float m_scaleCloudJs;
  Vec m_xformCen;

  float m_pointSize;

  bool m_active;
  QString m_fileName;
  QString m_levelString;
  Vec m_offset;
  Vec m_bmin, m_bmax;
  Vec m_tightMin, m_tightMax;
  Vec m_bminO, m_bmaxO;
  Vec m_tightMinO, m_tightMaxO;
  qint64 m_numpoints;
  uchar *m_coord;
  OctreeNode* m_child[8];
  int m_level;
  uchar m_maxVisLevel;
  int m_levelsBelow;
  int m_dpv;
  bool m_colorPresent;
  bool m_classPresent;
  float m_spacing;

  QStringList m_pointAttrib;
  int m_attribBytes;

  QList<Vec> m_colorMap;
  Vec m_globalMin;
  Vec m_globalMax;

  bool m_dataLoaded;

  bool m_removalFlag;

  bool m_editMode;

  void loadDataFromLASFile();
  void loadDataFromBINFile();

  Vec xformPoint(Vec);
};

#endif
