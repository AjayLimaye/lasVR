#ifndef VIEWER_H
#define VIEWER_H

#include "glewinitialisation.h"

#include "vr.h"

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QGLFramebufferObject>
#include <QTime>
#include <QKeyEvent>
#include <QMouseEvent>

#include "volumefactory.h"

//-------------------------------
// VR
//-------------------------------


class Viewer : public QGLViewer
{
  Q_OBJECT

 public :
  Viewer(QGLFormat&, QWidget *parent=0);
  ~Viewer();

  void setVolumeFactory(VolumeFactory *v) { m_volumeFactory = v; };
  VolumeFactory* volumeFactory() { return m_volumeFactory; };

  void setCoordMin(Vec cmin) { m_coordMin = cmin; };
  void setCoordMax(Vec cmax) { m_coordMax = cmax; };
  void setNumPoints(qint64 npt) { m_npoints = npt; };

  qint64 pointBudget() { return m_pointBudget; }

  void draw();
  void fastDraw();
  
  void wheelEvent(QWheelEvent*);

  void keyPressEvent(QKeyEvent*);
  
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mouseDoubleClickEvent(QMouseEvent*);

  void start();

  void resizeGL(int, int);

  void paintGL();

  bool pointType() { return m_pointType; }
  void setPointType(bool p) { m_pointType = p; updateGL(); }

  int pointSize() { return m_pointSize; }
  void setPointSize(int p) { m_pointSize = p; updateGL(); }

  void setFlyMode(bool f) { m_flyMode = f; toggleCameraMode(); }

  VR* vrPointer() { return &m_vr; }

  int currentTime() { return m_currTime; }

  void setNearFar(float n, float f) { m_nearDist = n; m_farDist = f; }

  Vec menuCamPos();

  bool editMode() { return m_editMode; }

  void toggleEditMode();
  void toggleCamMode();
  void centerPointClouds();
  void undo();
  void saveModInfo();

  public slots :
    void GlewInit();
    void showBox(bool);

    void vboLoaded(int, qint64);
    void vboLoadedAll(int, qint64);

    void loadLinkOnTop(QString);
    void loadLink(QString);
    void loadLink(QStringList);

    void updateFramerate();
    void setPointBudget(int);

    void setEditMode(bool);
    void setVRMode(bool);
    bool vrMode() { return m_vrMode; }

    void setKeyFrame(int);
    void updateLookFrom(Vec, Quaternion);

 signals :
    void loadLinkedData(QString);
    void loadLinkedData(QStringList);
    void switchVolume();
    void bbupdated(Vec, Vec);
    void loadPointsToVBO();
    void setVBOs(GLuint, GLuint);
    void setVisTex(GLuint);
    void stopLoading();
    void framesPerSecond(float);
    void message(QString);
    void updateView();
    void removeEditedNodes();
    void setKeyFrame(Vec, Quaternion, int, QImage);

 private :
    VR m_vr;

    Volume* m_volume;
    VolumeFactory* m_volumeFactory;

    Vec m_coordMin, m_coordMax;
    qint64 m_npoints;

    int m_firstImageDone;
    bool m_moveViewerToCenter;
    bool m_vboLoadedAll;

    QList<PointCloud*> m_pointClouds;
    QList<OctreeNode*> m_tiles;
    QList<OctreeNode*> m_orderedTiles;
    QList<OctreeNode*> m_loadNodes;
    QMultiMap<float, OctreeNode*> m_priorityQueue;

    GLhandleARB m_shadowShader;
    GLint m_shadowParm[10];

    GLhandleARB m_smoothShader;
    GLint m_smoothParm[10];
  
    bool m_fastDraw;

    bool m_pointType;
    bool m_flyMode;
    bool m_showInfo;
    bool m_showPoints;
    bool m_showBox;
    int m_pointSize;
    int m_dpv;

    int m_currTime;
    int m_maxTime;
    bool m_currTimeChanged;

    GLuint m_vertexArrayID;
    int m_vbID;
    qint64 m_vbPoints;
    GLuint m_vertexBuffer[2];

    GLuint m_vertexScreenBuffer;
    float m_scrGeo[8];

    GLuint m_depthBuffer;
    GLuint m_rbo;
    GLuint m_depthTex[4];

    GLuint m_colorTex;
    uchar *m_colorMap;
    QList<Vec> m_colorGrad;

    GLhandleARB m_depthShader;
    GLint m_depthParm[50];

    GLhandleARB m_blurShader;
    GLint m_blurParm[20];

    bool m_showEdges;
    bool m_softShadows;
    bool m_smoothDepth;
    bool m_showSphere;

    bool m_selectActive;
    qint64 m_pointBudget;
    qint64 m_pointsDrawn;

    int m_minNodePixelSize;

    int m_headSetType;

    Vec m_fruCnr[8];
    GLdouble m_planeCoefficients[6][4];

    QTime m_flyTime;

    QFile m_pFile;
    QString pickPointsFiles;
    bool m_savePointsToFile;

    GLuint m_minmaxTex;
    float *m_minmaxMap;

    GLuint m_visibilityTex;
    uchar *m_visibilityMap;

    int m_origWidth;
    int m_origHeight;

    float m_nearDist, m_farDist;

    QPoint m_backButtonPos;

    QVector3D m_hmdPos, m_hmdVD, m_hmdUP, m_hmdRV;
    Vec m_lp;

    int m_frames;

    Camera m_menuCam;

    QString m_dataDir;

    bool m_vrMode;
    bool m_editMode;
    QPoint m_prevPos;
    int m_moveAxis;
    Vec m_deltaShift;
    float m_deltaScale;
    bool m_deltaChanged;
    QList<QVector4D> m_undoXform;

    void createFBO();

    void genColorMap();

    void generateVBOs();

    void drawPointsWithReload();
    void drawPoints(vr::Hmd_Eye);
    void drawPointsWithShadows(vr::Hmd_Eye);
    
    void reset();

    void drawAABB();

    void drawInfo();

    void createShaders();

    void drawGeometry();

    Vec closestPointOnBox(Vec, Vec, Vec);

    void loadNodeData();

    void drawVAO();

    bool isVisible(Vec, Vec);
    bool isVisible(Vec, Vec,
		   int, int, int, int);

    void genDrawNodeList();
    bool genDrawNodeList(float, float);
    void orderTilesForCamera();


    void genDrawNodeListForVR();


    void calcFrustrumCornerPointsAndPlaneCoeff(float);

    bool nearCam(Vec, Vec, Vec, Vec, float);

    void savePointsToFile(Vec);

    void drawLabels();
    void drawLabelsForVR(vr::Hmd_Eye);

    void createMinMaxTexture();
    void createVisibilityTexture();

    void dummydraw();

    bool linkClicked(QMouseEvent*);

    void generateFirstImage();

    void loadTopJson(QString);
    void saveTopJson(QString);

    void stickLabelsToGround();

    void movePointCloud(QPoint);
    void scalePointCloud(QPoint);
};

#endif
