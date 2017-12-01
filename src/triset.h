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
  void draw(GLuint, GLuint, GLuint);

  int numpoints() { return m_vertices.count(); }
  
  bool loadVertexBufferData();
  bool loadVertexBufferData(GLuint, GLuint, GLuint);

  bool vboLoaded() { return m_vboLoaded; }

  QList<GLuint> vbObject();
    
 private :
  QString m_fileName;

  bool m_fileLoaded;
  bool m_vboLoaded;
  
  int m_time;
  Vec m_bmin, m_bmax;
  Vec m_gmin, m_gmax;
  
  QList<char*> plyStrings;

  int m_ntri, m_nvert;
  
  QVector<Vec> m_vertices;
  QVector<Vec> m_normals;
  QVector<Vec> m_vcolor;
  QVector<uint> m_triangles;

  float *m_vertData;
  unsigned int *m_indexData;
  
  GLuint m_glVertBuffer;
  GLuint m_glIndexBuffer;
  GLuint m_glVertArray;

  
  void clear();

  bool loadPLY(QString);

  bool loadVBO(QString);
  void saveVBO(QString);
  
  void genVBOs();
};



#endif // TRISET_H
