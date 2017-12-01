#ifndef GLHIDDENWIDGET_H
#define GLHIDDENWIDGET_H

#include "glewinitialisation.h"

#include "viewer.h"
#include "vr.h"
#include "volumefactory.h"

#include <QGLWidget>
#include <QMutex>
#include <QPair>

class GLHiddenWidget : public QGLWidget
{
  Q_OBJECT
 public:
  GLHiddenWidget(QGLFormat, QWidget*, QGLWidget*);
  ~GLHiddenWidget();
  
  void setVR(VR* vr) { m_vr = vr;}
  void setViewer(Viewer*);

  void setVolume(Volume*);

  void setVolumeFactory(VolumeFactory *v) { m_volumeFactory = v; }

  bool loading() { return m_loading; }

  void setPointBlockSize(qint64 pbs) { m_pointBlockSize = pbs; }

  void clearPrevNodes() { m_prevNodes.clear(); }
  
  public slots:
    void switchVolume();
    void setVBOs(GLuint, GLuint);  
    void setVisTex(GLuint);
    void loadPointsToVBO();
    void stopLoading();
    void updateView();
    void removeEditedNodes();

 signals :
    void vboLoaded(int, qint64);
    void vboLoadedAll(int, qint64);
    void message(QString);

    void meshLoadedAll();
    
 protected:
    virtual void glInit();
    virtual void glDraw();
    
    virtual void initializeGL();
    virtual void resizeGL(int width, int height);
    virtual void paintGL();
    
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *);
    
 private:
    Volume* m_volume;
    VolumeFactory* m_volumeFactory;

    Viewer *m_viewer;
    VR *m_vr;

    qint64 m_pointBlockSize;
    int m_dpv;
    
    int m_currVBO;
    GLuint m_vertexBuffer[2];

    QMutex m_mutex;
    bool m_loading;
    bool m_stopLoading;

    QMap<int, QPair<qint64, qint64> > m_prevNodes;

    int m_currTime;
    float m_fov, m_slope, m_projFactor;

    
    QList<Triset*> m_trisets;
    
      
    QList<PointCloud*> m_pointClouds;
    QList<OctreeNode*> m_tiles;
    QList<OctreeNode*> m_orderedTiles;
    QList<OctreeNode*> m_newNodes;
    QMultiMap<float, OctreeNode*> m_priorityQueue;

    bool m_firstLoad;

    GLuint m_visibilityTex;
    uchar *m_visibilityMap;

    qint64 m_pointsDrawn;
    qint64 m_pointBudget;
    int m_minNodePixelSize;
    bool m_newVisTex;

    void genDrawNodeList();
    void genPriorityQueue(Vec);
    void orderTiles(Vec);
    void createVisibilityTexture();

    int m_ntiles, m_maxwd;
    void uploadVisTex();

};

#endif
