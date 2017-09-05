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

  m_touchActiveRight = false;
  m_touchActiveLeft = false;
  m_triggerActiveRight = false;
  m_triggerActiveLeft = false;
  m_bothTriggersActive = false;
  
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
  m_speedDamper = 0.0005;

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

  connect(&m_leftMenu, SIGNAL(resetModel()),
	  this, SLOT(resetModel()));
  connect(&m_leftMenu, SIGNAL(updatePointSize(int)),
	  this, SLOT(updatePointSize(int)));
  connect(&m_leftMenu, SIGNAL(updateScale(int)),
	  this, SLOT(updateScale(int)));

  connect(&m_leftMenu, SIGNAL(updateEdges(bool)),
	  this, SLOT(updateEdges(bool)));
  connect(&m_leftMenu, SIGNAL(updateSoftShadows(bool)),
	  this, SLOT(updateSoftShadows(bool)));
}

void
VR::updatePointSize(int sz)
{
  m_pointSize = qMax(0.5f, m_pointSize + sz*0.1f);
}

void VR::updateEdges(bool b) { m_edges = b; }
void VR::updateSoftShadows(bool b) { m_softShadows = b; }

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
VR::resetModel()
{
  m_final_xform = m_las_xform;

  m_scaleFactor = m_coordScale;
  m_flightSpeed = 1.0;

  m_model_xform.setToIdentity();

  genEyeMatrices();

  m_play = false;
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
  
  bool touchPressedLeft = false;
  bool triggerPressedLeft = false;
  
  bool touchPressedRight = false;
  bool triggerPressedRight = false;

  bool touchTriggerPressedLeft = false;
  bool touchTriggerPressedRight = false;

  
  vr::VRControllerState_t stateLeft;
  bool leftActive = m_hmd->GetControllerState(m_leftController,
					      &stateLeft,
					      sizeof(stateLeft));

  vr::VRControllerState_t stateRight;
  bool rightActive = m_hmd->GetControllerState(m_rightController,
					       &stateRight,
					       sizeof(stateRight));

  bool leftTriggerActive = isTriggered(stateLeft);
  bool rightTriggerActive = isTriggered(stateRight);

  bool leftTouchActive = isTouched(stateLeft);
  bool rightTouchActive = isTouched(stateRight);

  touchTriggerPressedLeft = leftTriggerActive && leftTouchActive;
  touchTriggerPressedRight = rightTriggerActive && rightTouchActive;

  //-----------------------------
  if (leftTriggerActive && rightTriggerActive)
    {
      handleBothTriggersPressed(m_leftController, 
				m_rightController);
      return;
    }
  
  if (!leftTriggerActive && !rightTriggerActive)
    {
      if (m_bothTriggersActive)
	{
	  m_bothTriggersActive = false;

	  m_final_xform = m_model_xform * m_final_xform;
	  m_model_xform.setToIdentity();

	  // generate the drawlist each time changes are made
	  m_genDrawList = true;

	  return;
	}

      m_bothTriggersActive = false;
    }

  // true if one of them is still activ
  // after other has been released which happens most of the times
  if (m_bothTriggersActive)
    return;
  //-----------------------------
  
  //-----------------------------
  if (leftActive)
    handleControllerLeft(m_leftController, triggerPressedLeft, touchPressedLeft);
  
  if (m_triggerActiveLeft && !triggerPressedLeft)
    {
      m_triggerActiveLeft = false;

      m_final_xform = m_model_xform * m_final_xform;
      m_model_xform.setToIdentity();

      // generate the drawlist each time changes are made
      m_genDrawList = true;

    }
  
  if (m_touchActiveLeft && !touchPressedLeft)
    {
      if (m_touchX > m_startTouchX)
	nextMenu();
      else
	previousMenu();
      
      m_touchActiveLeft = false;
    }
  //-----------------------------
  

  //-----------------------------
  if (rightActive)
    handleRight(m_rightController, triggerPressedRight, touchPressedRight);
      
  if (m_flightActive)
    {
      // generate the drawlist each time changes are made
      if (!m_flyTimer.isActive())
	{
	  m_genDrawList = true;
	  m_flyTimer.start(5000); // generate new draw list every 5 sec
	}

      if (!rightTouchActive && !rightTriggerActive)
	{
	  m_flightActive = false;

	  m_final_xform = m_model_xform * m_final_xform;
	  m_model_xform.setToIdentity();

	  m_genDrawList = true;
	  m_flyTimer.stop();
	}
    }

  bool rightTriggered = false; // check whether trigger was released
  if (m_triggerActiveRight && !triggerPressedRight)
    {
      m_triggerActiveRight = false;
      rightTriggered = true; // trigger now released

      m_final_xform = m_model_xform * m_final_xform;
      m_model_xform.setToIdentity();

      // generate the drawlist each time changes are made
      m_genDrawList = true;

    }
  
  if (m_touchActiveRight && !touchPressedRight)
    {
      m_touchActiveRight = false;

      m_final_xform = m_model_xform * m_final_xform;
      m_model_xform.setToIdentity();      
      
      // generate the drawlist each time changes are made
      m_genDrawList = true;

    }
  //-----------------------------

  bool gripPressedLeft = false;
  bool gripPressedRight = false;

  if (leftActive)
    handleGripLeft(m_leftController, gripPressedLeft);

  if (rightActive)
    handleGripRight(m_rightController, gripPressedRight);

  if (m_gripActiveLeft && !gripPressedLeft)
    m_gripActiveLeft = false;

  if (m_gripActiveRight && !gripPressedRight)
    m_gripActiveRight = false;


  //-----------------------------
  // show and act on left map only if left trigger is not pressed
  if (!m_triggerActiveLeft)
    {
      if (m_showMap)
	checkTeleport(rightTriggered);
      
      checkOptions(rightTriggered);
    }
  //-----------------------------
}

void
VR::handleBothTriggersPressed(vr::TrackedDeviceIndex_t leftController,
			      vr::TrackedDeviceIndex_t rightController)
{
  QVector3D posL =  getPosition(m_trackedDevicePose[leftController].mDeviceToAbsoluteTracking);
  QVector3D posR =  getPosition(m_trackedDevicePose[rightController].mDeviceToAbsoluteTracking);
  if (m_bothTriggersActive)
    {
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
  else
    {
      m_bothTriggersActive = true;

      m_lrDist = posL.distanceToPoint(posR);

      m_prevPosL = posL;
      m_prevPosR = posR;
      m_prevDirection = (m_prevPosL - m_prevPosR).normalized();

      QVector3D cenL = getPosition(leftController);
      QVector3D cenR = getPosition(rightController);
      m_scaleCenter = (cenL + cenR)*0.5;

      m_prevScaleFactor = m_scaleFactor;
      m_prevFlightSpeed = m_flightSpeed;
    }
}

void
VR::handleRightTrigger(vr::TrackedDeviceIndex_t i,
		       bool &triggerPressedRight)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;
  
  if (!triggerPressedRight)
    triggerPressedRight = (state.ulButtonPressed &
			   vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
  
  // translation
  if (triggerPressedRight)
    {
      if (m_triggerActiveRight)
	{
	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
	  pos = (pos - m_startTranslate);
	  
	  m_model_xform.setToIdentity();
	  m_model_xform.translate(pos);
	}
      else
	{
	  m_triggerActiveRight = true;
		  
	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
	  m_startTranslate = pos;
	}
    }
}

void
VR::handleFlight(vr::TrackedDeviceIndex_t i,
		 bool &touchPressedRight)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;
  
  if (!touchPressedRight)
    touchPressedRight = (state.ulButtonPressed &
			vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));


  if (touchPressedRight)
    {
      if (m_flightActive)
	{
	  m_model_xform.setToIdentity();      

	  QMatrix4x4 mat = m_matrixDevicePose[i];    
	  QVector4D center = mat * QVector4D(0,0,0,1);
	  QVector4D point = mat * QVector4D(0,0,1,1);
	  QVector3D moveD = QVector3D(center-point);

	  moveD.normalize();
	  QVector3D move = moveD*m_flightSpeed*m_speedDamper;

	  if (m_touchY < -0.5) // move backward
	    move = -move;
	  
	  m_model_xform.translate(-move);

	  //---------------------
	  // scale up if needed
	  float sf = m_teleportScale/m_scaleFactor;
	  if (sf > 1.0)
	    {
	      sf = qPow(sf, 0.05f);

	      //QVector3D cenL = getPosition(m_rightController);
	      QVector3D cenL = m_final_xform.map(m_coordCen);
	      
	      m_model_xform.translate(cenL-move);
	      m_model_xform.scale(sf);
	      m_model_xform.translate(-(cenL-move));
	      
	      m_scaleFactor *= sf;
	      m_flightSpeed *= sf;
	    }
	  //---------------------

	  m_final_xform = m_model_xform * m_final_xform;

	  genEyeMatrices();
	}
      else
	{
	  m_flightActive = true;
		  
	  m_touchX = state.rAxis[0].x;
	  m_touchY = state.rAxis[0].y;

	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
	  m_startTranslate = pos;

	  m_flyTimer.start(5000); // generate new draw list every 5 sec
	}
    }
}

void
VR::handleRight(vr::TrackedDeviceIndex_t i,
		bool &triggerPressedRight,
		bool &touchPressedRight)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;
  
  if (!triggerPressedRight)
    triggerPressedRight = (state.ulButtonPressed &
			   vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));

  if (!touchPressedRight)
    touchPressedRight = (state.ulButtonTouched &
			vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));


  if (touchPressedRight)
    {
      handleFlight(m_rightController, touchPressedRight);
      return;
    }

  if (triggerPressedRight)
    {
      handleRightTrigger(i, triggerPressedRight);
      
      return;
    }

//  if (triggerPressedRight && touchPressedRight)
//    {
//      if (m_touchTriggerActiveRight)
//	{
//	  m_model_xform.setToIdentity();      
//
//	  //float sf = state.rAxis[0].y - m_touchY;		  
//
//	  QMatrix4x4 mat = m_matrixDevicePose[i];    
//	  QVector4D center = mat * QVector4D(0,0,0,1);
//	  QVector4D point = mat * QVector4D(0,0,1,1);
//	  QVector3D moveD = QVector3D(center-point);
//
//	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
//	  QVector3D sp = (pos-m_startTranslate);
//	  float len = sp.length();
//
//	  sp.normalize();
//	  moveD.normalize();
//
//	  len *= QVector3D::dotProduct(sp, moveD);
//
//	  QVector3D move = moveD*len*m_flightSpeed*m_speedDamper;
//
//	  
//	  m_model_xform.translate(-move);
//
//
//	  //--------------------------
//	  // rotation
//	  QQuaternion q = getQuaternion(m_matrixDevicePose[i]);
//	  q.normalize();
//	  q = QQuaternion::slerp(m_rotQuat, q, 0.5);
//	  q = m_rotQuat.conjugate() * q;
//
//	  m_model_xform.translate(pos);
//	  m_model_xform.rotate(q);
//	  m_model_xform.translate(-pos);
//	  //--------------------------
//
//	  m_final_xform = m_model_xform * m_final_xform;
//
//	  m_rotQuat = getQuaternion(m_matrixDevicePose[i]);
//	  m_rotQuat.normalize();
//	}
//      else
//	{
//	  m_touchTriggerActiveRight = true;
//		  
//	  m_touchX = state.rAxis[0].x;
//	  m_touchY = state.rAxis[0].y;
//
//
//	  m_touchPoint = QVector2D(state.rAxis[0].x,
//				   state.rAxis[0].y);
//	  m_touchPoint.normalize();
//
//
//	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
//	  m_startTranslate = pos;
//
//	  m_rotQuat = getQuaternion(m_matrixDevicePose[i]);
//	  m_rotQuat.normalize();
//
//	  m_flyTimer.start(5000); // generate new draw list every 5 sec
//	}
//    }
}


bool
VR::isTriggered(vr::VRControllerState_t &state)
{
  return (state.ulButtonPressed &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
}

bool
VR::isTouched(vr::VRControllerState_t &state)
{
  return (state.ulButtonTouched &
	  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));
}

void
VR::handleControllerLeft(vr::TrackedDeviceIndex_t i,
			 bool &triggerPressedLeft,
			 bool &touchPressedLeft)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;

  //------------------------
  if (!triggerPressedLeft)
    triggerPressedLeft = (state.ulButtonPressed &
			  vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger));
  
  if (triggerPressedLeft)
    {
      if (m_triggerActiveLeft)
	{
	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
	  pos = (pos - m_startTranslate);
	  
	  m_model_xform.setToIdentity();
	  m_model_xform.translate(pos);
	}
      else
	{
	  m_triggerActiveLeft = true;
		  
	  QVector3D pos =  getPosition(m_trackedDevicePose[i].mDeviceToAbsoluteTracking);
	  m_startTranslate = pos;
	}
    }
  //------------------------
  
  
	  //------------------------
  if (!touchPressedLeft)
    touchPressedLeft = (state.ulButtonTouched &
			vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad));

  if (touchPressedLeft)
    {
      if (m_touchActiveLeft)
	{
	  //m_nextStep = 0;
	  //if (state.rAxis[0].x > 0.5) m_nextStep = 1;
	  //if (state.rAxis[0].x < 0.5) m_nextStep = -1;
	  m_touchX = state.rAxis[0].x;
	  m_touchY = state.rAxis[0].y;
	}
      else
	{
	  m_touchActiveLeft = true;

	  m_startTouchX = state.rAxis[0].x;
	  m_startTouchY = state.rAxis[0].y;
	}
    }
}

void
VR::handleGripLeft(vr::TrackedDeviceIndex_t i,
		   bool &gripPressedLeft)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;

  if (!gripPressedLeft)
    gripPressedLeft = (state.ulButtonPressed &
		       vr::ButtonMaskFromId(vr::k_EButton_Grip));
  
  if (gripPressedLeft)
    {
      if (m_gripActiveLeft)
	{
	}
      else
	{
	  m_gripActiveLeft = true;

	  saveTeleportNode();
	}
    }  
}

void
VR::handleGripRight(vr::TrackedDeviceIndex_t i,
		    bool &gripPressedRight)
{
  vr::VRControllerState_t state;
  if (! m_hmd->GetControllerState(i, &state, sizeof(state)))
    return;

  if (!gripPressedRight)
    gripPressedRight = (state.ulButtonPressed &
		       vr::ButtonMaskFromId(vr::k_EButton_Grip));
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
  updatePoses();
  updateInput();
  buildAxes();
  if (m_showMap)
    {
      buildTeleport();
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
  if (m_showMap)
    renderTeleport(vr::Eye_Left);

  renderControllers(vr::Eye_Left);

  renderAxes(vr::Eye_Left);

  renderMenu(vr::Eye_Left);

  m_leftBuffer->release();

  QRect sourceRect(0, 0, m_eyeWidth, m_eyeHeight);
  QRect targetLeft(0, 0, m_eyeWidth, m_eyeHeight);
  QOpenGLFramebufferObject::blitFramebuffer(m_resolveBuffer, targetLeft,
					    m_leftBuffer, sourceRect);
}

void
VR::postDrawRightBuffer()
{
  if (m_showMap)
    renderTeleport(vr::Eye_Right);
  
  renderControllers(vr::Eye_Right);

  renderAxes(vr::Eye_Right);

  renderMenu(vr::Eye_Right);

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
  glUseProgram(m_pshader);

  
  QMatrix4x4 mvp = currentViewProjection(eye);

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
					  "shaders/punlit.vert",
					  "shaders/punlit.frag"))
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
}


void
VR::buildAxesVB()
{
  //------------------------
  glGenVertexArrays(1, &m_boxVID);
  glBindVertexArray(m_boxVID);

  glGenBuffers(1, &m_boxV);

  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferData(GL_ARRAY_BUFFER,
	       100*sizeof(float),
	       NULL,
	       GL_STATIC_DRAW);
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
  glUniform3f(rcShaderParm[2], 0.0,0.8,1); // mix color
  glUniform3f(rcShaderParm[3], vd.x(), vd.y(), vd.z()); // view direction
  glUniform1f(rcShaderParm[4], 1); // opacity modulator
  glUniform1i(rcShaderParm[5], 2); // applytexture
  m_rTrackedDeviceToRenderModel[ m_leftController ]->Draw();


  matDeviceToTracking = m_matrixDevicePose[m_rightController];
  mvp = currentViewProjection(eye) * matDeviceToTracking;
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  //glUniform1i(rcShaderParm[1], 0); // texture
  glUniform3f(rcShaderParm[2], 1,0.8,0.0); // mix color
  //glUniform3f(rcShaderParm[3], vd.x(), vd.y(), vd.z()); // view direction
  //glUniform1f(rcShaderParm[4], 1); // opacity modulator
  //glUniform1i(rcShaderParm[5], 2); // applytexture
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
  
  //QVector3D hpos = m_final_xform.map(hmdPosition());
  QVector3D cenL = getPosition(m_leftController);

  m_model_xform.translate(cenL);
  m_model_xform.scale(sf);
  m_model_xform.translate(-cenL);

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
  if (!sm)
    m_menuPanels.removeOne("01");
  
  m_currPanel = 0;
  m_leftMenu.setCurrentMenu(m_menuPanels[m_currPanel]);
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
  if (m_bothTriggersActive || m_triggerActiveLeft)
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
}

void
VR::setCurrPosOnMenuImage(Vec cp, Vec cd)
{
  QVector3D vcp = QVector3D(cp.x, cp.y, cp.z);
  QVector3D vcd = QVector3D(cd.x, cd.y, cd.z);
  m_leftMenu.setCurrPos(vcp, vcd);
}

void
VR::sendTeleportsToMenu(QList<QVector3D> tp)
{
  if (!m_showMap)
    return;
  
  m_newTeleportFound = false;
  m_leftMenu.setTeleports(tp);

  loadTeleports();
}

void
VR::checkTeleport(bool triggered)
{
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
      //m_pinPt = m_leftMenu.pinPoint2D();
      if (m_pinPt.x() >= 0)
	teleport(m_projectedPinPt);
    }
}

void
VR::checkOptions(bool triggered)
{
  QMatrix4x4 matL = m_matrixDevicePose[m_leftController];
  QMatrix4x4 matR = m_matrixDevicePose[m_rightController];

  int option = m_leftMenu.checkOptions(matL, matR, triggered);

  if (option < 0 || !triggered)
    return;
  
  if (option == 0)
    {
      m_play = !m_play;
      m_leftMenu.setPlay(m_play);
      
      if (m_play)
	gotoNextStep();
    }
  else if (option == 1)
    {
      m_play = false;
      m_leftMenu.setPlay(m_play);
      gotoPreviousStep();
    }
  else if (option == 2)
    {
      m_play = false;
      m_leftMenu.setPlay(m_play);
      gotoNextStep();
    }
  else if (option == 3)
    {
      m_play = false;
      m_leftMenu.setPlay(m_play);
      gotoFirstStep();
    }
}

void
VR::setTimeStep(QString stpStr)
{
  m_leftMenu.setTimeStep(stpStr);
}


void
VR::buildTeleport()
{
  if (m_currTeleportNumber < 0)
    {
      m_telPoints = 0;
      return;
    }

  float tht = 0.25/m_coordScale;
  QVector3D telPos = m_teleports[m_currTeleportNumber] - QVector3D(0,0,1);
  QVector3D telPosU = telPos + QVector3D(0,0,tht);

  QVector<float> vert;
  QVector<uchar> col;
  int npt = 0;

  QVector3D color(255,255,255);

  vert << telPos.x();
  vert << telPos.y();
  vert << telPos.z();      
  vert << telPosU.x();
  vert << telPosU.y();
  vert << telPosU.z();      
      
  col << color.x();
  col << color.y();
  col << color.z();      
  col << color.x();
  col << color.y();
  col << color.z();
  
  npt = vert.count()/3;


  float vt[100];  
  memset(vt, 0, sizeof(float)*100);
  for(int v=0; v<npt; v++)
    {
      vt[8*v + 0] = vert[3*v+0];
      vt[8*v + 1] = vert[3*v+1];
      vt[8*v + 2] = vert[3*v+2];

      vt[8*v + 6] = 1-v; // texture coordinates
      vt[8*v + 7] = 1-v;
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  m_axesPoints*15,
		  sizeof(float)*npt*8,
		  &vt[0]);

  m_telPoints = npt;
  m_nboxPoints = m_axesPoints + m_telPoints;
}

QVector2D
VR::pinPoint2D()
{
  m_pinPt = m_leftMenu.pinPoint2D();
  return m_pinPt;
}

void
VR::buildPinPoint()
{
  // don't draw pin point if teleport detected
  // or pin point out of range
  if (m_pinPt.x() < 0 ||
      m_currTeleportNumber >= 0)
    {
      m_pinPoints = 0;
      return;
    }
  
  float tht = 0.25/m_coordScale;
  QVector3D telPos = m_projectedPinPt;
  QVector3D telPosU = telPos + QVector3D(0,0,tht);

  QVector<float> vert;
  QVector<uchar> col;
  int npt = 0;

  QVector3D color(255,255,255);

  vert << telPos.x();
  vert << telPos.y();
  vert << telPos.z();      
  vert << telPosU.x();
  vert << telPosU.y();
  vert << telPosU.z();      
      
  col << color.x();
  col << color.y();
  col << color.z();      
  col << color.x();
  col << color.y();
  col << color.z();
  
  npt = vert.count()/3;


  float vt[100];  
  memset(vt, 0, sizeof(float)*100);
  for(int v=0; v<npt; v++)
    {
      vt[8*v + 0] = vert[3*v+0];
      vt[8*v + 1] = vert[3*v+1];
      vt[8*v + 2] = vert[3*v+2];

      vt[8*v + 6] = 1-v; // texture coordinates
      vt[8*v + 7] = 1-v;
    }
  
  glBindBuffer(GL_ARRAY_BUFFER, m_boxV);
  glBufferSubData(GL_ARRAY_BUFFER,
		  m_axesPoints*15+m_telPoints*sizeof(float)*8,
		  sizeof(float)*npt*8,
		  &vt[0]);

  m_pinPoints = npt;
  m_nboxPoints = m_axesPoints + m_telPoints + m_pinPoints;
}

void
VR::renderTeleport(vr::Hmd_Eye eye)
{
  if (m_telPoints == 0 && m_pinPoints == 0)
    return;
  
  QMatrix4x4 mvp = viewProjection(eye);
    
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  glUseProgram(ShaderFactory::rcShader());
  GLint *rcShaderParm = ShaderFactory::rcShaderParm();
  glUniformMatrix4fv(rcShaderParm[0], 1, GL_FALSE, mvp.data() );  
  glUniform1i(rcShaderParm[1], 4); // texture
  glUniform3f(rcShaderParm[2], 1, 1, 1); // mix color
  glUniform3f(rcShaderParm[3], 0, 0, 0); // view direction
  glUniform1f(rcShaderParm[4], 0.5); // opacity modulator
  glUniform1i(rcShaderParm[5], 4); // do not apply texture

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

  glLineWidth(20);

  glDrawArrays(GL_LINES, 0, m_telPoints+m_pinPoints);

//  if (m_telPoints > 0)
//    glDrawArrays(GL_LINES, 0, m_telPoints);
//
//  if (m_pinPoints > 0)
//    {
//      glUniform3f(rcShaderParm[2], 1, 1, 1); // mix color
//      glDrawArrays(GL_LINES, m_telPoints*sizeof(float)*8, m_pinPoints);
//    }
  
	
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);  
  glDisableVertexAttribArray(2);  

  glBindVertexArray(0);

  glUseProgram(0);

  glEnable(GL_BLEND);
}
