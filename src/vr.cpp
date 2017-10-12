#include "glewinitialisation.h"
#include "global.h"
#include "staticfunctions.h"
#include "shaderfactory.h"

#include "vr.h"
#include <QMessageBox>
#include <QtMath>
#include <QDir>

#define NEAR_CLIP 0.1f
#define FAR_CLIP 2.0f


VR::VR() : QObject()
{
  m_hmd =0;
  m_eyeWidth = 0;
  m_eyeHeight = 0;
  m_leftBuffer = 0;
  m_rightBuffer = 0;
  m_mapBuffer = 0;

  m_gripActiveRight = false;
  m_gripActiveLeft = false;
  m_touchActiveRight = false;
  m_touchActiveLeft = false;
  m_touchPressActiveRight = false;
  m_touchPressActiveLeft = false;
  m_triggerActiveRight = false;
  m_triggerActiveLeft = false;
  m_triggerActiveBoth = false;
  
  m_showLeft = true;
  m_showRight = true;
  
  m_final_xform.setToIdentity();    

  m_genDrawList = false;
  m_nextStep = 0;

  m_pointSize = 1.5;

  m_coordCen = QVector3D(0,0,0);
  m_coordScale = 1.0;
  m_scaleFactor = 1.0;
  m_teleportScale = 1.0;
  m_flightSpeed = 1.0;
  m_speedDamper = 0.1;

  m_flyTimer.setSingleShot(true);

  m_newTeleportFound = false;
  m_teleports.clear();

  m_depthBuffer = 0;

  m_pinPt = QVector2D(-1,-1);

  m_groundHeight = 0.16;

  m_menuPanels = m_leftMenu.menuList();
  m_currPanel = -1;
  m_leftMenu.setCurrentMenu("none");

  m_edges = true;
  m_softShadows = true;

  m_updateMap = false;

  connect(&m_leftMenu, SIGNAL(resetModel()),
	  this, SLOT(resetModel()));
  connect(&m_leftMenu, SIGNAL(updateMap()),
	  this, SLOT(updateMap()));
  connect(&m_leftMenu, SIGNAL(updatePointSize(int)),
	  this, SLOT(updatePointSize(int)));
  connect(&m_leftMenu, SIGNAL(updateScale(int)),
	  this, SLOT(updateScale(int)));

  connect(&m_leftMenu, SIGNAL(updateEdges(bool)),
	  this, SLOT(updateEdges(bool)));
  connect(&m_leftMenu, SIGNAL(updateSoftShadows(bool)),
	  this, SLOT(updateSoftShadows(bool)));

  connect(&m_leftMenu, SIGNAL(gotoFirstStep()),
	  this, SLOT(gotoFirstStep()));
  connect(&m_leftMenu, SIGNAL(gotoPreviousStep()),
	  this, SLOT(gotoPreviousStep()));
  connect(&m_leftMenu, SIGNAL(gotoNextStep()),
	  this, SLOT(gotoNextStep()));
  connect(&m_leftMenu, SIGNAL(playPressed(bool)),
	  this, SLOT(playPressed(bool)));
}

void
VR::updatePointSize(int sz)
{
  if (m_showMap)
    m_pointSize = qMax(0.5f, m_pointSize + sz*0.1f);
  else
    m_pointSize = qMax(0.5f, m_pointSize + sz);
}

void VR::updateEdges(bool b) { m_edges = b; }
void VR::updateSoftShadows(bool b) { m_softShadows = b; }

void
VR::gotoFirstStep()
{
  // set a very high next step so that
  // it is reset to 0 in the viewer
  m_genDrawList = true;
  m_nextStep = 1000000;
  m_play = false;
  m_leftMenu.setPlay(false);
}
void
VR::gotoPreviousStep()
{
  m_genDrawList = true;
  m_nextStep = -1;
  m_play = false;
  m_leftMenu.setPlay(false);
}
void
VR::gotoNextStep()
{
  m_genDrawList = true;
  m_nextStep = 1;
  m_play = false;
  m_leftMenu.setPlay(false);
}
void
VR::playPressed(bool b)
{
  m_play = b;
  if (m_play)
    {
      m_genDrawList = true;
      m_nextStep = 1;
    }
}

void
VR::setDataDir(QString d)
{
  m_dataDir = d;
  m_currTeleportNumber = -1;
  m_newTeleportFound = false;

  m_menuPanels = m_leftMenu.menuList();
  m_currPanel = -1;
  m_leftMenu.setCurrentMenu("none");
}

VR::~VR()
{
  shutdown();
}

void
VR::shutdown()
{
  delete m_mapBuffer;
  delete m_leftBuffer;
  delete m_rightBuffer;
  delete m_resolveBuffer;
  
  if (m_hmd)
    {
      vr::VR_Shutdown();
        m_hmd = 0;
    }


  for( int i=0; i<m_vecRenderModels.count(); i++)
    delete m_vecRenderModels[i];
  m_vecRenderModels.clear();
	
}

void
VR::updateMap()
{
  if (m_showMap)
    m_updateMap = true;
  else
    m_updateMap = false;
}

void
VR::resetModel()
{
  m_final_xform = m_las_xform;

  m_scaleFactor = m_coordScale;
  m_flightSpeed = 1.0;

  m_model_xform.setToIdentity();

  genEyeMatrices();

  m_play = false;

  
  QVector3D cmid = (m_coordMin+m_coordMax)/2;
  cmid.setZ(m_coordMax.z());
  teleport(cmid);
}

QMatrix4x4
VR::initXform(float x, float y, float z, float a)
{
  QMatrix4x4 xform;
  xform.setToIdentity();
  xform.translate(QVector3D(0, 1.0, 0));
  if (qAbs(a) > 0)
    xform.rotate(a, x, y, z);
  xform.scale(m_coordScale);
  xform.translate(-m_coordCen);
  return xform;
}

void
VR::initModel(QVector3D cmin, QVector3D cmax)
{
  QVector3D box = cmax - cmin;
  m_coordScale = 1.0/qMax(box.x(), qMax(box.y(), box.z()));

  m_coordMin = cmin;
  m_coordMax = cmax;
  m_coordCen = (cmin+cmax)/2;

  m_las_xform = initXform(1, 0, 0, -90);

  m_final_xform = m_las_xform;

  m_model_xform.setToIdentity();

  m_scaleFactor = m_coordScale;
  m_flightSpeed = 1.0;
  m_teleportScale = 1.0;


  genEyeMatrices();

  m_play = false;
}

void
VR::initVR()
{
  vr::EVRInitError error = vr::VRInitError_None;

  //-----------------------------
  m_hmd = vr::VR_Init(&error, vr::VRApplication_Scene);
  if (error != vr::VRInitError_None)
    {
      m_hmd = 0;
      
      QString message = vr::VR_GetVRInitErrorAsEnglishDescription(error);
      qCritical() << message;
      QMessageBox::critical(0, "Unable to init VR", message);
      //exit(0);
      return;
    }
  //-----------------------------
    
  m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &error );


  genEyeMatrices();


  //-----------------------------
  // setup frame buffers for eyes
  m_hmd->GetRecommendedRenderTargetSize(&m_eyeWidth, &m_eyeHeight);
  
  QOpenGLFramebufferObjectFormat mapbuffFormat;
  mapbuffFormat.setAttachment(QOpenGLFramebufferObject::Depth);
  mapbuffFormat.setInternalTextureFormat(GL_RGBA);
  mapbuffFormat.setSamples(0);  
  m_mapBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, mapbuffFormat);


  QOpenGLFramebufferObjectFormat buffFormat;
  buffFormat.setAttachment(QOpenGLFramebufferObject::Depth);
  buffFormat.setInternalTextureFormat(GL_RGBA16F);
  buffFormat.setSamples(8);
  m_leftBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);
  m_rightBuffer = new QOpenGLFramebufferObject(m_eyeWidth, m_eyeHeight, buffFormat);
  
  QOpenGLFramebufferObjectFormat resolveFormat;
  resolveFormat.setInternalTextureFormat(GL_RGBA);
  buffFormat.setSamples(0);
  
  m_resolveBuffer = new QOpenGLFramebufferObject(m_eyeWidth*2, m_eyeHeight, resolveFormat);
  //-----------------------------
  

  //-----------------------------
  // turn on compositor
  if (!vr::VRCompositor())
    {
      QString message = "Compositor initialization failed. See log file for details";
      qCritical() << message;
      QMessageBox::critical(0, "Unable to init VR", message);
      exit(0);
    }
  //-----------------------------
  
  //#ifdef QT_DEBUG
  vr::VRCompositor()->ShowMirrorWindow();
  //#endif

  createShaders();

  setupRenderModels();

  buildAxesVB();


// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
  QStringList clist;
  clist << "assets/images/right.jpg";
  clist << "assets/images/left.jpg";
  clist << "assets/images/top.jpg";
  clist << "assets/images/bottom.jpg";
  clist << "assets/images/back.jpg";
  clist << "assets/images/front.jpg";
  m_skybox.loadCubemap(clist);
}

void
VR::ProcessVREvent( const vr::VREvent_t & event )
{
  switch( event.eventType )
    {
    case vr::VREvent_TrackedDeviceActivated:
      {
	setupRenderModelForTrackedDevice( event.trackedDeviceIndex );
	//QMessageBox::information(0, "", QString("Device %1 attached.").arg(event.trackedDeviceIndex));
      }
      break;
    case vr::VREvent_TrackedDeviceDeactivated:
      {
	//QMessageBox::information(0, "", QString("Device %1 detached.").arg(event.trackedDeviceIndex));
      }
      break;
    case vr::VREvent_TrackedDeviceUpdated:
      {
	//QMessageBox::information(0, "", QString("Device %1 updated.").arg(event.trackedDeviceIndex));
      }
      break;
    }
}

void
VR::updatePoses()
{
    vr::VRCompositor()->WaitGetPoses(m_trackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

    for (unsigned int i=0; i<vr::k_unMaxTrackedDeviceCount; i++)
    {
        if (m_trackedDevicePose[i].bPoseIsValid)
        {
            m_matrixDevicePose[i] =  vrMatrixToQt(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
        }
    }

    if (m_trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
    {
        m_hmdPose = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd].inverted();
    }
}

void
VR::updateInput()
{
  vr::VREvent_t event;
  
  //----------------------------------
  // Process SteamVR events
  while(m_hmd->PollNextEvent(&event, sizeof(event)))
    {
      ProcessVREvent( event );
    }
  //----------------------------------

  //----------------------------------
  // Process SteamVR controller state
  for( vr::TrackedDeviceIndex_t unDevice = 0;
       unDevice < vr::k_unMaxTrackedDeviceCount;
       unDevice++ )
    {
      vr::VRControllerState_t state;
      if( m_hmd->GetControllerState( unDevice, &state, sizeof(state) ) )
	{
	  m_rbShowTrackedDevice[ unDevice ] = state.ulButtonPressed == 0;
	}
    }
  //----------------------------------
  

  m_leftController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
  
  m_rightController = m_hmd->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

  bool leftActive = m_hmd->GetControllerState(m_leftController,
					      &m_stateLeft,
					      sizeof(m_stateLeft));

  bool rightActive = m_hmd->GetControllerState(m_rightController,
					       &m_stateRight,
					       sizeof(m_stateRight));

  bool leftTriggerActive = isTriggered(m_stateLeft);
  bool rightTriggerActive = isTriggered(m_stateRight);

  bool leftTouchActive = isTouched(m_stateLeft);
  bool rightTouchActive = isTouched(m_stateRight);

  bool leftTouchPressActive = isTouchPressed(m_stateLeft);
  bool rightTouchPressActive = isTouchPressed(m_stateRight);

  bool leftGripActive = isGripped(m_stateLeft);
  bool rightGripActive = isGripped(m_stateRight);


// -----------------------
// press events
  if (!m_triggerActiveBoth)
    {
      if (leftTriggerActive && !m_triggerActiveLeft)
	leftTriggerPressed();

      if (rightTriggerActive && !m_triggerActiveRight)
	rightTriggerPressed();
    }


  // touch
  if (leftTouchActive && !m_touchActiveLeft)
    leftTouched();

  if (rightTouchActive && !m_touchActiveRight)
    rightTouched();


  // touch press
  if (leftTouchPressActive && !m_touchPressActiveLeft)
    leftTouchPressed();

  if (rightTouchPressActive && !m_touchPressActiveRight)
    rightTouchPressed();


  // grip
  if (leftGripActive && !m_gripActiveLeft)
    leftGripPressed();

  if (rightGripActive && !m_gripActiveRight)
    rightGripPressed();

// -----------------------


// -----------------------
// move events
  if (m_triggerActiveBoth)
    bothTriggerMove();
  else
    {
      if (m_triggerActiveLeft)
	leftTriggerMove();
      if (m_triggerActiveRight)
	rightTriggerMove();
    }


  if (!m_triggerActiveLeft) // check map/menu only if left trigger is not pressed
    {
      if (m_showMap)
	checkTeleport(false);
      
      checkOptions(false);
    }

  //touch
  if (leftTouchActive && m_touchActiveLeft)
    leftTouchMove();

  if (rightTouchActive && m_touchActiveRight)
    rightTouchMove();


  //touch press
  if (leftTouchPressActive && m_touchPressActiveLeft)
    leftTouchPressMove();

  if (rightTouchPressActive && m_touchPressActiveRight)
    rightTouchPressMove();


  // grip
  if (leftGripActive && m_gripActiveLeft)
    leftGripMove();

  if (rightGripActive && m_gripActiveRight)
    rightGripMove();

// -----------------------


// -----------------------
// release events
  if (m_triggerActiveBoth)
    {
      if (!leftTriggerActive && !rightTriggerActive)
	bothTriggerReleased();
    }
  else
    {      
      if (!leftTriggerActive && m_triggerActiveLeft)
	leftTriggerReleased();

      if (!rightTriggerActive && m_triggerActiveRight)
	rightTriggerReleased();

    }


  // touch
  if (!leftTouchActive && m_touchActiveLeft)
    leftTouchReleased();

  if (!rightTouchActive && m_touchActiveRight)
    rightTouchReleased();


  // touch press
  if (!leftTouchPressActive && m_touchPressActiveLeft)
    leftTouchPressReleased();

  if (!rightTouchPressActive && m_touchPressActiveRight)
    rightTouchPressReleased();


  // grip
  if (!leftGripActive && m_gripActiveLeft)
    leftGripReleased();

  if (!rightGripActive && m_gripActiveRight)
    rightGripReleased();

// -----------------------

}
//---------------------------------------


//---------------------------------------
//---------------------------------------
void
VR::leftGripPressed()
{
  m_gripActiveLeft = true;
}
void
VR::leftGripMove()
{
  QVector3D cen = vrHmdPosition();
  m_model_xform.setToIdentity();
  m_model_xform.translate(cen);
  m_model_xform.rotate(0.25, 0, 1, 0); // rotate 1 degree
  m_model_xform.translate(-cen);
  m_final_xform = m_model_xform * m_final_xform;
}
void
VR::leftGripReleased()
{
  m_gripActiveLeft = false;

//  m_final_xform = m_model_xform * m_final_xform;
//  m_model_xform.setToIdentity();
  
  // generate the drawlist each time changes are made
  m_genDrawList = true;
}
//---------------------------------------


//---------------------------------------
void
VR::rightGripPressed()
{
  m_gripActiveRight = true;
}
void
VR::rightGripMove()
{
//  //---------------------------
//  // point cloud distortion to show underlying data
//  if (m_showMap && m_pinPt.x() >= 0)
//    {
//      m_deadRadius = 0.5;
//      m_deadPoint = m_projectedPinPt;
//    }
//  //---------------------------

  QVector3D cen = vrHmdPosition();
  m_model_xform.setToIdentity();
  m_model_xform.translate(cen);
  m_model_xform.rotate(-0.25, 0, 1, 0); // rotate 1 degree
  m_model_xform.translate(-cen);
  m_final_xform = m_model_xform * m_final_xform;
}
void
VR::rightGripReleased()
{
  m_gripActiveRight = false;

//  m_final_xform = m_model_xform * m_final_xform;
//  m_model_xform.setToIdentity();
  
  // generate the drawlist each time changes are made
  m_genDrawList = true;
}
//---------------------------------------
//---------------------------------------


//---------------------------------------
//---------------------------------------
void
VR::leftTriggerPressed()
{
  m_triggerActiveLeft = true;
  if (m_triggerActiveRight)
    {
      m_triggerActiveLeft = false;
      m_triggerActiveRight = false;
      bothTriggerPressed();
      return;
    }
		  
  QVector3D pos =  getPosition(m_trackedDevicePose[m_leftController].mDeviceToAbsoluteTracking);
  m_startTranslate = pos;
}
void
VR::leftTriggerMove()
{
  QVector3D pos =  getPosition(m_trackedDevicePose[m_leftController].mDeviceToAbsoluteTracking);
  pos = (pos - m_startTranslate);
  
  m_model_xform.setToIdentity();
  m_model_xform.translate(pos);
}
void
VR::leftTriggerReleased()
{
  m_triggerActiveLeft = false;

  m_final_xform = m_model_xform * m_final_xform;
  m_model_xform.setToIdentity();
  
  // generate the drawlist each time changes are made
  m_genDrawList = true;
}
//---------------------------------------


//---------------------------------------
void
VR::rightTriggerPressed()
{
  m_triggerActiveRight = true;
  if (m_triggerActiveLeft)
    {
      m_triggerActiveLeft = false;
      m_triggerActiveRight = false;
      bothTriggerPressed();
      return;
    }

  m_startTranslate =  getPosition(m_trackedDevicePose[m_rightController].mDeviceToAbsoluteTracking);
}
void
VR::rightTriggerMove()
{
  QVector3D pos =  getPosition(m_trackedDevicePose[m_rightController].mDeviceToAbsoluteTracking);
  pos = (pos - m_startTranslate);
  
  m_model_xform.setToIdentity();
  m_model_xform.translate(pos);
}
void
VR::rightTriggerReleased()
{
  m_triggerActiveRight = false;

  m_final_xform = m_model_xform * m_final_xform;
  m_model_xform.setToIdentity();

  // generate the drawlist each time changes are made
  m_genDrawList = true;

  //----------------------
  // trigger teleport only if not used to move the model
  QVector3D pos =  getPosition(m_trackedDevicePose[m_rightController].mDeviceToAbsoluteTracking);
  pos = (pos - m_startTranslate);
  if (pos.length() < 0.1)
    {
      if (!m_triggerActiveLeft) // check map/menu only if left trigger is not pressed
	{
	  if (m_showMap)
	    checkTeleport(true);
	  
	  checkOptions(true);
	}
    }
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
//---------------------------------------
void
VR::leftTouched()
{
  m_touchActiveLeft = true;

  m_startTouchX = m_stateLeft.rAxis[0].x;
  m_startTouchY = m_stateLeft.rAxis[0].y;
}
void
VR::leftTouchMove()
{
  m_touchX = m_stateLeft.rAxis[0].x;
  m_touchY = m_stateLeft.rAxis[0].y;
}
void
VR::leftTouchReleased()
{
  m_touchActiveLeft = false;

  if (m_touchX > 0.5)
    nextMenu();
  else if (m_touchX < -0.5)
    previousMenu();
}
//---------------------------------------


//---------------------------------------
void
VR::rightTouched()
{
  m_touchActiveRight = true;

  m_flightActive = true;
  
  m_startTouchX = m_stateRight.rAxis[0].x;
  m_startTouchY = m_stateRight.rAxis[0].y;

  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  m_rcDir = QVector3D(center-point);
  m_rcDir.normalize();
  
  m_flyTimer.start(5000); // generate new draw list every 5 sec
}
void
VR::rightTouchMove()
{
  if (m_touchPressActiveRight)
    {
      rightTouchReleased();
      return;
    }
  
  m_touchX = m_stateRight.rAxis[0].x;
  m_touchY = m_stateRight.rAxis[0].y;

  float acc = (m_touchY-m_startTouchY);

  m_model_xform.setToIdentity();      

  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  QVector3D moveD = QVector3D(center-point);
  moveD.normalize();

  //float throttle = qBound(0.01f, 0.2f, m_speedDamper*m_flightSpeed);

//  // change speed based on height above ground
//  {
//    int wd = screenWidth();
//    int ht = screenHeight();
//    
//    QVector3D hpos = hmdPosition();
//    QVector3D hp = Global::menuCamProjectedCoordinatesOf(hpos);
//    int dx = hp.x();
//    int dy = hp.y();
//    
//    if (dx > 0 && dx < wd-1 &&
//	dy > 0 && dy < ht-1)
//      {
//	float z = m_depthBuffer[(ht-1-dy)*wd + dx];
//	if (z > 0.0 && z < 1.0)
//	  {
//	    float sf = m_teleportScale/m_scaleFactor;
//	    QVector3D hitP = Global::menuCamUnprojectedCoordinatesOf(QVector3D(dx, dy, z));
//	    QVector3D pos = hitP+QVector3D(0,0,m_groundHeight*sf); // raise the height
//	    
//	    // increase speed when away from normal height
//	    float speed = qAbs(pos.z()-hpos.z())/(m_groundHeight*sf);
//	    speed += 1.0;
//	    throttle *= speed;
//	  }
//      }
//  }


  QVector3D move = moveD;
  //move *= throttle;
  move *= acc;

  m_model_xform.translate(-move);
  
//--------------------------------------------
// change scaling based on where viewer is moving
//--------------------------------------------
//  float sf = 1.0-qBound(-0.005, 0.005, qPow(moveD.y(),5));
//  if (m_scaleFactor*sf > m_coordScale &&
//      m_scaleFactor*sf < m_teleportScale)
//    {
//      QVector3D cen;
//      if (m_pinPt.x() >= 0)
//	cen = m_final_xform.map(m_projectedPinPt);
//      else 
//	cen = getPosition(m_rightController);
//      
//      m_model_xform.translate(cen);
//      m_model_xform.scale(sf);
//      m_model_xform.translate(-cen);
//      
//      m_scaleFactor *= sf;
//      m_flightSpeed *= sf;
//    }
//--------------------------------------------

  m_final_xform = m_model_xform * m_final_xform;

  {
    QVector3D v0 = QVector3D(m_rcDir.x(), 0, m_rcDir.z());
    QVector3D v1 = QVector3D(moveD.x(), 0, moveD.z());
    QVector3D axis = QVector3D::normal(v1,v0);
    float angle = StaticFunctions::getAngleBetweenVectors(v1,v0);

    // modulate angle based on tilt above/below horizon
    // parallel with horizon means take the full effect
    // more the tile less the effect
    angle *= (1.0-StaticFunctions::smoothstep(0.0, 0.5, qAbs(moveD.y())));

    QQuaternion q = QQuaternion::fromAxisAndAngle(axis, qRadiansToDegrees(angle*0.005));
    QVector3D cen = vrHmdPosition();
    m_model_xform.setToIdentity();
    m_model_xform.translate(cen);
    m_model_xform.rotate(q);
    m_model_xform.translate(-cen);
    m_final_xform = m_model_xform * m_final_xform;
  }
  
  m_final_xformInverted = m_final_xform.inverted();
      

  //------------------  
  // keep head above ground
  if (m_showMap && m_depthBuffer)
    {
      int wd = screenWidth();
      int ht = screenHeight();

      QVector3D hpos = hmdPosition();
      QVector3D hp = Global::menuCamProjectedCoordinatesOf(hpos);
      int dx = hp.x();
      int dy = hp.y();

      if (dx > 0 && dx < wd-1 &&
	  dy > 0 && dy < ht-1)
	{
	  float z = m_depthBuffer[(ht-1-dy)*wd + dx];
	  if (z > 0.0 && z < 1.0)
	    {
	      float sf = m_teleportScale/m_scaleFactor;
	      QVector3D hitP = Global::menuCamUnprojectedCoordinatesOf(QVector3D(dx, dy, z));
	      QVector3D pos = hitP+QVector3D(0,0,m_groundHeight*sf); // raise the height

	      if (m_gravity && // stick close to ground
		  pos.z() > hpos.z()) // push it above the ground
		{
		  float mup = (m_final_xform.map(pos)-m_final_xform.map(hpos)).y();

		  // move only vertically
		  if (pos.z() > hpos.z())
		    mup *= 0.1; // move quickly above ground
		  else
		    mup*=0.05; // come down slowly

		  QVector3D move(0,mup,0);
		  m_model_xform.setToIdentity();
		  m_model_xform.translate(-move);
		  m_final_xform = m_model_xform * m_final_xform;
		}
	    }
	}
    }
  //------------------  


  genEyeMatrices();

  
  // generate the drawlist each time changes are made
  if (!m_flyTimer.isActive())
    {
      m_genDrawList = true;
      m_flyTimer.start(5000); // generate new draw list every 5 sec
    }
}
void
VR::rightTouchReleased()
{
  m_touchActiveRight = false;

  m_flightActive = false;

  m_model_xform.setToIdentity();
  
  m_genDrawList = true;
  m_flyTimer.stop();
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
//---------------------------------------
void
VR::leftTouchPressed()
{
  m_touchPressActiveLeft = true;

  m_startTouchX = m_stateLeft.rAxis[0].x;
  m_startTouchY = m_stateLeft.rAxis[0].y;

  // save teleport when both touchpads pressed
  if (m_touchPressActiveRight)
    saveTeleportNode();
}
void
VR::leftTouchPressMove()
{
  m_touchX = m_stateLeft.rAxis[0].x;
  m_touchY = m_stateLeft.rAxis[0].y;
}
void
VR::leftTouchPressReleased()
{
  m_touchPressActiveLeft = false;

  if (m_touchX > 0.5)
    nextMenu();
  else if (m_touchX < -0.5)
    previousMenu();
}
//---------------------------------------


//---------------------------------------
void
VR::rightTouchPressed()
{
  m_touchPressActiveRight = true;

//  m_flightActive = true;
  
  m_touchX = m_stateRight.rAxis[0].x;
  m_touchY = m_stateRight.rAxis[0].y;
  
//  m_flyTimer.start(5000); // generate new draw list every 5 sec

  // save teleport when both touchpads pressed
  if (m_touchPressActiveLeft)
    saveTeleportNode();
}
void
VR::rightTouchPressMove()
{
//  //---------------------------
//  // point cloud distortion to show underlying data
//  if (m_showMap && m_pinPt.x() >= 0)
//    {
//      m_deadRadius = 0.5;
//      m_deadPoint = m_projectedPinPt;
//    }
//  //---------------------------

//  m_model_xform.setToIdentity(); 
//
//  QMatrix4x4 mat = m_matrixDevicePose[m_rightController];    
//  QVector4D center = mat * QVector4D(0,0,0,1);
//  QVector4D point = mat * QVector4D(0,0,1,1);
//  QVector3D moveD = QVector3D(center-point);
//
//  moveD.normalize();
//  float throttle = qBound(0.01f, 0.1f, m_flightSpeed*m_speedDamper);
//  QVector3D move = moveD*throttle;
//  
//  if (m_touchY < 0) // move backward
//    move = -move;
//  
//  m_model_xform.translate(-move);
//
//  bool changeScale = false;
//  float sf = 1.0;
//  if (moveD.y() > 0.8) // moving up
//    {
//      // scale down while going up in the sky
//      sf = 0.99;
//      changeScale = true;
//    }
//  else if (moveD.y() < -0.4) // moving down
//    {
//      // scale up while going down to ground
//      // but only if below threshold 
//      sf = m_teleportScale/m_scaleFactor;
//      if (sf > 1.0)
//	{
//	  sf = qPow(sf, 0.01f);
//	  changeScale = true;
//	}
//    }
//
//  if (changeScale)
//    {
//      QVector3D cen;
//      if (m_pinPt.x() >= 0)
//	cen = m_final_xform.map(m_projectedPinPt);
//      else 
//	cen = getPosition(m_rightController);
//      
//      m_model_xform.translate(cen);
//      m_model_xform.scale(sf);
//      m_model_xform.translate(-cen);
//      
//      m_scaleFactor *= sf;
//      m_flightSpeed *= sf;
//    }
//
//  
//  m_final_xform = m_model_xform * m_final_xform;
//
//  m_final_xformInverted = m_final_xform.inverted();
//      
//
//  //------------------  
//  // keep head above ground
//  if (m_showMap && m_depthBuffer)
//    {
//      int wd = screenWidth();
//      int ht = screenHeight();
//
//      QVector3D hpos = hmdPosition();
//      QVector3D hp = Global::menuCamProjectedCoordinatesOf(hpos);
//      int dx = hp.x();
//      int dy = hp.y();
//
//      if (dx > 0 && dx < wd-1 &&
//	  dy > 0 && dy < ht-1)
//	{
//	  float z = m_depthBuffer[(ht-1-dy)*wd + dx];
//	  if (z > 0.0 && z < 1.0)
//	    {
//	      float sf = m_teleportScale/m_scaleFactor;
//	      QVector3D hitP = Global::menuCamUnprojectedCoordinatesOf(QVector3D(dx, dy, z));
//	      QVector3D pos = hitP+QVector3D(0,0,m_groundHeight*sf); // raise the height
//
//	      if (m_gravity || // stick close to ground
//		  pos.z() > hpos.z()) // push it above the ground
//		{
//		  float mup = (m_final_xform.map(pos)-m_final_xform.map(hpos)).y();
//
//		  // move only vertically
//		  if (pos.z() > hpos.z())
//		    mup *= 0.1; // move quickly above ground
//		  else
//		    mup*=0.05; // come down slowly
//
//		  QVector3D move(0,mup,0);
//		  m_model_xform.setToIdentity();
//		  m_model_xform.translate(-move);
//		  m_final_xform = m_model_xform * m_final_xform;
//		}
//	    }
//	}
//    }
//  //------------------  
//
//
//  genEyeMatrices();
//
//  
//  // generate the drawlist each time changes are made
//  if (!m_flyTimer.isActive())
//    {
//      m_genDrawList = true;
//      m_flyTimer.start(5000); // generate new draw list every 5 sec
//    }
}
void
VR::rightTouchPressReleased()
{
  m_touchPressActiveRight = false;

  m_flightActive = false;

  m_model_xform.setToIdentity();
  
  m_genDrawList = true;
  m_flyTimer.stop();
}
//---------------------------------------
//---------------------------------------


//---------------------------------------
//---------------------------------------
void
VR::bothTriggerPressed()
{
  m_triggerActiveBoth = true;

  QVector3D posL =  getPosition(m_trackedDevicePose[m_leftController].mDeviceToAbsoluteTracking);
  QVector3D posR =  getPosition(m_trackedDevicePose[m_rightController].mDeviceToAbsoluteTracking);

  m_lrDist = posL.distanceToPoint(posR);

  m_prevPosL = posL;
  m_prevPosR = posR;
  m_prevDirection = (m_prevPosL - m_prevPosR).normalized();

  QVector3D cenL = getPosition(m_leftController);
  QVector3D cenR = getPosition(m_rightController);
  m_scaleCenter = (cenL + cenR)*0.5;
  
  m_prevScaleFactor = m_scaleFactor;
  m_prevFlightSpeed = m_flightSpeed;
}

void
VR::bothTriggerMove()
{
  QVector3D posL =  getPosition(m_trackedDevicePose[m_leftController].mDeviceToAbsoluteTracking);
  QVector3D posR =  getPosition(m_trackedDevicePose[m_rightController].mDeviceToAbsoluteTracking);

  m_model_xform.setToIdentity();

  float newDist = posL.distanceToPoint(posR);

  float sf = newDist/m_lrDist;

  //--------------------------
  // translation
  if (sf > 0.9)
    {
      QVector3D pos = (posL - m_prevPosL);	  
      m_model_xform.translate(pos);
    }
  //--------------------------

  //--------------------------
  // rotation
  QVector3D cdirec = (posL - posR).normalized();      
  float angle = qAcos(QVector3D::dotProduct(m_prevDirection, cdirec));
  QVector3D axis = QVector3D::crossProduct(m_prevDirection, cdirec);
  m_model_xform.translate(m_scaleCenter);
  m_model_xform.rotate(qRadiansToDegrees(angle), axis.x(), axis.y(), axis.z());
  m_model_xform.translate(-m_scaleCenter);
  //--------------------------


  //--------------------------
  // scale
  m_model_xform.translate(m_scaleCenter);
  m_model_xform.scale(sf);
  m_model_xform.translate(-m_scaleCenter);
  //--------------------------
  
  m_scaleFactor = m_prevScaleFactor * sf;
  m_flightSpeed = m_prevFlightSpeed * sf;
  
  genEyeMatrices();
}

void
VR::bothTriggerReleased()
{
  m_triggerActiveBoth = false;

  m_final_xform = m_model_xform * m_final_xform;
  m_model_xform.setToIdentity();

  // generate the drawlist each time changes are made
  m_genDrawList = true;
}
//---------------------------------------
//---------------------------------------



//---------------------------------------
bool
VR::isTriggered(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
}

bool
VR::isGripped(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_Grip));
}

bool
VR::isTouched(vr::VRControllerState_t &state)
{
  return (state.ulButtonTouched &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
}

bool
VR::isTouchPressed(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
}



QMatrix4x4
VR::currentViewProjection(vr::Hmd_Eye eye)
{
    if (eye == vr::Eye_Left)
      return m_leftProjection * m_leftPose * m_hmdPose;
    else
      return m_rightProjection * m_rightPose * m_hmdPose;
}

QMatrix4x4
VR::viewProjection(vr::Hmd_Eye eye)
{
  QMatrix4x4 total_xform = (m_model_xform*m_final_xform);

  m_final_xformInverted = total_xform.inverted();

  return currentViewProjection(eye)*total_xform;
}

QMatrix4x4
VR::modelViewNoHmd()
{
  QMatrix4x4 total_xform = (m_model_xform*m_final_xform);

  return total_xform;
}

QMatrix4x4
VR::modelView()
{
  QMatrix4x4 total_xform = (m_model_xform*m_final_xform);

  QMatrix4x4 model;
  model = m_hmdPose;

  return model*total_xform;
}

QMatrix4x4
VR::modelView(vr::Hmd_Eye eye)
{
  QMatrix4x4 total_xform = (m_model_xform*m_final_xform);

  QMatrix4x4 model;
  if (eye == vr::Eye_Left)
     model = m_leftPose * m_hmdPose;
  else
    model = m_rightPose * m_hmdPose;

  return model*total_xform;
}



QMatrix4x4
VR::vrMatrixToQt(const vr::HmdMatrix34_t &mat)
{
  return QMatrix4x4(
		    mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		    mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		    mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		    0.0,         0.0,         0.0,         1.0f
		    );
}

QMatrix4x4
VR::vrMatrixToQt(const vr::HmdMatrix44_t &mat)
{
  return QMatrix4x4(
		    mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
		    mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
		    mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
		    mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3]
		    );
}

QString
VR::getTrackedDeviceString(vr::TrackedDeviceIndex_t device,
			   vr::TrackedDeviceProperty prop,
			   vr::TrackedPropertyError *error)
{
    uint32_t len = m_hmd->GetStringTrackedDeviceProperty(device, prop, NULL, 0, error);
    if(len == 0)
        return "";

    char *buf = new char[len];
    m_hmd->GetStringTrackedDeviceProperty(device, prop, buf, len, error);

    QString result = QString::fromLocal8Bit(buf);
    delete [] buf;

    return result;
}

QMatrix4x4 VR::matrixDevicePoseLeft() { return m_matrixDevicePose[m_leftController]; }
QMatrix4x4 VR::matrixDevicePoseRight() { return m_matrixDevicePose[m_rightController]; }

QVector3D
VR::getModelSpacePosition(vr::TrackedDeviceIndex_t trackedDevice)
{
  QMatrix4x4 mat = m_matrixDevicePose[trackedDevice];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D cpos = m_final_xformInverted.map(center);
  QVector3D pos(cpos.x(),cpos.y(),cpos.z());
  return pos;
}

QMatrix4x4
VR::hmdMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}
QMatrix4x4
VR::leftMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_leftController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}
QMatrix4x4
VR::rightMVP(vr::Hmd_Eye eye)
{
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_rightController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  return mvp;
}

QVector3D
VR::getPosition(vr::TrackedDeviceIndex_t trackedDevice)
{
  QMatrix4x4 mat = m_matrixDevicePose[trackedDevice];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector3D pos(center.x(),center.y(),center.z());
  return pos;		
}

QVector3D
VR::getPosition(const vr::HmdMatrix34_t mat)
{
  return QVector3D(mat.m[0][3],
		   mat.m[1][3],
		   mat.m[2][3]);
		
}

QVector3D
VR::getPosition(QMatrix4x4 mat)
{
  return QVector3D(mat(0,3), mat(1,3), mat(2,3));
		
}


float
VR::copysign(float x, float y)
{
  return y == 0.0 ? qAbs(x) : qAbs(x)*y/qAbs(y);
}

// Get the quaternion representing the rotation
QQuaternion
VR::getQuaternion(QMatrix4x4 mat)
{
  float w = sqrt(qMax(0.0f, 1 + mat(0,0) + mat(1,1) + mat(2,2))) / 2;
  float x = sqrt(qMax(0.0f, 1 + mat(0,0) - mat(1,1) - mat(2,2))) / 2;
  float y = sqrt(qMax(0.0f, 1 - mat(0,0) + mat(1,1) - mat(2,2))) / 2;
  float z = sqrt(qMax(0.0f, 1 - mat(0,0) - mat(1,1) + mat(2,2))) / 2;

  x = copysign(x, (mat(2,1) - mat(1,2)));
  y = copysign(y, (mat(0,2) - mat(2,0)));
  z = copysign(z, (mat(1,0) - mat(0,1)));

  QQuaternion q(x,y,z,w);
  
  return q;
}

// Get the quaternion representing the rotation
QQuaternion
VR::getQuaternion(vr::HmdMatrix34_t mat)
{
  float w = sqrt(qMax(0.0f, 1 + mat.m[0][0] + mat.m[1][1] + mat.m[2][2])) / 2;
  float x = sqrt(qMax(0.0f, 1 + mat.m[0][0] - mat.m[1][1] - mat.m[2][2])) / 2;
  float y = sqrt(qMax(0.0f, 1 - mat.m[0][0] + mat.m[1][1] - mat.m[2][2])) / 2;
  float z = sqrt(qMax(0.0f, 1 - mat.m[0][0] - mat.m[1][1] + mat.m[2][2])) / 2;

  x = copysign(x, (mat.m[2][1] - mat.m[1][2]));
  y = copysign(y, (mat.m[0][2] - mat.m[2][0]));
  z = copysign(z, (mat.m[1][0] - mat.m[0][1]));

  QQuaternion q(x,y,z,w);
  
  return q;
}

void
VR::preDraw()
{
  m_deadRadius = -1;

  updatePoses();
  updateInput();
  buildAxes();

  if (m_showMap)
    {
      projectPinPoint();

      if (!buildTeleport())
	buildPinPoint();
    }
  
}

void
VR::bindBuffer(vr::Hmd_Eye eye)
{
  if (eye == vr::Eye_Left)
    bindLeftBuffer();
  else
    bindRightBuffer();
}

void
VR::bindMapBuffer()
{
  glDisable(GL_MULTISAMPLE);
  m_mapBuffer->bind();
}

void
VR::bindLeftBuffer()
{
  glEnable(GL_MULTISAMPLE);
  m_leftBuffer->bind();
}

void
VR::bindRightBuffer()
{
  glEnable(GL_MULTISAMPLE);
  m_rightBuffer->bind();
}

void
VR::postDrawLeftBuffer()
{
  renderSkyBox(vr::Eye_Left);
  
  renderControllers(vr::Eye_Left);

  renderAxes(vr::Eye_Left);

  renderMenu(vr::Eye_Left);

  if (m_showMap)
    renderTeleport(vr::Eye_Left);

  m_leftBuffer->release();

  QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);
  QRect targetLeft(0, 0, m_eyeWidth, m_eyeHeight);
  QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetLeft,
					    m_leftBuffer, sourceRect);
}

void
VR::postDrawRightBuffer()
{
  renderSkyBox(vr::Eye_Right);
  
  renderControllers(vr::Eye_Right);

  renderAxes(vr::Eye_Right);

  renderMenu(vr::Eye_Right);

  if (m_showMap)
    renderTeleport(vr::Eye_Right);
  
  m_rightBuffer->release();

  QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);
  QRect targetRight(m_eyeWidth, 0, m_eyeWidth, m_eyeHeight);
  QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetRight,
					    m_rightBuffer, sourceRect);
}

void
VR::postDraw()
{
  if (m_hmd)
    {
      vr::VRTextureBounds_t leftRect = { 0.0f, 0.0f, 0.5f, 1.0f };
      vr::VRTextureBounds_t rightRect = { 0.5f, 0.0f, 1.0f, 1.0f };
      vr::Texture_t composite = { (void*)m_resolveBuffer->texture(),
				  vr::TextureType_OpenGL,
				  vr::ColorSpace_Gamma };
      
      vr::VRCompositor()->Submit(vr::Eye_Left, &composite, &leftRect);
      vr::VRCompositor()->Submit(vr::Eye_Right, &composite, &rightRect);
    }
}


QVector3D
VR::vrViewDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}
QVector3D
VR::vrUpDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,1,0,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}
QVector3D
VR::vrRightDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(1,0,0,1);
  QVector4D dir = point-center;
  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();
  return dir3;
}

QVector3D
VR::viewDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

QVector3D
VR::upDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,1,0,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

QVector3D
VR::rightDir()
{
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(1,0,0,1);

  center = m_final_xformInverted.map(center);
  point = m_final_xformInverted.map(point);

  QVector4D dir = point-center;

  QVector3D dir3 = QVector3D(dir.x(), dir.y(), dir.z());
  dir3.normalize();

  return dir3;
}

void
VR::buildAxes()
{
  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];

  QVector<float> vert;
  QVector<uchar> col;
  int npt = 0;

  QVector3D colorL(0,255,255);
  QVector3D colorR(255,255,0);
  QVector4D point(0,0,-0.05,1);


  QVector4D centerR = matR * QVector4D(0,0,0,1);
  QVector4D frontR = matR * QVector4D(0,0,-0.1,1) - centerR; 
  vert << centerR.x();
  vert << centerR.y();
  vert << centerR.z();      
  vert << centerR.x() + frontR.x();
  vert << centerR.y() + frontR.y();
  vert << centerR.z() + frontR.z();      

  col << colorR.x();
  col << colorR.y();
  col << colorR.z(); 
  col << colorR.x();
  col << colorR.y();
  col << colorR.z();

  QVector4D centerL = matL * QVector4D(0,0,0,1);
  QVector4D frontL = matL * QVector4D(0,0,-0.1,1) - centerL; 
  vert << centerL.x();
  vert << centerL.y();
  vert << centerL.z();      
  vert << centerL.x() + frontL.x();
  vert << centerL.y() + frontL.y();
  vert << centerL.z() + frontL.z();      
      
  col << colorL.x();
  col << colorL.y();
  col << colorL.z();      
  col << colorL.x();
  col << colorL.y();
  col << colorL.z();
  
  npt = 4;


  uchar vc[1000];  
  for(int v=0; v<vert.count()/3; v++)
    {
      float *vt = (float*)(vc + 15*v);
      vt[0] = vert[3*v+0];
      vt[1] = vert[3*v+1];
      vt[2] = vert[3*v+2];
      
      uchar *cl = (uchar*)(vc + 15*v + 12);
      cl[0] = col[3*v+0];
      cl[1] = col[3*v+1];
      cl[2] = col[3*v+2];
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  0,
		  vert.count()*15,
		  &vc[0]);

  m_axesPoints = npt;
  m_nboxPoints = npt;

  m_pinPoints = 0;
  m_telPoints = 0;
}

QMatrix4x4
VR::quatToMat(QQuaternion q)
{
  //based on algorithm on wikipedia
  // http://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
  float w = q.scalar ();
  float x = q.x();
  float y = q.y();
  float z = q.z();
  
  float n = q.lengthSquared();
  float s =  n == 0?  0 : 2 / n;
  float wx = s * w * x, wy = s * w * y, wz = s * w * z;
  float xx = s * x * x, xy = s * x * y, xz = s * x * z;
  float yy = s * y * y, yz = s * y * z, zz = s * z * z;
  
  float m[16] = { 1 - (yy + zz),         xy + wz ,         xz - wy ,0,
		  xy - wz ,    1 - (xx + zz),         yz + wx ,0,
		  xz + wy ,         yz - wx ,    1 - (xx + yy),0,
		  0 ,               0 ,               0 ,1  };
  QMatrix4x4 result =  QMatrix4x4(m,4,4);
  result.optimize ();
  return result;
}

void
VR::renderAxes(vr::Hmd_Eye eye)
{  
  if (m_pinPoints > 0)
    return;
  
  QMatrix4x4 mvp = currentViewProjection(eye);

  glUseProgram(m_pshader);
  glUniformMatrix4fv(m_pshaderParm[0], 1, GL_FALSE, mvp.data());
  glUniform1f(m_pshaderParm[1], 20);
  // send dummy positions
  glUniform3f(m_pshaderParm[2], -1, -1, -1); // head position
  glUniform3f(m_pshaderParm[3], -1, -1, -1); // left controller
  glUniform3f(m_pshaderParm[4], -1, -1, -1); // right controller
  glUniform1i(m_pshaderParm[5], 1); // lines


  glLineWidth(2.0);

  glBindVertexArray(m_boxVID);
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			15, // stride
			(void*)0 ); // array buffer offset
  
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			15, // stride
			(char *)NULL+12 ); // array buffer offset
  
//  if (!m_showMenu && !m_showMap)
//    glDrawArrays(GL_LINES, 0, m_axesPoints);
//  else // don't show left axes as we have map/menu on that
    glDrawArrays(GL_LINES, 0, m_axesPoints/2);

    
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  

  glBindVertexArray(0);

  glUseProgram(0);
}

void
VR::createShaders()
{
  //------------------------
  m_pshader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_pshader,
					  "assets/shaders/punlit.vert",
					  "assets/shaders/punlit.frag"))
    {
      QMessageBox::information(0, "", "Cannot load shaders");
    }

  m_pshaderParm[0] = glGetUniformLocation(m_pshader, "MVP");
  m_pshaderParm[1] = glGetUniformLocation(m_pshader, "pointSize");
  m_pshaderParm[2] = glGetUniformLocation(m_pshader, "hmdPos");
  m_pshaderParm[3] = glGetUniformLocation(m_pshader, "leftController");
  m_pshaderParm[4] = glGetUniformLocation(m_pshader, "rightController");
  m_pshaderParm[5] = glGetUniformLocation(m_pshader, "gltype");
  //------------------------


  ShaderFactory::createTextureShader();

  ShaderFactory::createCubeMapShader();
}


void
VR::buildAxesVB()
{
  glGenVertexArrays(1, &m_boxVID);
  glBindVertexArray(m_boxVID);

  glGenBuffers(1, &m_boxV);

  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferData(GL_ARRAY_BUFFER,
	       1000*sizeof(float),
	       NULL,
	       GL_STATIC_DRAW);


  //------------------------
  uchar indexData[6];
  indexData[0] = 0;
  indexData[1] = 1;
  indexData[2] = 2;
  indexData[3] = 0;
  indexData[4] = 2;
  indexData[5] = 3;
  
  glGenBuffers( 1, &m_boxIB );
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_boxIB );
  glBufferData( GL_ELEMENT_ARRAY_BUFFER,
		sizeof(uchar) * 6,
		&indexData[0],
		GL_STATIC_DRAW );
  //------------------------
}

CGLRenderModel*
VR::findOrLoadRenderModel(QString pchRenderModelName)
{
  CGLRenderModel *pRenderModel = NULL;
  for(int i=0; i<m_vecRenderModels.count(); i++)
    {
      if(m_vecRenderModels[i]->GetName() == pchRenderModelName)
	{
	  pRenderModel = m_vecRenderModels[i];
	  break;
	}
    }

  // load the model if we didn't find one
  if( !pRenderModel )
    {
      vr::RenderModel_t *pModel;
      vr::EVRRenderModelError error;
      while ( 1 )
	{
	  error = vr::VRRenderModels()->LoadRenderModel_Async( pchRenderModelName.toLatin1(),
							       &pModel );
	  if ( error != vr::VRRenderModelError_Loading )
	    break;
	  
	  Sleep( 1 );
	}
      
      if ( error != vr::VRRenderModelError_None )
	{
	  QMessageBox::information(0, "", QString("Unable to load render model %1 - %2").\
				   arg(pchRenderModelName).\
				   arg(vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error)));
	  return NULL; // move on to the next tracked device
	}
      
      vr::RenderModel_TextureMap_t *pTexture;
      while ( 1 )
	{
	  error = vr::VRRenderModels()->LoadTexture_Async( pModel->diffuseTextureId, &pTexture );
	  if ( error != vr::VRRenderModelError_Loading )
	    break;
	  
	  Sleep( 1 );
	}
      
      if ( error != vr::VRRenderModelError_None )
	{
	  QMessageBox::information(0, "", QString("Unable to load render texture id:%1 for render model %2"). \
				   arg(pModel->diffuseTextureId).arg(pchRenderModelName));
	  vr::VRRenderModels()->FreeRenderModel( pModel );
	  return NULL; // move on to the next tracked device
	}
      
      pRenderModel = new CGLRenderModel( pchRenderModelName );
      if ( !pRenderModel->BInit( *pModel, *pTexture ) )
	{
	  QMessageBox::information(0, "", "Unable to create GL model from render model " + pchRenderModelName);
	  delete pRenderModel;
	  pRenderModel = NULL;
	}
      else
	{
	  m_vecRenderModels << pRenderModel;
	}
      vr::VRRenderModels()->FreeRenderModel( pModel );
      vr::VRRenderModels()->FreeTexture( pTexture );
    }
  return pRenderModel;
}


void
VR::setupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
  if(unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
    return;

  // try to find a model we've already set up
  QString sRenderModelName = getTrackedDeviceString(unTrackedDeviceIndex,
						    vr::Prop_RenderModelName_String);

  CGLRenderModel *pRenderModel = findOrLoadRenderModel( sRenderModelName);
  if( !pRenderModel )
    {
      QString sTrackingSystemName = getTrackedDeviceString(unTrackedDeviceIndex,
							   vr::Prop_TrackingSystemName_String);
      QMessageBox::information(0, "", QString("Unable to load render model for tracked device %1 (%2.%3)").\
			       arg(unTrackedDeviceIndex).arg(sTrackingSystemName).arg(sRenderModelName));
    }
  else
    {
      m_rTrackedDeviceToRenderModel[ unTrackedDeviceIndex ] = pRenderModel;
      m_rbShowTrackedDevice[ unTrackedDeviceIndex ] = true;
    }
}


void
VR::setupRenderModels()
{
  memset( m_rTrackedDeviceToRenderModel, 0, sizeof( m_rTrackedDeviceToRenderModel ) );

  if( !m_hmd )
    return;

  for( uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++ )
    {
      if( !m_hmd->IsTrackedDeviceConnected( unTrackedDevice ) )
	continue;
      
      setupRenderModelForTrackedDevice( unTrackedDevice );
    }  
}

void
VR::renderControllers(vr::Hmd_Eye eye)
{
  if (!m_hmd)
    return;
  
  bool bIsInputCapturedByAnotherProcess = m_hmd->IsInputFocusCapturedByAnotherProcess();
  if(bIsInputCapturedByAnotherProcess)
    return;
  
  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];    
  QVector4D center = mat * QVector4D(0,0,0,1);
  QVector4D point = mat * QVector4D(0,0,1,1);
  QVector3D vd = QVector3D(center-point);
  vd.normalize();
  
//-------------------------------------------------
//-------------------------------------------------
  glUseProgram(ShaderFactory::rcShader());

  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[m_leftController];
  QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;  
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 0); // texture
  glUniform3f(rcShaderParm[2], 0.0,0.8,1); // color
  glUniform3f(rcShaderParm[3], vd.x(), vd.y(), vd.z()); // view direction
  glUniform1f(rcShaderParm[4], 1); // opacity modulator
  glUniform1i(rcShaderParm[5], 2); // applytexture
  glUniform1f(rcShaderParm[7], 0.5); // mixcolor
  if( m_rTrackedDeviceToRenderModel[m_leftController])
    m_rTrackedDeviceToRenderModel[m_leftController]->Draw();


  matDeviceToTracking = m_matrixDevicePose[m_rightController];
  mvp = currentViewProjection(eye) * matDeviceToTracking;
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform3f(rcShaderParm[2], 1,0.8,0.0); // color
  if(m_rTrackedDeviceToRenderModel[m_rightController])
    m_rTrackedDeviceToRenderModel[ m_rightController ]->Draw();


  glUseProgram( 0 );
//-------------------------------------------------
//-------------------------------------------------

////  bool bIsInputCapturedByAnotherProcess = m_hmd->IsInputFocusCapturedByAnotherProcess();
////
//  glUseProgram( m_rcShader );
//  
//  for( uint32_t i = 0; i < vr::k_unMaxTrackedDeviceCount; i++ )
//    {
//      if( !m_rTrackedDeviceToRenderModel[i])
//	continue;
//     
//      const vr::TrackedDevicePose_t & pose = m_trackedDevicePose[i];
//      if( !pose.bPoseIsValid )
//	continue;
//      
//      if(m_hmd->GetTrackedDeviceClass(i) == vr::TrackedDeviceClass_Controller )
//	continue;
//      
//      QMatrix4x4 matDeviceToTracking = m_matrixDevicePose[i];
//      QMatrix4x4 mvp = currentViewProjection(eye) * matDeviceToTracking;
//
//      glUniformMatrix4fv(m_rcShaderParm[0], 1, GL_FALSE, mvp.data() );
//      glUniform3f(m_rcShaderParm[1], 0.5,0.5,0.5); // mix color
//      glUniform3f(m_rcShaderParm[2], vd.x(), vd.y(), vd.z()); // view direction
//
//      m_rTrackedDeviceToRenderModel[i]->Draw();
//    }
//  
//  glUseProgram( 0 );
}

void
VR::genEyeMatrices()
{
  m_rightProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Right,
							      NEAR_CLIP,
							      m_flightSpeed*FAR_CLIP));
  m_rightPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Right)).inverted();
  

  m_leftProjection = vrMatrixToQt(m_hmd->GetProjectionMatrix(vr::Eye_Left,
							     NEAR_CLIP,
							     m_flightSpeed*FAR_CLIP));
  m_leftPose = vrMatrixToQt(m_hmd->GetEyeToHeadTransform(vr::Eye_Left)).inverted();
}

void
VR::teleport()
{
  QDir jsondir(m_dataDir);
  QString jsonfile = jsondir.absoluteFilePath("teleport.json");

  QJsonArray jsonTeleportData;

  //------------------------------

  if (jsondir.exists("teleport.json"))
    {
      QFile loadFile(jsonfile);
      loadFile.open(QIODevice::ReadOnly);
      
      QByteArray data = loadFile.readAll();
      
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      
      jsonTeleportData = jsonDoc.array();
    }

  if (jsonTeleportData.count() > 0)
    {
      if (m_currTeleportNumber >= jsonTeleportData.count())
	m_currTeleportNumber = 0;
      
      QJsonObject jsonTeleportNode = jsonTeleportData[m_currTeleportNumber].toObject();
      QJsonObject jsonInfo = jsonTeleportNode["teleport"].toObject();

      {
	QString str = jsonInfo["matrix"].toString();
	
	QStringList matstr = str.split(" ", QString::SkipEmptyParts);
	float mat[16];
	for(int i=0; i<16; i++)
	  mat[i] = matstr[i].toFloat();

	m_final_xform = QMatrix4x4(mat);
	
	m_model_xform.setToIdentity();
      }

      {
	QString str = jsonInfo["scale"].toString();
	
	QStringList sclstr = str.split(" ", QString::SkipEmptyParts);
	m_scaleFactor = sclstr[0].toFloat();
	m_flightSpeed = sclstr[1].toFloat();
      }

      m_genDrawList = true;

      genEyeMatrices();
    }
}

void
VR::teleport(QVector3D newPos)
{
  QVector3D hpos = m_final_xform.map(hmdPosition());

  // calculate scaling
  float sf = 1.0;
  //if (m_scaleFactor < m_teleportScale)
  sf = m_teleportScale/m_scaleFactor;
  

  QVector3D pos = newPos+QVector3D(0,0,m_groundHeight); // raise the height
  pos = m_final_xform.map(pos);

  QVector3D move = pos-hpos;

  m_model_xform.setToIdentity();
  m_model_xform.translate(-move);

  sf = m_teleportScale/m_scaleFactor;
//  if (sf > 1.0)
    {
      m_model_xform.translate(pos);
      m_model_xform.scale(sf);
      m_model_xform.translate(-pos);
 
      m_scaleFactor *= sf;
      m_flightSpeed *= sf;
    }
  

  m_final_xform = m_model_xform * m_final_xform;

  m_model_xform.setToIdentity();

  m_genDrawList = true;

  genEyeMatrices();
}

void
VR::updateScale(int scl)
{
  float sf = 1.5;
  if (scl < 0)
    sf = 0.67;
  

  // pivot point for scaling is the leftcontroller position
  QVector3D cen = getPosition(m_leftController);

  if (m_showMap)
    {
      QVector3D hpos = hmdPosition();
      QVector3D hp = Global::menuCamProjectedCoordinatesOf(hpos);    
      int dx = hp.x();
      int dy = hp.y();
      int wd = screenWidth();
      int ht = screenHeight();
      if (dx > 0 && dx < wd-1 &&
	  dy > 0 && dy < ht-1)
	{
	  float z = m_depthBuffer[(ht-1-dy)*wd + dx];
	  if (z > 0.0 && z < 1.0)
	    {
	      // pivot point for scaling is the ground
	      cen = Global::menuCamUnprojectedCoordinatesOf(QVector3D(dx, dy, z));
	      cen = m_final_xform.map(cen);
	    }
	}
    }

  m_model_xform.translate(cen);
  m_model_xform.scale(sf);
  m_model_xform.translate(-cen);

  m_scaleFactor *= sf;
  m_flightSpeed *= sf;

  m_final_xform = m_model_xform * m_final_xform;
  m_model_xform.setToIdentity();

  genEyeMatrices();
}

void
VR::setShowMap(bool sm)
{
  m_showMap = sm;
  m_leftMenu.setShowMap(sm);

  if (!m_showMap)
    m_menuPanels.removeOne("00");
  
  m_currPanel = 0;
  m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::setShowTimeseriesMenu(bool sm)
{  
  m_currPanel = 0;
  m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);

  m_leftMenu.setPlayMenu(sm);
}

void
VR::nextMenu()
{
  m_currPanel = (m_currPanel+1)%m_menuPanels.count();
  m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::previousMenu()
{
  m_currPanel = (m_menuPanels.count() + m_currPanel-1)%m_menuPanels.count();
  m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
}

void
VR::renderMenu(vr::Hmd_Eye eye)
{
  // do not draw menu if both or left trigger is pressed
  if (m_triggerActiveBoth || m_triggerActiveLeft)
    return;

  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];

  QMatrix4x4 mvp = currentViewProjection(eye);

  m_leftMenu.draw(mvp, matL, m_triggerActiveRight);
}

void
VR::menuImageFromMapBuffer()
{
  m_mapBuffer->release();


  if (m_depthBuffer)
    delete [] m_depthBuffer;
 
  int wd = screenWidth();
  int ht = screenHeight();
  m_depthBuffer = new float[wd*ht];

  glBindFramebuffer(GL_READ_FRAMEBUFFER, m_mapBuffer->handle());
  glReadBuffer(GL_COLOR_ATTACHMENT0);
  glReadPixels(0, 0,
	       wd, ht,
	       GL_DEPTH_COMPONENT,
	       GL_FLOAT,
	       m_depthBuffer);

  QImage img = m_mapBuffer->toImage(true);
  m_leftMenu.setImage(img);
	
  m_mapBuffer->release();

  //-----------------------------------
  // also update teleports if any
  sendTeleportsToMenu();
  //-----------------------------------

  Global::setDepthBuffer(m_depthBuffer);
}

void
VR::setCurrPosOnMenuImage(Vec cp, Vec cd)
{
  QVector3D vcp = QVector3D(cp.x, cp.y, cp.z);
  QVector3D vcd = QVector3D(cd.x, cd.y, cd.z);
  m_leftMenu.setCurrPos(vcp, vcd);
}

void
VR::loadTeleports()
{
  m_teleports.clear();

  QDir jsondir(m_dataDir);
  QString jsonfile = jsondir.absoluteFilePath("teleport.json");

  QJsonArray jsonTeleportData;

  //------------------------------

  if (jsondir.exists("teleport.json"))
    {
      QFile loadFile(jsonfile);
      loadFile.open(QIODevice::ReadOnly);
      
      QByteArray data = loadFile.readAll();
      
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      
      jsonTeleportData = jsonDoc.array();

      loadFile.close();
    }

  for(int i=0; i<jsonTeleportData.count(); i++)
    {
      QJsonObject jsonTeleportNode = jsonTeleportData[i].toObject();
      QJsonObject jsonInfo = jsonTeleportNode["teleport"].toObject();

      {
	QString str = jsonInfo["pos"].toString();
	QStringList xyz = str.split(" ", QString::SkipEmptyParts);
	m_teleports << QVector3D(xyz[0].toFloat(),
				 xyz[1].toFloat(),
				 xyz[2].toFloat());
      }
    }
}

void
VR::saveTeleportNode()
{
  QDir jsondir(m_dataDir);
  QString jsonfile = jsondir.absoluteFilePath("teleport.json");

  QJsonArray jsonTeleportData;
  QJsonObject jsonTeleportNode;

  //------------------------------

  if (jsondir.exists("teleport.json"))
    {
      QFile loadFile(jsonfile);
      loadFile.open(QIODevice::ReadOnly);
      
      QByteArray data = loadFile.readAll();
      
      QJsonDocument jsonDoc(QJsonDocument::fromJson(data));
      
      jsonTeleportData = jsonDoc.array();

      loadFile.close();
    }

  //------------------------------

  float mat[16];
  m_final_xform.copyDataTo(mat);
  QString matStr;
  for(int i=0; i<16; i++)
    matStr += QString("%1 ").arg(mat[i]);
  
  QJsonObject jsonInfo;
  jsonInfo["matrix"] = matStr;

  QVector3D hmdPos = hmdPosition();

  jsonInfo["pos"] = QString("%1 %2 %3").arg(hmdPos.x()).arg(hmdPos.y()).arg(hmdPos.z());

  jsonInfo["scale"] = QString("%1 %2").arg(m_scaleFactor).arg(m_flightSpeed);

  jsonTeleportNode["teleport"] = jsonInfo ;

  jsonTeleportData << jsonTeleportNode;

  //------------------------------

  QJsonDocument saveDoc(jsonTeleportData);  
  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());
  saveFile.close();
  
  m_newTeleportFound = true;

  //---------------------
  sendTeleportsToMenu();
  //---------------------
}

void
VR::sendTeleportsToMenu()
{
  if (!m_showMap)
    return;

  loadTeleports();

  QList<QVector3D> teleports;

  for(int i=0; i<m_teleports.count(); i++)
    teleports << Global::menuCamProjectedCoordinatesOf(m_teleports[i]);

  m_leftMenu.setTeleports(teleports);

  m_newTeleportFound = false;
}

void
VR::checkTeleport(bool triggered)
{
  if (!m_showMap)
    return;

  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];

  m_currTeleportNumber = m_leftMenu.checkTeleport(matL, matR);
  if (m_currTeleportNumber >= 0 && triggered)
    {
      teleport();
      return;
    }

  if (triggered)
    {
      if (m_pinPt.x() >= 0)
	{
	  if (!m_leftMenu.pointingToMenu() ||
	      m_menuPanels[m_currPanel] == "00")
	    teleport(m_projectedPinPt);
	}
    }
}

void
VR::checkOptions(bool triggered)
{
  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];
  m_leftMenu.checkOptions(matL, matR, triggered);
}

void
VR::setTimeStep(QString stpStr)
{
  m_leftMenu.setTimeStep(stpStr);
}


bool
VR::buildTeleport()
{
  if (!m_leftMenu.pointingToMenu() ||
      m_menuPanels[m_currPanel] != "00") // not pointing to map menu
    {
      if (m_currTeleportNumber < 0)
	{
	  m_telPoints = 0;
	  return false;
	}
    }


  float tht = 0.25/m_coordScale;
  QVector3D telPos; 
  if (m_leftMenu.pointingToMenu() &&
      m_menuPanels[m_currPanel] == "00") // pointing to map menu
    {
      if (m_pinPt.x() < 0)
	return false;

      telPos = m_projectedPinPt;
    }
  else
    telPos = m_teleports[m_currTeleportNumber] - QVector3D(0,0,1);

  QVector3D telPosU = telPos + QVector3D(0,0,tht);

  QVector<float> vert;
  vert << telPos.x();
  vert << telPos.y();
  vert << telPos.z();      
  vert << telPosU.x();
  vert << telPosU.y();
  vert << telPosU.z();      
        
  int npt = 2;

  float vt[100];  
  memset(vt, 0, sizeof(float)*100);
  for(int v=0; v<npt; v++)
    {
      vt[8*v + 0] = vert[3*v+0];
      vt[8*v + 1] = vert[3*v+1];
      vt[8*v + 2] = vert[3*v+2];

      vt[8*v + 6] = 0.5*(1-v); // texture coordinates
      vt[8*v + 7] = 0.5*(1-v);
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  m_axesPoints*15,
		  sizeof(float)*npt*8,
		  &vt[0]);

  m_telPoints = npt;
  m_nboxPoints = m_axesPoints + m_telPoints;

  return true;
}

void
VR::buildPinPoint()
{
  m_pinPoints = 0;


  if (m_flightActive ||
      m_currTeleportNumber >= 0)
    return;
  

  float tht = 0.25/m_coordScale;
  QVector3D telPos = m_projectedPinPt;
  QVector3D telPosU = telPos + QVector3D(0,0,tht);

  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];
  QVector3D cenR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D cenW = m_final_xformInverted.map(cenR);

  if (m_leftMenu.pointingToMenu())
    {
      if (m_menuPanels[m_currPanel] != "00") // not pointing to map menu
	{
	  QVector3D pp = m_leftMenu.pinPoint();
	  
	  if (pp.x() < 0)
	    return;

	  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
	  QVector3D ppW = m_final_xformInverted.map(pp);
	  
	  telPos = cenW;
	  telPosU = ppW;
	}
    }
  else
    {
      if (m_pinPt.x() < 0)
	return;

      telPosU = telPos;
      telPos = cenW;
    }

  
  QVector<float> vert;
  vert << telPos.x();
  vert << telPos.y();
  vert << telPos.z();      
  vert << telPosU.x();
  vert << telPosU.y();
  vert << telPosU.z();      

  int npt = 2;
  
  float vt[100];  
  memset(vt, 0, sizeof(float)*100);
  for(int v=0; v<npt; v++)
    {
      vt[8*v + 0] = vert[3*v+0];
      vt[8*v + 1] = vert[3*v+1];
      vt[8*v + 2] = vert[3*v+2];
    }
  
  if (!m_leftMenu.pointingToMenu() ||
      m_menuPanels[m_currPanel] != "00")
    {
      m_telPoints = 0;
      m_pinPoints = npt;

      vt[6] = 0.8; // texture coordinates
      vt[7] = 0.8;
      vt[12]= 0.5;
      vt[13]= 0.5;
    }
  else
    {
      m_telPoints = npt;
      m_pinPoints = 0;

      vt[6] = 1.0; // texture coordinates
      vt[7] = 1.0;
      vt[12] = 0.5;
      vt[13] = 0.5;
    }

  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  m_axesPoints*15+m_telPoints*sizeof(float)*8,
		  sizeof(float)*npt*8,
		  &vt[0]);

  m_nboxPoints = m_axesPoints + m_telPoints + m_pinPoints;
}

void
VR::renderTeleport(vr::Hmd_Eye eye)
{
  if (m_telPoints == 0 && m_pinPoints == 0)
    return;
  
  QMatrix4x4 mvp = viewProjection(eye);
    
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture

  if (m_telPoints > 0)
    glUniform3f(rcShaderParm[2], 1, 1, 1); // color
  else
    glUniform3f(rcShaderParm[2], 1, 1, 0); // color

  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], 0.5); // opacity modulator
  glUniform1i(rcShaderParm[5], 4); // do not apply texture

  glUniform1f(rcShaderParm[7], 0.5); // mixcolor

  glBindVertexArray(m_boxVID);
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);

  // Identify the components in the vertex buffer
  glEnableVertexAttribArray( 0 );
  glVertexAttribPointer( 0, //attribute 0
			 3, // size
			 GL_FLOAT, // type
			 GL_FALSE, // normalized
			 sizeof(float)*8, // stride
			 (char *)NULL + m_axesPoints*15 ); // starting offset

  glEnableVertexAttribArray( 1 );
  glVertexAttribPointer( 1,
			 3,
			 GL_FLOAT,
			 GL_FALSE,
			 sizeof(float)*8,
			 (char *)NULL + m_axesPoints*15 + sizeof(float)*3 );
  
  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + m_axesPoints*15 + sizeof(float)*6 );

  if (m_telPoints > 0)
    glLineWidth(20);
  else
    glLineWidth(2);

  glDrawArrays(GL_LINE_STRIP, 0, m_telPoints+m_pinPoints);

  
	
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_BLEND);
}


void
VR::projectPinPoint()
{
  if (!m_showMap || m_depthBuffer == 0) // no depth buffer found
    return;

  if (m_leftMenu.pointingToMenu() &&
      m_menuPanels[m_currPanel] == "00")
    {
      m_pinPt = m_leftMenu.pinPoint2D();

      if (m_pinPt.x() >= 0)
	{
	  int wd = screenWidth();
	  int ht = screenHeight();
	  
	  float px = m_pinPt.x();
	  float py = m_pinPt.y();
	  
	  int x = (1-px)*(wd-1);
	  int y = py*(ht-1);
	  
	  int dx = (1-px)*(wd-1);
	  int dy = (1-py)*(ht-1);
	  float z = m_depthBuffer[dy*wd + dx];
	  
	  Vec ppt;
	  if (z > 0.0 && z < 1.0)
	    {
	      ppt = Vec(x, y, z);
	      ppt = Global::menuCamUnprojectedCoordinatesOf(ppt);	  
	      m_projectedPinPt = QVector3D(ppt.x, ppt.y, ppt.z);
	      return;
	    }
	}
    }

  // if we are not pointing into the map then
  // may be we are looking directly on the terrain
  if (!nextHit())
    m_pinPt = QVector2D(-1, -1);
    
}

bool
VR::nextHit()
{
  if (!m_showMap || m_depthBuffer == 0) // we don't have any depth buffer
    return false;

  // don't find hit point if we are pointing to a menu
  if (pointingToMenu())
    return false;
  
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];

  QVector3D centerR = QVector3D(matR * QVector4D(0,0,0,1));
  QVector3D pinPoint = QVector3D(matR * QVector4D(0,0,-0.1,1));

  QVector3D cenV = m_final_xformInverted.map(centerR);
  QVector3D pinV = m_final_xformInverted.map(pinPoint);
  
  Vec cenW = Vec(cenV.x(), cenV.y(), cenV.z());
  Vec pinW = Vec(pinV.x(), pinV.y(), pinV.z());

  Vec cenP = Global::menuCamProjectedCoordinatesOf(cenW);
  Vec pinP = Global::menuCamProjectedCoordinatesOf(pinW);

  // walk along the vector in depth buffer to find a hit
  Vec dvec = (pinP-cenP).unit();

  int wd = screenWidth();
  int ht = screenHeight();

  Vec startP = cenP;

  //---------------------------
  if (startP.x < 0 ||
      startP.x >= wd ||
      startP.y < 0 ||
      startP.y >= ht)
    {
      // if we are not on the map then get to the edge
      // before start of search
      // find ray box intersection
      // screen box is (0,0) and (wd,ht)
      if (qAbs(dvec.x) > 0 &&
	  qAbs(dvec.y) > 0)
	{
	  float t[6];
	  t[0] = (0  - cenP.x)/dvec.x;
	  t[1] = (wd - cenP.x)/dvec.x;
	  
	  t[2] = (0  - cenP.y)/dvec.y;
	  t[3] = (ht - cenP.y)/dvec.y;
	  
	  t[4] = qMax(qMin(t[0],t[1]), qMin(t[2],t[3]));
	  t[5] = qMin(qMax(t[0],t[1]), qMax(t[2],t[3]));
	  
	  float db;
	  if (t[5] < 0 || t[4]>t[5])
	    return false;
	  else
	    db = t[4];
	  
	  startP = cenP + db*dvec;
	}
      else
	{
	  float t[4];
	  if (qAbs(dvec.x) > 0)
	    {
	      t[0] = (0  - cenP.x)/dvec.x;
	      t[1] = (wd - cenP.x)/dvec.x;
	    }
	  else if (qAbs(dvec.y) > 0)
	    {	  
	      t[0] = (0  - cenP.y)/dvec.y;
	      t[1] = (ht - cenP.y)/dvec.y;	  
	    }
	  else
	    return false;
	  
	  t[2] = qMin(t[0],t[1]);
	  t[3] = qMax(t[0],t[1]);
	  
	  float db;
	  if (t[3] < 0 || t[2]>t[3])
	    return false;
	  else
	    db = t[2];
	  
	  startP = cenP + db*dvec;      
	}
    }
  //---------------------------


  Vec v = startP + dvec;
  while(1)
    {
      int dx = v.x;
      int dy = v.y;
      if (dx < 0 || dx > wd-1 ||
	  dy < 0 || dy > ht-1)
	return false;
      
      float z = m_depthBuffer[(ht-1-dy)*wd + dx];

      if (z > 0.0 &&
	  z < 1.0 &&
	  z <= v.z)
	{
	  Vec hitP = Global::menuCamUnprojectedCoordinatesOf(v);

	  m_projectedPinPt = QVector3D(hitP.x, hitP.y, hitP.z);

	  m_pinPt = QVector2D(v.x/wd, v.y/ht);
	  return true;
	}

      v += dvec;
    }

  return false;
}

void
VR::sendCurrPosToMenu()
{
  if (!m_showMap)
    return;

  QVector3D hmdPos = hmdPosition();

  Vec hpos(hmdPos.x(),hmdPos.y(),hmdPos.z());
  hpos = Global::menuCamProjectedCoordinatesOf(hpos);    
  
  QVector3D thdir = hmdPos - 10*viewDir();
  Vec hposD(thdir.x(),thdir.y(),thdir.z());
  hposD = Global::menuCamProjectedCoordinatesOf(hposD);    
  
  Vec hdr = hposD-hpos;
  hdr.normalize();
  hposD = hpos + 40*hdr;
  
  setCurrPosOnMenuImage(hpos, hposD);  
}

void
VR::renderSkyBox(vr::Hmd_Eye eye)
{
  if (!m_showSkybox)
    return;

  QVector3D hpos = hmdPosition();
  QMatrix4x4 mvp = viewProjection(eye);
  
  m_skybox.draw(mvp, hpos, 2.0/m_coordScale);
}

void
VR::showHUD(vr::Hmd_Eye eye,
	    GLuint texId, QSize texSize)
{
//  //glDepthMask(GL_FALSE); // disable writing to depth buffer
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  QMatrix4x4 mvp = currentViewProjection(eye);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 0,0,0); // color
  glUniform3f(rcShaderParm[3], 0,0,0); // view direction
  glUniform1f(rcShaderParm[4], 0.5); // opacity modulator
  glUniform1i(rcShaderParm[5], 5); // apply texture
  glUniform1f(rcShaderParm[6], 50); // pointsize
  glUniform1f(rcShaderParm[7], 0); // mix color

  QMatrix4x4 mat = m_matrixDevicePose[vr::k_unTrackedDeviceIndex_Hmd];

  QVector3D cen =QVector3D(mat * QVector4D(0,0,0,1));
  QVector3D cx = QVector3D(mat * QVector4D(1,0,0,1));
  QVector3D cy = QVector3D(mat * QVector4D(0,1,0,1));
  QVector3D cz = QVector3D(mat * QVector4D(0,0,1,1));

  int texWd = texSize.width();
  int texHt = texSize.height();
  float frc = 1.0/qMax(texWd, texHt);

  QVector3D rDir = (cx-cen).normalized();
  QVector3D uDir = (cy-cen).normalized();
  QVector3D vDir = (cz-cen).normalized();

  cen = cen - vDir - 0.5*uDir;

  QVector3D fv = 0.5*(uDir-vDir).normalized();
  QVector3D fr = 0.5*rDir;

  QVector3D vu = frc*texHt*fv;
  QVector3D vr0 = cen - frc*texWd*0.5*fr;
  QVector3D vr1 = cen + frc*texWd*0.5*fr;

  QVector3D v0 = vr0;
  QVector3D v1 = vr0 + vu;
  QVector3D v2 = vr1 + vu;
  QVector3D v3 = vr1;

  float vertData[50];

  vertData[0] = v0.x();
  vertData[1] = v0.y();
  vertData[2] = v0.z();
  vertData[3] = 0;
  vertData[4] = 0;
  vertData[5] = 0;
  vertData[6] = 0.0;
  vertData[7] = 0.0;

  vertData[8] = v1.x();
  vertData[9] = v1.y();
  vertData[10] = v1.z();
  vertData[11] = 0;
  vertData[12] = 0;
  vertData[13] = 0;
  vertData[14] = 0.0;
  vertData[15] = 1.0;

  vertData[16] = v2.x();
  vertData[17] = v2.y();
  vertData[18] = v2.z();
  vertData[19] = 0;
  vertData[20] = 0;
  vertData[21] = 0;
  vertData[22] = 1.0;
  vertData[23] = 1.0;

  vertData[24] = v3.x();
  vertData[25] = v3.y();
  vertData[26] = v3.z();
  vertData[27] = 0;
  vertData[28] = 0;
  vertData[29] = 0;
  vertData[30] = 1.0;
  vertData[31] = 0.0;


  // Create and populate the index buffer

  glBindVertexArray(m_boxVID);  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxIB);  

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0,  // attribute 0
			3,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200); // starting offset
  
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1,  // attribute 1
			3,  // size
			GL_UNSIGNED_BYTE, // type
			GL_FALSE, // normalized
			sizeof(float)*8, // stride
			(char *)NULL + 200 + sizeof(float)*3 );

  glEnableVertexAttribArray( 2 );
  glVertexAttribPointer( 2,
			 2,
			 GL_FLOAT,
			 GL_FALSE, 
			 sizeof(float)*8,
			 (char *)NULL + 200 + sizeof(float)*6 );

  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_2D, texId);
  //glBindTexture(GL_TEXTURE_2D, Global::boxSpriteTexture());
  glEnable(GL_TEXTURE_2D);
  
  glBufferSubData(GL_ARRAY_BUFFER,
		  200, // offset of 200 bytes
		  sizeof(float)*8*4,
		  &vertData[0]);

  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);  

  glBindVertexArray(0);

  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glDisable(GL_TEXTURE_2D);

  glDisable(GL_BLEND);

  //glDepthMask(GL_TRUE); // enable writing to depth buffer
  glEnable(GL_DEPTH_TEST);
}
