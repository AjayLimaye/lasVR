#ifndef VR_H
#define VR_H

#include "vrmenu.h"

#include <QFile>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector3D>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


#include <openvr.h>

#include "cglrendermodel.h"

class VR : public QObject
{
 Q_OBJECT

 public :
  VR();
  ~VR();


  void shutdown();

  void initVR();
  bool vrEnabled() { return (m_hmd > 0); }

  int screenWidth() { return m_eyeWidth; }
  int screenHeight() { return m_eyeHeight; }

  void initModel(QVector3D, QVector3D);

  void setDataDir(QString);

  void setShowMap(bool);
  void setShowTimeseriesMenu(bool);
  void setGroundHeight(float gh) { m_groundHeight = gh; }
  void setTeleportScale(float gh) { m_teleportScale = gh; }
  void setCurrPosOnMenuImage(Vec, Vec);
  void sendTeleportsToMenu(QList<QVector3D>);
  QVector2D pinPoint2D();
  void setProjectedPinPoint(QVector3D p) { m_projectedPinPt = p; }

  float* depthBuffer() { return m_depthBuffer; }

  bool genDrawList() { return m_genDrawList; }
  void resetGenDrawList() { m_genDrawList = false; }

  void resetNextStep() { m_nextStep = 0; }
  int nextStep() { return m_nextStep; }
  void setTimeStep(QString);


  QVector3D hmdPosition()
  { return getModelSpacePosition(vr::k_unTrackedDeviceIndex_Hmd); }

  QVector3D leftControllerPosition()
  { return getModelSpacePosition(m_leftController); }

  QVector3D rightControllerPosition()
  { return getModelSpacePosition(m_rightController); }

  QVector3D vrHmdPosition()
  { return getPosition(vr::k_unTrackedDeviceIndex_Hmd); }

  QVector3D vrLeftControllerPosition()
  { return getPosition(m_leftController); }

  QVector3D vrRightControllerPosition()
  { return getPosition(m_rightController); }


  QVector3D viewDir();
  QVector3D upDir();
  QVector3D rightDir();

  QVector3D vrViewDir();
  QVector3D vrUpDir();
  QVector3D vrRightDir();


  QMatrix4x4 viewProjection(vr::Hmd_Eye);
  QMatrix4x4 currentViewProjection(vr::Hmd_Eye);
  QMatrix4x4 modelView(vr::Hmd_Eye eye);
  QMatrix4x4 modelView();

  QMatrix4x4 matrixDevicePoseLeft();
  QMatrix4x4 matrixDevicePoseRight();

  QMatrix4x4 final_xform() { return m_final_xform; }


  void preDraw();
  void postDraw();

  void bindBuffer(vr::Hmd_Eye);

  void bindLeftBuffer();
  void bindRightBuffer();

  void postDrawLeftBuffer();
  void postDrawRightBuffer();


  bool newTeleportFound() { return m_newTeleportFound; }

  bool play() { return m_play; }
  void gotoNextStep()
  {
    m_genDrawList = true;
    m_nextStep = 1;
  }


  void bindMapBuffer();
  void menuImageFromMapBuffer();

  float scaleFactor() { return m_scaleFactor; }
  float flightSpeed() { return m_flightSpeed; }
  float pointSize() { return m_pointSize; }
  
 public slots:
  void resetModel();
  void updatePointSize(int);
  void updateScale(int);  

 private :
  vr::IVRSystem *m_hmd;
  vr::TrackedDevicePose_t m_trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  QMatrix4x4 m_matrixDevicePose[vr::k_unMaxTrackedDeviceCount];
  
  vr::TrackedDeviceIndex_t m_leftController;
  vr::TrackedDeviceIndex_t m_rightController;
  
  vr::IVRRenderModels *m_pRenderModels;
  bool m_rbShowTrackedDevice[ vr::k_unMaxTrackedDeviceCount ];
  QList< CGLRenderModel * > m_vecRenderModels;
  CGLRenderModel *m_rTrackedDeviceToRenderModel[ vr::k_unMaxTrackedDeviceCount ];

  QMatrix4x4 m_leftProjection, m_leftPose;
  QMatrix4x4 m_rightProjection, m_rightPose;
  QMatrix4x4 m_hmdPose;

  QMatrix4x4 m_las_xform;
  QMatrix4x4 m_model_xform;
  QMatrix4x4 m_final_xform;
  QMatrix4x4 m_final_xformInverted;
  QMatrix4x4 m_scaleMat;
  
  uint32_t m_eyeWidth, m_eyeHeight;
  
  QOpenGLFramebufferObject *m_mapBuffer;
  QOpenGLFramebufferObject *m_leftBuffer;
  QOpenGLFramebufferObject *m_rightBuffer;
  QOpenGLFramebufferObject *m_resolveBuffer;
  
  
  bool m_gripClickedLeft;
  bool m_gripClickedRight;
  
  bool m_triggerActiveRight;
  bool m_triggerActiveLeft;
  
  bool m_touchActiveRight;
  bool m_touchActiveLeft;

  bool m_touchTriggerActiveRight;
  bool m_touchTriggerActiveLeft;
  
  bool m_gripActiveRight;
  bool m_gripActiveLeft;
  
  QTimer m_flyTimer;

  QVector3D m_prevDirection;
  QVector3D m_prevPosL, m_prevPosR;
  float m_lrDist;
  bool m_bothTriggersActive;

  bool m_genDrawList;

  int m_nextStep;

  bool m_play;

  QVector3D m_startTranslate;
  QVector3D m_scaleCenter;
  
  QQuaternion m_rotQuat;
  QMatrix4x4 m_rotQ;
  QVector3D m_rotCenter;
  
  QVector2D m_touchPoint;
  
  float m_touchX, m_touchY;
  float m_startTouchX, m_startTouchY;
  float m_pointSize;
  float m_startPS;

  float m_scaleFactor;
  float m_prevScaleFactor;

  float m_speedDamper;
  float m_flightSpeed;
  float m_prevFlightSpeed;

  QVector3D m_coordCen;
  float m_coordScale;

  bool m_showLeft, m_showRight;  

  //-----------------
  GLhandleARB m_pshader;
  GLint m_pshaderParm[10];
  //-----------------

  //-----------------
  GLuint m_boxVID;
  GLuint m_boxV;
  int m_axesPoints;
  int m_nboxPoints;
  //-----------------

  QString m_dataDir;
  float *m_depthBuffer;


  QList<QVector3D> m_teleports;
  bool m_newTeleportFound;
  int m_currTeleportNumber;
  QJsonObject m_teleportJsonInfo;
  int m_telPoints;
  QVector2D m_pinPt;
  QVector3D m_projectedPinPt;
  int m_pinPoints;

  bool m_showMap;
  float m_groundHeight;
  float m_teleportScale;

  VrMenu m_leftMenu;
  QStringList m_menuPanels;
  int m_currPanel;



  void buildAxes();
  void buildTeleport();
  void renderAxes(vr::Hmd_Eye);
  void createShaders();
  void buildAxesVB();


  QString getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
				 vr::TrackedDeviceProperty prop,
				 vr::TrackedPropertyError *error = 0);
  
  void updatePoses();
  void updateInput();

  void ProcessVREvent(const vr::VREvent_t & event);
  
  void renderEye(vr::Hmd_Eye eye);

  QMatrix4x4 hmdMVP(vr::Hmd_Eye);
  QMatrix4x4 leftMVP(vr::Hmd_Eye);
  QMatrix4x4 rightMVP(vr::Hmd_Eye);

  QVector3D getPosition(const vr::HmdMatrix34_t);
  QVector3D getPosition(QMatrix4x4);
  float copysign(float, float);
  QQuaternion getQuaternion(vr::HmdMatrix34_t);
  QQuaternion getQuaternion(QMatrix4x4);
  QVector3D getModelSpacePosition(vr::TrackedDeviceIndex_t);
  QVector3D getPosition(vr::TrackedDeviceIndex_t);
  
  
  QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix34_t&);
  QMatrix4x4 vrMatrixToQt(const vr::HmdMatrix44_t&);
    
  QMatrix4x4 modelViewNoHmd();
  
    
  bool isTriggered(vr::VRControllerState_t&);
  bool isTouched(vr::VRControllerState_t&);
  
  void handleBothTriggersPressed(vr::TrackedDeviceIndex_t,
				 vr::TrackedDeviceIndex_t);
  
  void handleControllerRight(vr::TrackedDeviceIndex_t,
			     bool&, bool&);
  void handleControllerLeft(vr::TrackedDeviceIndex_t,
			    bool&, bool&);

  void handleFlight(vr::TrackedDeviceIndex_t,
		    bool&, bool&);
  
  void handleGripRight(vr::TrackedDeviceIndex_t,
		       bool&);
  void handleGripLeft(vr::TrackedDeviceIndex_t,
			bool&);
  
  QMatrix4x4 quatToMat(QQuaternion); 


  void renderControllers(vr::Hmd_Eye);
  void setupRenderModels();
  void setupRenderModelForTrackedDevice( vr::TrackedDeviceIndex_t unTrackedDeviceIndex );
  CGLRenderModel *findOrLoadRenderModel(QString);


  void genEyeMatrices();


  void saveTeleportNode();
  void teleport();
  void teleport(QVector3D);
  void loadTeleports();
  void checkTeleport(bool);
  void renderTeleport(vr::Hmd_Eye);

  void buildPinPoint();


  void nextMenu();
  void previousMenu();
  void renderMenu(vr::Hmd_Eye);
  void checkOptions(bool);

  bool showRight() { return m_showRight; }
  bool showLeft() { return m_showLeft; }

  QMatrix4x4 initXform(float, float, float, float);

  void gotoFirstStep()
  {
    // set a very high next step so that
    // it is reset to 0 in the viewer
    m_genDrawList = true;
    m_nextStep = 1000000;
  }

  void gotoPreviousStep()
  {
    m_genDrawList = true;
    m_nextStep = -1;
  }
};


#endif
