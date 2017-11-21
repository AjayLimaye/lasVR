#ifndef TRISET_H
#define TRISET_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Triset
{
 public :
  Triset();
  ~Triset();

  void setFilename(QString flnm) { m_fileName = flnm; }

  bool load();

  QString filename() { return m_fileName; }

  void setTime(int t) { m_time = t; }
  int time() { return m_time; }

    void setGlobalMinMax(Vec, Vec);

  Vec bmin() { return m_bmin; }
  Vec bmax() { return m_bmax; }  
  
  void draw();

  int numpoints() { return m_vertices.count(); }
  
 private :
  QString m_fileName;

  int m_time;
  Vec m_bmin, m_bmax;
  Vec m_gmin, m_gmax;
  
  QList<char*> plyStrings;

  QVector<Vec> m_vertices;
  QVector<Vec> m_normals;
  QVector<Vec> m_vcolor;
  QVector<uint> m_triangles;

  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;

  
  void clear();

  bool loadPLY(QString);
  
  void loadVertexBufferData();
};



#endif // TRISET_H
