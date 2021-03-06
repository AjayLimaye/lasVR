#ifndef VR_H
#define VR_H

#include "vrmenu.h"
#include "cubemap.h"
#include "captionwidget.h"

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

  void setHeadSetType(int hs) { m_headSetType = hs; }

  int screenWidth() { return m_eyeWidth; }
  int screenHeight() { return m_eyeHeight; }

  void initModel(QVector3D, QVector3D);

  void setDataDir(QString);

  bool pointingToMenu() { return m_leftMenu.pointingToMenu(); }

  void setShowMap(bool);
  void setGravity(bool g) { m_gravity = g; }
  void setSkyBox(bool sm) { m_showSkybox = sm; }
  void setSkyBoxType(int);
  void setPlayButton(bool sm) { m_leftMenu.setPlayButton(sm); }
  void setShowTimeseriesMenu(bool);
  void setGroundHeight(float gh) { m_groundHeight = gh; }
  void setTeleportScale(float gh) { m_teleportScale = gh; }
  void setCurrPosOnMenuImage(Vec, Vec);
  void sendTeleportsToMenu();

  float* depthBuffer() { return m_depthBuffer; }

  void setGenDrawList(bool b) { m_genDrawList = b; }
  bool genDrawList() { return m_genDrawList; }
  void resetGenDrawList() { m_genDrawList = false; }

  void resetNextStep() { m_nextStep = 0; }
  int nextStep() { return m_nextStep; }
  void takeNextStep()
  {
    m_genDrawList = true;
    m_nextStep = 1;
  }

  void setTimeStep(QString);
  void setDataShown(QString);

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
  QMatrix4x4 final_xformInverted() { return m_final_xformInverted; }


  bool preDraw();
  void postDraw();

  void bindBuffer(vr::Hmd_Eye);

  void bindLeftBuffer();
  void bindRightBuffer();

  void postDrawLeftBuffer();
  void postDrawRightBuffer();


  bool newTeleportFound() { return m_newTeleportFound; }

  bool play() { return m_play; }


  void bindMapBuffer();
  void menuImageFromMapBuffer();

  float scaleFactor() { return m_scaleFactor; }
  float flightSpeed() { return m_flightSpeed; }
  float pointSize() { return m_pointSize; }

  bool spheres() { return m_spheres; }
  bool edges() { return m_edges; }
  bool softShadows() { return m_softShadows; }

  void sendCurrPosToMenu();


  float deadRadius() { return m_deadRadius; }
  QVector3D deadPoint() { return m_deadPoint; }

  void renderSkyBox(vr::Hmd_Eye);
  void showLabel(GLuint, QSize);
  void hideLabel();

  bool reUpdateMap() { return m_updateMap; }
  void resetUpdateMap() { m_updateMap = false; }

  void teleport(QVector3D);

 public slots:
  void resetModel();
  void updateMap();
  void updatePointSize(int);
  void updateScale(int);  
  void updateSoftShadows(bool);
  void updateEdges(bool);
  void updateSpheres(bool);
  void gotoFirstStep();
  void gotoPreviousStep();
  void gotoNextStep();
  void playPressed(bool);
  void toggle(QString, QString);
    
 signals :
  void addTempLabel(Vec, QString);
  void moveTempLabel(Vec);
  void makeTempLabelPermanent();
  void addLabel(Vec, QString);
    
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

  QString m_leftControllerName;
  QString m_rightControllerName;
  QStringList m_leftComponentNames;
  QStringList m_rightComponentNames;
  QList< CGLRenderModel* > m_leftRenderModels;
  QList< CGLRenderModel* > m_rightRenderModels;

  QMatrix4x4 m_leftProjection, m_leftPose;
  QMatrix4x4 m_rightProjection, m_rightPose;
  QMatrix4x4 m_hmdPose;

  QMatrix4x4 m_las_xform;
  QMatrix4x4 m_model_xform;
  QMatrix4x4 m_final_xform;
  QMatrix4x4 m_final_xformInverted;
  QMatrix4x4 m_scaleMat;
  
  int m_headSetType;
  uint32_t m_eyeWidth, m_eyeHeight;
  
  QOpenGLFramebufferObject *m_mapBuffer;
  QOpenGLFramebufferObject *m_leftBuffer;
  QOpenGLFramebufferObject *m_rightBuffer;
  QOpenGLFramebufferObject *m_resolveBuffer;
  
    
  bool m_triggerActiveRight;
  bool m_triggerActiveLeft;
  bool m_triggerActiveBoth;
  
  bool m_touchPressActiveRight;
  bool m_touchPressActiveLeft;

  bool m_touchActiveRight;
  bool m_touchActiveLeft;

  bool m_gripActiveRight;
  bool m_gripActiveLeft;

  bool m_flightActive;
//  bool m_touchTriggerActiveLeft;
  
  bool m_updateMap;

  QTimer m_flyTimer;

  QVector3D m_prevDirection;
  QVector3D m_prevPosL, m_prevPosR;
  float m_lrDist;
  //bool m_bothTriggersActive;

  bool m_genDrawList;

  int m_nextStep;

  bool m_play;

  QVector3D m_startTranslate;
  QVector3D m_scaleCenter;
  QVector3D m_rcDir;
  
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

  QVector3D m_coordMin;
  QVector3D m_coordMax;
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
  GLuint m_boxIB;
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
  bool m_gravity;
  bool m_showSkybox;
  float m_groundHeight;
  float m_teleportScale;

  VrMenu m_leftMenu;
  QStringList m_menuPanels;
  int m_currPanel;

  QList<CaptionWidget*> m_captionWidgets;

  CubeMap m_skybox;

  bool m_spheres;
  bool m_edges;
  bool m_softShadows;


  float m_deadRadius;
  QVector3D m_deadPoint;

  int m_annoMode;
  float m_moveAnnotation;

  void buildAxes();
  bool buildTeleport();
  void renderAxes(vr::Hmd_Eye);
  void createShaders();
  void buildAxesVB();


  QString getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
				 vr::TrackedDeviceProperty prop,
				 vr::TrackedPropertyError *error = 0);
  
  void updatePoses();
  bool updateInput();

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
  
  vr::VRControllerState_t m_stateRight;
  vr::VRControllerState_t m_stateLeft;
    
  bool isTriggered(vr::VRControllerState_t&);
  bool isTouched(vr::VRControllerState_t&);
  bool isTouchPressed(vr::VRControllerState_t&);
  bool isGripped(vr::VRControllerState_t&);

  void leftTriggerPressed();
  void leftTriggerMove();
  void leftTriggerReleased();

  void rightTriggerPressed();
  void rightTriggerMove();
  void rightTriggerReleased();

  void bothTriggerPressed();
  void bothTriggerMove();
  void bothTriggerReleased();

  void leftTouchPressed();
  void leftTouchPressMove();
  void leftTouchPressReleased();

  void leftTouched();
  void leftTouchMove();
  void leftTouchReleased();

  void rightTouchPressed();
  void rightTouchPressMove();
  void rightTouchPressReleased();

  void rightTouched();
  void rightTouchMove();
  void rightTouchReleased();

  void leftGripPressed();
  void leftGripMove();
  void leftGripReleased();

  void rightGripPressed();
  void rightGripMove();
  void rightGripReleased();
  
  bool m_xActive;
  bool m_yActive;
  bool isXYTriggered(vr::VRControllerState_t&);
  void xButtonPressed();
  void xButtonMoved();
  void xButtonReleased();
  void yButtonPressed();
  void yButtonMoved();
  void yButtonReleased();

  QMatrix4x4 quatToMat(QQuaternion); 


  bool getControllers();
  void renderControllers(vr::Hmd_Eye);
  void setupRenderModels();
  void setupRenderModelForTrackedDevice(int, vr::TrackedDeviceIndex_t);
  CGLRenderModel *findOrLoadRenderModel(QString);


  void genEyeMatrices();


  void saveTeleportNode();
  void teleport();
  void loadTeleports();
  void checkTeleport(bool);
  void renderTeleport(vr::Hmd_Eye);

  void buildPinPoint();

  int m_gotoMenu;
  void nextMenu();
  void previousMenu();
  void renderMenu(vr::Hmd_Eye);
  void checkOptions(int);

  bool showRight() { return m_showRight; }
  bool showLeft() { return m_showLeft; }

  QMatrix4x4 initXform(float, float, float, float);

  void projectPinPoint();
  bool nextHit();

  void loadAnnotationIcons();

  void preAnnotation();
  void moveAnnotation();
  void fixAnnotation();
};


#endif
