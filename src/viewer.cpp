#include "staticfunctions.h"
#include "viewer.h"
#include "global.h"
#include "shaderfactory.h"

#include <QMessageBox>
#include <QtMath>

#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include <QApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QInputDialog>
#include <QLineEdit>

#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

void Viewer::setImageMode(int im) { m_imageMode = im; }
void Viewer::setCurrentFrame(int fno)
{
  m_currFrame = fno;
}
void Viewer::setImageFileName(QString imgfl)
{
  m_imageFileName = imgfl;
  QFileInfo f(m_imageFileName);
  setSnapshotFormat(f.completeSuffix());
}
void Viewer::setSaveSnapshots(bool flag) { m_saveSnapshots = flag; }
void Viewer::setSaveMovie(bool flag) { m_saveMovie = flag; }
void
Viewer::setImageSize(int wd, int ht)
{
  m_imageWidth = wd;
  m_imageHeight = ht;

  resize(wd, ht);
}
void
Viewer::endPlay()
{
  if (m_saveMovie)
    endMovie();

  m_saveMovie = false;
  m_saveSnapshots = false;
}

Viewer::Viewer(QGLFormat &glfmt, QWidget *parent) :
  QGLViewer(glfmt, parent)
{
  //------------------------
  setMouseTracking(true);
  //------------------------

  m_pointSize = 2;

  m_npoints = 0;

  m_shadowShader = 0;
  m_smoothShader = 0;
  m_meshShader = 0;
  
  m_vbID = -1;
  m_vbPoints = 0;
  m_vertexBuffer[0] = 0;
  m_vertexBuffer[1] = 0;
  m_vertexArrayID = 0;

  m_blurShader = 0;

  m_vertexScreenBuffer = 0;

  m_flyMode = false;
  m_showInfo = true;
  m_showBox = false;
  m_showPoints = true;

  m_depthBuffer = 0;
  m_depthTex[0] = 0;
  m_depthTex[1] = 0;
  m_depthTex[2] = 0;
  m_depthTex[3] = 0;
  m_rbo = 0;

  m_colorTex = 0;
  m_colorMap = 0;

  m_minmaxTex = 0;
  m_minmaxMap = 0;

  m_visibilityTex = 0;
  m_visibilityMap = 0;


  m_smoothDepth = false;
  m_showEdges = true;
  m_softShadows = true;
  m_showSphere = false;

  m_selectActive = false;

  m_tiles.clear();
  m_orderedTiles.clear();

  m_pointClouds.clear();
  m_trisets.clear();
  
  m_nearDist = 0;
  m_farDist = 1;

  m_volume = 0;
  m_volumeFactory = 0;

  m_backButtonPos = QPoint(10, 100);

  m_vrMode = false;

#ifdef USE_GLMEDIA
  m_movieWriter = 0;
#endif // USE_GLMEDIA
  m_movieFrame = 0;

  reset();

  setMinimumSize(100,100);

  connect(this, SIGNAL(loadLinkedData(QString)),
	  this, SLOT(loadLink(QString)));

  connect(this, SIGNAL(loadLinkedData(QStringList)),
	  this, SLOT(loadLink(QStringList)));

  connect(this, SIGNAL(resetModel()),
	  &m_vr, SLOT(resetModel()));

  QTimer *fpsTimer = new QTimer(this);
  connect(fpsTimer, &QTimer::timeout,
	  this, &Viewer::updateFramerate);
  m_frames = 0;
  fpsTimer->start(1000);
}

Viewer::~Viewer()
{
  reset();

  if (m_vr.vrEnabled())
    m_vr.shutdown();
}

void
Viewer::updateFramerate()
{
//    if (m_frames > 0)
//      emit framesPerSecond(m_frames);

    m_frames = 0;
}

void
Viewer::reset()
{
  m_moveViewerToCenter = true;

  qint64 million = 1000000; 
  m_pointBudget = 5*million;

  m_editMode = false;
  m_moveAxis = -1;

  m_headSetType = 0; // None

  QString assetDir = qApp->applicationDirPath() + QDir::separator() + "assets";
  QString jsonfile = QDir(assetDir).absoluteFilePath("top.json");
  loadTopJson(jsonfile);

  m_firstImageDone = 0;
  m_vboLoadedAll = false;
  m_fastDraw = false;

  m_pointType = true; // adaptive
  m_pointSize = 2;

  m_pointsDrawn = 0;
  m_minNodePixelSize = 100;

  m_npoints = 0;
  m_dpv = 3;

  m_currTime = 0;
  m_maxTime = 0;

  if (m_vertexBuffer[0])glDeleteBuffers(2, m_vertexBuffer);
  if (m_vertexArrayID) glDeleteVertexArrays(1, &m_vertexArrayID);

  m_vertexBuffer[0] = 0;
  m_vertexBuffer[1] = 0;
  m_vertexArrayID = 0;


  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex[0]) glDeleteTextures(4, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  m_depthBuffer = 0;
  m_depthTex[0] = 0;
  m_depthTex[1] = 0;
  m_depthTex[2] = 0;
  m_depthTex[3] = 0;
  m_rbo = 0;


  if (m_colorTex) glDeleteTextures(1, &m_colorTex);
  m_colorTex = 0;
  if (m_colorMap) delete [] m_colorMap;
  m_colorMap = 0;


  if (m_minmaxTex) glDeleteTextures(1, &m_minmaxTex);
  m_minmaxTex = 0;
  if (m_minmaxMap) delete [] m_minmaxMap;
  m_minmaxMap = 0;


  if (m_visibilityTex) glDeleteTextures(1, &m_visibilityTex);
  m_visibilityTex = 0;
  if (m_visibilityMap) delete [] m_visibilityMap;
  m_visibilityMap = 0;


  m_tiles.clear();
  m_orderedTiles.clear();

  m_pointClouds.clear();
  m_trisets.clear();

  m_nearDist = 0;
  m_farDist = 1;

  m_saveSnapshots = false;
  m_saveMovie = false;
  m_currFrame = 0;
  m_imageFileName.clear();
  m_imageMode = 0;

  m_vrModeSwitched = false;
}

void
Viewer::genColorMap()
{
  if (m_colorMap)
    return;

  m_colorGrad << Vec(47 ,173,0  )/255.0; 
  m_colorGrad << Vec(80 ,183,20	)/255.0;
  m_colorGrad << Vec(112,194,42	)/255.0;
  m_colorGrad << Vec(143,205,67	)/255.0;
  m_colorGrad << Vec(172,216,94	)/255.0;
  m_colorGrad << Vec(199,227,124)/255.0;
  m_colorGrad << Vec(223,238,156)/255.0;
  m_colorGrad << Vec(253,255,209)/255.0;
  

//  m_colorGrad << Vec(1,102, 94)/255.0f;
//  m_colorGrad << Vec(53,151,143)/255.0f;
//  m_colorGrad << Vec(128,205,193)/255.0f;
//  m_colorGrad << Vec(199,234,229)/255.0f;
//  m_colorGrad << Vec(246,232,195)/255.0f;
//  m_colorGrad << Vec(223,194,125)/255.0f;
//  m_colorGrad << Vec(191,129,45)/255.0f;
//  m_colorGrad << Vec(140,81,10)/255.0f;


  Global::setColorMap(m_colorGrad);


  if (!m_colorMap)
    m_colorMap = new uchar[m_colorGrad.count()*4];


  for(int i=0; i<m_colorGrad.count(); i++)
    {
      m_colorMap[4*i+0] = 255*m_colorGrad[i].x;
      m_colorMap[4*i+1] = 255*m_colorGrad[i].y;
      m_colorMap[4*i+2] = 255*m_colorGrad[i].z;
      m_colorMap[4*i+3] = 255;
    }

  if (!m_colorTex) glGenTextures(1, &m_colorTex);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_1D, m_colorTex);
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(GL_TEXTURE_1D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage1D(GL_TEXTURE_1D,
	       0,
	       GL_RGBA,
	       m_colorGrad.count(),
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_colorMap);  
}

void
Viewer::GlewInit()
{
  GlewInit::initialise();

  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);  

  emit message("Ready");

//  m_vr.initVR();
//
//  if (m_vr.vrEnabled())
//    m_vrMode = true;
//
//  emit message(QString("VRMode %1").arg(m_vr.vrEnabled()));
}

void
Viewer::setVRMode(bool b)
{
  if (b)
    {
      m_vrModeSwitched = true;

      m_moveViewerToCenter = true;

      if (!m_vr.vrEnabled())
	m_vr.initVR();

      if (m_vr.vrEnabled())
	{
	  m_vrMode = true;     
	  emit message("VR mode enabled");
	}
      else
	{
	  QMessageBox::information(0, "", "Cannot enable VR mode");
	  return;
	}
    }
  
  m_vrMode = b;

  if (!m_volume)
    return;

  m_volumeFactory->pushVolume(m_volume);

  if (!m_volume->shiftToZero())
    {
      if (m_vrMode && m_vr.vrEnabled())
	m_volume->setShiftToZero(true);
      else
	{
	  if (m_pointClouds.count() > 1)
	    m_volume->setShiftToZero(false);
	  else
	    m_volume->setShiftToZero(true);
	}

      m_volume->postLoad(false);
    }

  resizeGL(size().width(), size().height());

  start();

  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
			 QPointF(0,0),
			 Qt::LeftButton,
			 Qt::LeftButton,
			 Qt::NoModifier);
  mouseReleaseEvent(&dummyEvent);
}

void
Viewer::start()
{
  m_tiles.clear();
  m_orderedTiles.clear();
  m_pointClouds.clear();
  m_trisets.clear();
  m_loadNodes.clear();

  genColorMap();

  m_volume = m_volumeFactory->topVolume();

  emit switchVolume();


  m_dpv = m_volume->dataPerVertex();
  m_maxTime = m_volume->maxTime();

  m_pointClouds = m_volume->pointClouds();
  m_trisets = m_volume->trisets();

  if (m_vrMode && m_vr.vrEnabled())
    {
      if (m_maxTime > 0)
	m_vr.setShowTimeseriesMenu(true);
      else
	m_vr.setShowTimeseriesMenu(false);
    }

  if (m_pointClouds.count() > 0)
    m_pointType = m_pointClouds[0]->pointType();
  
  createMinMaxTexture();

  m_coordMin = m_volume->coordMin();
  m_coordMax = m_volume->coordMax();
  m_npoints = m_volume->numPoints();

  //--------------------
  QVector3D cmin, cmax;
  cmin = QVector3D(m_coordMin.x,m_coordMin.y,m_coordMin.z);
  cmax = QVector3D(m_coordMax.x,m_coordMax.y,m_coordMax.z);
  if (m_vrMode && m_vr.vrEnabled())
    m_vr.initModel(cmin, cmax);
  //--------------------

  if (m_vrMode && m_vr.vrEnabled())
    {
      if (m_pointClouds.count() > 0)
	{
	  m_vr.setShowMap(m_pointClouds[0]->showMap());
	  m_vr.setGravity(m_pointClouds[0]->gravity());
	  m_vr.setSkybox(m_pointClouds[0]->skybox());
	  m_vr.setPlayButton(m_pointClouds[0]->playButton());
	  m_vr.setGroundHeight(m_pointClouds[0]->groundHeight());
	  m_vr.setTeleportScale(m_pointClouds[0]->teleportScale());
	  
	  //----------------
	  QString dataShown;
	  dataShown = m_pointClouds[0]->name();
	  m_vr.setDataShown(dataShown);
	  //----------------
	}

      m_vr.setShowMap(false);
      m_vr.setSkybox(true);
      m_vr.setPlayButton(true);
    }

  if (!m_shadowShader)
    createShaders();

  if (!m_depthBuffer)
    createFBO();

  setSceneBoundingBox(m_coordMin, m_coordMax);

  if (!m_vertexBuffer[0])
    {
      generateVBOs();

      GLuint vb0 = m_vertexBuffer[0];
      GLuint vb1 = m_vertexBuffer[1];
      emit setVBOs(vb0, vb1);
    }

  if (!m_visibilityTex)
    {
      glGenTextures(1, &m_visibilityTex);
      emit setVisTex(m_visibilityTex);
    }


  //set flying speed some percentage of sceneRadius
  float flyspeed = sceneRadius()*0.001;
  camera()->setFlySpeed(flyspeed);

  camera()->setZNearCoefficient(0.001);
  camera()->setZClippingCoefficient(1);


  m_firstImageDone = 0;
  m_vboLoadedAll = false;

  if (m_vrMode && m_vr.vrEnabled())
    {
      //m_vr.setGenDrawList(true);

      m_menuCam.setScreenWidthAndHeight(m_vr.screenWidth(), m_vr.screenHeight());
      m_menuCam.setSceneBoundingBox(m_coordMin, m_coordMax);
      m_menuCam.setType(Camera::ORTHOGRAPHIC);
      m_menuCam.showEntireScene();

      Global::setScreenSize(m_vr.screenWidth(),
			    m_vr.screenHeight());
      Global::setMenuCam(m_menuCam);
    }

  if (m_volume->validCamera())
    {
      camera()->setPosition(m_volume->getCamPosition());
      camera()->setOrientation(m_volume->getCamOrientation());
      camera()->setSceneCenter(m_volume->getCamSceneCenter());
      camera()->setPivotPoint(m_volume->getCamPivot());
    }
  else
    showEntireScene();

  emit message("Ready to serve");
}

Vec
Viewer::menuCamPos()
{
  return m_menuCam.position();
}

void
Viewer::generateVBOs()
{
  if (m_vertexBuffer[0])glDeleteBuffers(2, m_vertexBuffer);
  if (m_vertexArrayID) glDeleteVertexArrays(1, &m_vertexArrayID);

  glGenVertexArrays(1, &m_vertexArrayID);
  glBindVertexArray(m_vertexArrayID);

  glGenBuffers(2, m_vertexBuffer);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[0]);
  glBufferData(GL_ARRAY_BUFFER,
	       m_dpv*m_pointBudget*sizeof(float),
	       NULL,
	       GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[1]);
  glBufferData(GL_ARRAY_BUFFER,
	       m_dpv*m_pointBudget*sizeof(float),
	       NULL,
	       GL_STATIC_DRAW);
}

void
Viewer::resizeGL(int width, int height)
{
  m_origWidth = width;
  m_origHeight = height;

  QGLViewer::resizeGL(width, height);
  
  createFBO();
}

void
Viewer::createFBO()
{
  if (!GlewInit::initialised())
    return;
  
  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();

  if (m_vrMode && m_vr.vrEnabled())
    {
      wd = m_vr.screenWidth();
      ht = m_vr.screenHeight();

      QGLViewer::resizeGL(wd, ht);
    }

  //------------------
  m_scrGeo[0] = 0;
  m_scrGeo[1] = 0;
  m_scrGeo[2] = wd;
  m_scrGeo[3] = 0;
  m_scrGeo[4] = wd;
  m_scrGeo[5] = ht;
  m_scrGeo[6] = 0;
  m_scrGeo[7] = ht;
  if (m_vertexScreenBuffer) glDeleteBuffers(1, &m_vertexScreenBuffer);
  glGenBuffers(1, &m_vertexScreenBuffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glBufferData(GL_ARRAY_BUFFER,
	       8*sizeof(float),
	       &m_scrGeo,
	       GL_STATIC_DRAW);
  //------------------


  if (m_depthBuffer) glDeleteFramebuffers(1, &m_depthBuffer);
  if (m_depthTex) glDeleteTextures(4, m_depthTex);
  if (m_rbo) glDeleteRenderbuffers(1, &m_rbo);

  glGenFramebuffers(1, &m_depthBuffer);
  glGenTextures(4, m_depthTex);
  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);

  glGenRenderbuffers(1, &m_rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, wd, ht);
  // attach the renderbuffer to depth attachment point
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,      // 1. fbo target: GL_FRAMEBUFFER
			    GL_DEPTH_ATTACHMENT, // 2. attachment point
			    GL_RENDERBUFFER,     // 3. rbo target: GL_RENDERBUFFER
			    m_rbo);              // 4. rbo ID
  glBindRenderbuffer(GL_RENDERBUFFER, 0);

  for(int dt=0; dt<4; dt++)
    {
      glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[dt]);
      glTexImage2D(GL_TEXTURE_RECTANGLE,
		   0,
		   GL_RGBA16F,
		   wd, ht,
		   0,
		   GL_RGBA,
		   GL_UNSIGNED_BYTE,
		   0);
    }
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  //glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
  //glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void
Viewer::
wheelEvent(QWheelEvent *ev)
{
  m_fastDraw = true;
  QGLViewer::wheelEvent(ev);

  genDrawNodeList();
}

void
Viewer::showBox(bool b)
{
  m_showBox = b;
  update();
}

void
Viewer::createShaders()
{ 
  //--------------------------
  if (!m_shadowShader)
    {
      m_shadowShader = glCreateProgramObjectARB();
      if (! ShaderFactory::loadShadersFromFile(m_shadowShader,
					       "assets/shaders/shadow.vert",
					       "assets/shaders/shadow.frag"))
	{
	  m_npoints = 0;
	  return;
	}

      m_shadowParm[0] = glGetUniformLocation(m_shadowShader, "MVP");
      m_shadowParm[1] = glGetUniformLocation(m_shadowShader, "colorTex");
      m_shadowParm[2] = glGetUniformLocation(m_shadowShader, "depthTex");
      m_shadowParm[3] = glGetUniformLocation(m_shadowShader, "showedges");
      m_shadowParm[4] = glGetUniformLocation(m_shadowShader, "softShadows");
      m_shadowParm[5] = glGetUniformLocation(m_shadowShader, "nearDist");
      m_shadowParm[6] = glGetUniformLocation(m_shadowShader, "farDist");
      m_shadowParm[7] = glGetUniformLocation(m_shadowShader, "showsphere");
      m_shadowParm[8] = glGetUniformLocation(m_shadowShader, "copyOnly");
    }
  //--------------------------


  //--------------------------
  if (!m_smoothShader)
    {
      m_smoothShader = glCreateProgramObjectARB();
      if (! ShaderFactory::loadShadersFromFile(m_smoothShader,
					       "assets/shaders/smooth.vert",
					       "assets/shaders/smooth.frag"))
	{
	  m_npoints = 0;
	  return;
	}

      m_smoothParm[0] = glGetUniformLocation(m_smoothShader, "MVP");
      m_smoothParm[1] = glGetUniformLocation(m_smoothShader, "colorTex");
      m_smoothParm[2] = glGetUniformLocation(m_smoothShader, "depthTex");
    }
  //--------------------------


  //--------------------------
  if (!m_blurShader)
    {
      QString VshaderString;
      QString FshaderString;
      VshaderString = ShaderFactory::drawShadowsVertexShader();
      FshaderString = ShaderFactory::genBilateralFilterShaderString();
      m_blurShader = glCreateProgramObjectARB();
      if (! ShaderFactory::loadShader(m_blurShader,
				      VshaderString,
				      FshaderString))
	return;


      m_blurParm[0] = glGetUniformLocation(m_blurShader, "MV");
      m_blurParm[1] = glGetUniformLocation(m_blurShader, "P");
      m_blurParm[2] = glGetUniformLocation(m_blurShader, "depthTex");
    }
  //--------------------------


  //--------------------------
  m_depthShader = glCreateProgramObjectARB();

  if (! ShaderFactory::loadShadersFromFile(m_depthShader,
					   "assets/shaders/drawpoints.vert",
					   "assets/shaders/drawpoints.frag"))
    {
      QMessageBox::information(0, "Error drawpoints shader", "..");
      m_npoints = 0;
      return;
    }

  m_depthParm[0] = glGetUniformLocation(m_depthShader, "MV");
  m_depthParm[1] = glGetUniformLocation(m_depthShader, "MVP");
  m_depthParm[2] = glGetUniformLocation(m_depthShader, "pointSize");
  m_depthParm[3] = glGetUniformLocation(m_depthShader, "eyepos");
  m_depthParm[4] = glGetUniformLocation(m_depthShader, "viewdir");

  m_depthParm[5] = glGetUniformLocation(m_depthShader, "scaleFactor");

  //m_depthParm[6] = glGetUniformLocation(m_depthShader, "MP");

  m_depthParm[7] = glGetUniformLocation(m_depthShader, "colorTex");

  m_depthParm[8] = glGetUniformLocation(m_depthShader, "viewDir");
  m_depthParm[9] = glGetUniformLocation(m_depthShader, "screenWidth");

  m_depthParm[10] = glGetUniformLocation(m_depthShader, "projFactor");

  m_depthParm[11] = glGetUniformLocation(m_depthShader, "ommTex");
  m_depthParm[12] = glGetUniformLocation(m_depthShader, "visTex");

  m_depthParm[13] = glGetUniformLocation(m_depthShader, "pointType");
  m_depthParm[14] = glGetUniformLocation(m_depthShader, "shadows");

  m_depthParm[15] = glGetUniformLocation(m_depthShader, "zFar");

  m_depthParm[16] = glGetUniformLocation(m_depthShader, "minPointSize");
  m_depthParm[17] = glGetUniformLocation(m_depthShader, "maxPointSize");

  m_depthParm[18] = glGetUniformLocation(m_depthShader, "deadRadius");
  m_depthParm[19] = glGetUniformLocation(m_depthShader, "deadPoint");

  m_depthParm[20] = glGetUniformLocation(m_depthShader, "applyXform");
  m_depthParm[21] = glGetUniformLocation(m_depthShader, "xformTileId");
  m_depthParm[22] = glGetUniformLocation(m_depthShader, "xformShift");
  m_depthParm[23] = glGetUniformLocation(m_depthShader, "xformCen");
  m_depthParm[24] = glGetUniformLocation(m_depthShader, "xformScale");
  m_depthParm[25] = glGetUniformLocation(m_depthShader, "xformRot");
  //--------------------------

  //--------------------------
  if (!m_meshShader)
    {
      m_meshShader = glCreateProgramObjectARB();
      if (! ShaderFactory::loadShadersFromFile(m_meshShader,
					       "assets/shaders/mesh.vert",
					       "assets/shaders/mesh.frag"))
	{
	  m_npoints = 0;
	  return;
	}

      m_meshParm[0] = glGetUniformLocation(m_meshShader, "MVP");
      m_meshParm[1] = glGetUniformLocation(m_meshShader, "eyepos");
      m_meshParm[2] = glGetUniformLocation(m_meshShader, "viewDir");
      m_meshParm[3] = glGetUniformLocation(m_meshShader, "ambient");
      m_meshParm[4] = glGetUniformLocation(m_meshShader, "diffuse");
      m_meshParm[5] = glGetUniformLocation(m_meshShader, "specular");
      m_meshParm[6] = glGetUniformLocation(m_meshShader, "shadows");
    }
  //--------------------------


}

void
Viewer::setPointBudget(int b)
{
  qint64 million = 1000000; 
  m_pointBudget = b*million;

  generateVBOs();
  GLuint vb0 = m_vertexBuffer[0];
  GLuint vb1 = m_vertexBuffer[1];
  emit setVBOs(vb0, vb1);	  

  QString assetDir = qApp->applicationDirPath() + QDir::separator() + "assets";
  QString jsonfile = QDir(assetDir).absoluteFilePath("top.json");
  saveTopJson(jsonfile);

  if (m_pointClouds.count() > 0)
    {
      genDrawNodeList();
      update();
    }
}

void
Viewer::setEditMode(bool b)
{
  m_editMode = b;

  m_pointPairs.clear();

  if (m_pointClouds.count() != 2)
    return;

  m_pointClouds[1]->setEditMode(m_editMode);
  
  if (m_editMode)
    {
      m_deltaRot = m_pointClouds[1]->getRotation();
      m_deltaShift = m_pointClouds[1]->getShift();
      m_deltaScale = m_pointClouds[1]->getScale();
    }
  
  genDrawNodeList();
  update();
}

void
Viewer::toggleEditMode()
{
  setEditMode(!m_editMode);
}

void
Viewer::setCamMode(bool b)
{
  if (b) // set orthographic mode
    camera()->setType(Camera::ORTHOGRAPHIC);
  else
    camera()->setType(Camera::PERSPECTIVE);

  updateGL();

}
void
Viewer::toggleCamMode()
{
  if (camera()->type() == Camera::ORTHOGRAPHIC)
    setCamMode(false);
  else
    setCamMode(true);
}

void
Viewer::centerPointClouds()
{
  if (m_pointClouds.count() != 2)
    return;

  emit removeEditedNodes();

  Vec cmin0 = m_pointClouds[0]->tightOctreeMin();
  Vec cmax0 = m_pointClouds[0]->tightOctreeMax();
  Vec cmid0 = (cmax0+cmin0)/2;

  Vec cmin1 = m_pointClouds[1]->tightOctreeMin();
  Vec cmax1 = m_pointClouds[1]->tightOctreeMax();
  Vec cmid1 = (cmax1+cmin1)/2;

  Vec shift = cmid0 - cmid1;

  m_pointClouds[1]->translate(shift);
  
  genDrawNodeList();
}

void
Viewer::undo()
{
  if (m_pointClouds.count() == 2)
    {
      m_pointClouds[1]->undo();
      genDrawNodeList();
    }
}

void
Viewer::saveModInfo()
{
  m_pointClouds[1]->saveModInfo("Time step for modified point cloud", true);
}

void
Viewer::setTimeStep(int ct)
{
  m_currTime = ct;
  m_currTime = qMin(m_maxTime, m_currTime);
  m_currTime = qMax(m_currTime, 0);

  genDrawNodeList();
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{
  if (event->key() == Qt::Key_I)
    {
      if (m_pointClouds.count() != 2)
	return;

      QString mesg;
      Quaternion rot = m_pointClouds[1]->getRotation();
      Vec shift = m_pointClouds[1]->getShift();
      float scale = m_pointClouds[1]->getScale();
      mesg = QString("Shift : %1 %2 %3\nScale : %4\nRotation : %5 %6 %7 %8").\
	arg(shift.x).arg(shift.y).arg(shift.z).\
	arg(scale).\
	arg(rot[0]).arg(rot[1]).arg(rot[2]).arg(rot[3]);
      QMessageBox::information(0, "", mesg.simplified());

      return;
    }

  if (event->key() == Qt::Key_N)
    {      
      if (event->modifiers() & Qt::ShiftModifier)
	m_minNodePixelSize += 50;
      else
	m_minNodePixelSize = qMax(m_minNodePixelSize-10, 10);

      genDrawNodeList();
      update();      
      return;
    }

  if (event->key() == Qt::Key_Space)
    {
      m_flyMode = !m_flyMode;
      toggleCameraMode();
      return;
    }

  if (event->key() == Qt::Key_F)
    {
      m_pointType = !m_pointType;
      update();
      return;
    }  

  if (event->key() == Qt::Key_B)
    {
      m_showBox = !m_showBox;
      update();
      return;
    }  

  if (event->key() == Qt::Key_P)
    {
      m_showPoints = !m_showPoints;

      if (!m_showPoints)
	m_showBox = true;

      update();
      return;
    }  

  if (event->key() == Qt::Key_Question)
    {
      m_showInfo = !m_showInfo;

      update();
      return;
    }  

  if (event->key() == Qt::Key_Right)
    {
      if (m_maxTime > 0)
	{
	  m_currTime = m_currTime+1;
	  if (m_currTime > m_maxTime)
	    m_currTime = 0;
	  emit timeStepChanged(m_currTime);

	  genDrawNodeList();
	  update();
	}
		      
      return;
    }

  if (event->key() == Qt::Key_Left)
    {
      if (m_maxTime > 0)
	{
	  m_currTime = m_currTime-1;
	  if (m_currTime < 0)
	    m_currTime = m_maxTime;
	  emit timeStepChanged(m_currTime);

	  genDrawNodeList();
	  update();
	}

      return;
    }

  if (event->key() == Qt::Key_Up)
    {
      m_pointSize ++;
      update();
      return;
    }  
  
  if (event->key() == Qt::Key_Down)
    {
      m_pointSize = qMax(1, m_pointSize-1);
      update();
      return;
    }  

  QGLViewer::keyPressEvent(event);
}

void
Viewer::mouseDoubleClickEvent(QMouseEvent *event)
{
  if (m_vrMode && m_vr.vrEnabled())
    return;
  
  if (linkClicked(event))
    return;
  
  if (event->buttons() == Qt::LeftButton &&
      !m_flyMode)
    {
      m_selectActive = true;
      updateGL();
      m_selectActive = false;
      bool found = false;
      Vec newp = camera()->pointUnderPixel(event->pos(), found);
      if (found)
	camera()->setPivotPoint(newp);
      setVisualHintsMask(1);
    }
}

void
Viewer::mousePressEvent(QMouseEvent *event)
{ 
  if (m_vrMode && m_vr.vrEnabled())
    return;

  if (m_pointClouds.count() == 0 &&
      m_trisets.count() == 0)
    return;
  
  m_prevPos = event->pos();
  m_deltaRot = Quaternion();
  m_deltaShift = Vec(0,0,0);
  m_deltaScale = 1.0;
  m_deltaChanged = false;
  if (m_pointClouds.count() == 2)
    {
      m_deltaRot = m_pointClouds[1]->getRotation();
      m_deltaShift = m_pointClouds[1]->getShift();
      m_deltaScale = m_pointClouds[1]->getScale();
    }
  m_fastDraw = true;

//
//  if (event->modifiers() & Qt::ShiftModifier &&
//      event->buttons() == Qt::RightButton)
//    {
//      m_selectActive = true;
//      updateGL();
//      m_selectActive = false;
//    }

  //------------------------------------------
  // save point
  if (event->modifiers() & Qt::ShiftModifier &&
      event->buttons() == Qt::LeftButton)
    {
      m_selectActive = true;
      updateGL();
      bool found = false;
      Vec newp = camera()->pointUnderPixel(event->pos(), found);
      if (m_editMode && found)
	{
	  m_pointPairs << newp;
	}
//      if (found)
//	{
//	  if (!m_savePointsToFile)
//	    {
//	      QFileInfo fi(m_volume->filenames()[0]);
//	      QString flnmOut = QFileDialog::getSaveFileName(0,
//							     "Save CSV",
//							     fi.absolutePath(),
//							     "Files (*.csv)",
//							     0,
//							     QFileDialog::DontUseNativeDialog);
//	      
//	      if (!flnmOut.isEmpty())
//		{
//		  m_savePointsToFile = true;
//		  m_pFile.setFileName(flnmOut);
//		}
//	    }
//
//	  if (m_savePointsToFile)
//	    savePointsToFile(newp);
//	}
      m_selectActive = false;
    }
  //------------------------------------------
  // remove point
  if (event->modifiers() & Qt::ShiftModifier &&
      event->buttons() == Qt::RightButton)
    {
      for (int i=0; i<m_pointPairs.count(); i++)
	{
	  Vec pp = camera()->projectedCoordinatesOf(m_pointPairs[i]);
	  if (qAbs(pp.x-event->pos().x()) < 10 &&
	      qAbs(pp.y-event->pos().y()) < 10)
	    {
	      m_pointPairs.removeAt(i);
	      return;
	    }
	}      
    }
  //------------------------------------------

  QGLViewer::mousePressEvent(event);

  //if (m_flyMode)
  m_flyTime.restart();
}

void
Viewer::mouseReleaseEvent(QMouseEvent *event)
{
  if (m_pointClouds.count() == 0 &&
      m_trisets.count() == 0)
    return;

  if (m_vrMode && m_vr.vrEnabled())
    {
      genDrawNodeListForVR();
      return;
    }
  
  m_fastDraw = false;

  // generate new node list only when mouse released
  if (m_editMode && m_deltaChanged)
    {
      Vec xformcen = m_pointClouds[1]->getXformCen();
      m_pointClouds[1]->setXform(m_deltaScale,m_deltaShift,m_deltaRot, xformcen);

      m_deltaRot = Quaternion();
      m_deltaShift = Vec(0,0,0);
      m_deltaScale = 1.0;      
      m_deltaChanged = false;
    }

  QGLViewer::mouseReleaseEvent(event);

  if (m_pointClouds.count() > 0)
    genDrawNodeList();
}

void
Viewer::mouseMoveEvent(QMouseEvent *event)
{
  if (m_vrMode && m_vr.vrEnabled())
    return;

  if (m_pointClouds.count() == 0 &&
      m_trisets.count() == 0)
    return;

  if (m_editMode &&
      m_pointClouds.count() == 2 &&
      event->modifiers() & Qt::ControlModifier &&
      event->buttons())
    {
      m_deltaChanged = true;
      QPoint delta = event->pos() - m_prevPos;

      if (event->buttons() == Qt::LeftButton)
	rotatePointCloud(event, delta);
      if (event->buttons() == Qt::RightButton)
	movePointCloud(delta);
      else if (event->buttons() == Qt::MiddleButton)
	scalePointCloud(delta);

      m_prevPos = event->pos();
      updateGL();
    }
  else
    QGLViewer::mouseMoveEvent(event);

//  if (!m_flyMode && event->buttons())
//    {
//      int flytime = m_flyTime.elapsed();
//      if (flytime > 500) // generate draw list every half sec in examine mode
//	{
//	  m_flyTime.restart();
//	  genDrawNodeList();
//	}
//    }
}

void
Viewer::rotatePointCloud(QMouseEvent *event, QPoint delta)
{
  Vec axis;
  float angle;

  
  Vec cminO = m_pointClouds[1]->tightOctreeMinO();
  Vec cmaxO = m_pointClouds[1]->tightOctreeMaxO();
  Vec cmidO = (cmaxO+cminO)*0.5;	      
  m_pointClouds[1]->setXformCen(cmidO);

  if (event->modifiers() & Qt::AltModifier)
    {
      Vec cmin = m_pointClouds[1]->tightOctreeMin();
      Vec cmax = m_pointClouds[1]->tightOctreeMax();
      Vec cmid = (cmax+cmin)/2;

      Vec trans = camera()->projectedCoordinatesOf(cmid);

      float prev_angle = atan2(m_prevPos.y()-trans[1], m_prevPos.x()-trans[0]);
      angle = atan2(event->y()-trans[1], event->x()-trans[0]);
      axis = camera()->viewDirection();
      angle = angle-prev_angle;
    }
  else
    {
      Vec upVec = camera()->upVector();
      Vec rtVec = camera()->rightVector();
      
      axis = delta.y()*rtVec + delta.x()*upVec;
      angle = qSqrt(delta.x()*delta.x() + 
		    delta.y()*delta.y());
      angle = qDegreesToRadians(angle);
    }

  Quaternion q(axis, angle);

  m_deltaRot = q * m_deltaRot;
}

void
Viewer::movePointCloud(QPoint delta)
{
  Vec cmin = m_pointClouds[0]->tightOctreeMin();
  Vec cmax = m_pointClouds[0]->tightOctreeMax();
  Vec cmid = (cmax+cmin)/2;

  Vec scrPt = camera()->projectedCoordinatesOf(cmid);
  Vec moveScrPt = scrPt + Vec(delta.x(), delta.y(), 0);
  Vec move = camera()->unprojectedCoordinatesOf(moveScrPt) - cmid;

  move *= 0.5; // reduce the movement for finer control

  m_deltaShift += move;
}

void
Viewer::scalePointCloud(QPoint delta)
{
  if (qAbs(delta.x()) > qAbs(delta.y()))
    m_deltaScale += 0.005*delta.x();
  else
    m_deltaScale += 0.005*delta.y();
}


void
Viewer::dummydraw()
{
  glClearColor(0.2,0.2,0.2,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawAABB();
}

void
Viewer::paintGL()
{
  glClearColor(0,0,0,0);

  if (!m_vrMode ||
      !m_vr.vrEnabled() ||
      (m_pointClouds.count() == 0 &&
       m_trisets.count() == 0) )
    {
      // Clears screen, set model view matrix...
      preDraw();
      // Used defined method. Default calls draw()
      if (camera()->frame()->isManipulated())
	fastDraw();
      else
	draw();
      // Add visual hints: axis, camera, grid...
      postDraw();
      return;
    }

  if (!m_vr.preDraw())
    {
      update();
      return;
    }

  if (m_vr.genDrawList())
    {
      if (m_vr.nextStep() != 0)
	{
	  m_currTimeChanged = true;
	  m_currTime = m_currTime + m_vr.nextStep();
	  if (m_currTime > m_maxTime) m_currTime = 0;
	  if (m_currTime < 0) m_currTime = m_maxTime;	  
	  m_vr.resetNextStep();

	  m_vr.setTimeStep(QString("%1").arg(m_currTime));

	  if (m_pointClouds.count() > 0)
	    {
	      //----------------
	      QString dataShown;
	      for(int d=0; d<m_pointClouds.count(); d++)
		if (m_pointClouds[d]->visible())
		  dataShown = dataShown + " " + m_pointClouds[d]->name();
	      
	      m_vr.setDataShown(dataShown);
	      //----------------
	      
	      
	      if (m_pointClouds[0]->showMap())
		m_firstImageDone = 0;
	    }
	}

      m_vr.resetGenDrawList();
      genDrawNodeListForVR();
    }
  
  if (m_npoints <= 0)
    return;
  
  //---------------------------
  m_hmdPos = m_vr.hmdPosition();

  m_hmdVD = -m_vr.viewDir();
  m_hmdUP = -m_vr.upDir();
  m_hmdRV = -m_vr.rightDir();


  if (m_pointClouds.count() > 0)
    {
      if (m_pointClouds[0]->showMap())
	m_vr.sendCurrPosToMenu();
    }

  //---------------------------

  if (m_vbPoints == 0 && m_trisets.count() == 0)
    {
      // just for evaluation of projection matrices
      m_vr.viewProjection(vr::Eye_Left);
      m_vr.viewProjection(vr::Eye_Right);

      m_vr.bindLeftBuffer();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_vr.postDrawLeftBuffer();

      m_vr.bindRightBuffer();
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      m_vr.postDrawRightBuffer();
    }
  else
    {
      
      // left eye
      m_vr.bindLeftBuffer();

      if (m_trisets.count() > 0)
	drawTrisets(vr::Eye_Left);
		    
      if (m_pointClouds.count() > 0)
	{
	  drawPointsWithShadows(vr::Eye_Left);
	  drawLabelsForVR(vr::Eye_Left);
	}
      
      m_vr.postDrawLeftBuffer();
      
      
      // right eye
      m_vr.bindRightBuffer();

      if (m_trisets.count() > 0)
	drawTrisets(vr::Eye_Right);
		    
      if (m_pointClouds.count() > 0)
	{
	  drawPointsWithShadows(vr::Eye_Right);
	  drawLabelsForVR(vr::Eye_Right);
	}
      m_vr.postDrawRightBuffer();
    }

  m_vr.postDraw();

  m_frames++;
  
  //---------------------------
  if (m_pointClouds.count() > 0)
    {
      if (m_vr.reUpdateMap())
	m_firstImageDone = 0;
      
      
      if (m_vboLoadedAll &&
	  m_pointClouds[0]->showMap()&&
	  m_firstImageDone < 2)
	{
	  generateFirstImage();
	  m_firstImageDone++;
	  
	  m_vr.resetUpdateMap();
	  
	  //--------------
	  if (m_moveViewerToCenter && m_firstImageDone == 2) // do it only once
	    {
	      m_vr.resetModel();
	      m_moveViewerToCenter = false;
	    }
	  //--------------
	  
	  //----------------
	  QString dataShown;
	  for(int d=0; d<m_pointClouds.count(); d++)
	    if (m_pointClouds[d]->visible())
	      dataShown = dataShown + " " + m_pointClouds[d]->name();
	  
	  m_vr.setDataShown(dataShown);
	  //----------------
	}  
    }
  //---------------------------

  update();
}

void
Viewer::captureKeyFrameImage(int kfn)
{
  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit replaceKeyFrameImage(kfn, image);
}

void
Viewer::draw()
{
  glClearColor(0.2,0.2,0.2,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_npoints <= 0)
    return;

  if (m_fastDraw)
    {
      fastDraw();
      m_fastDraw = false;
      return;
    }
  
  if (m_trisets.count() > 0)
    drawTrisets();

  if (m_pointClouds.count() > 0)
    {
      drawPointsWithReload();

      drawLabels();
  
      if (m_showBox || m_editMode)
	drawAABB();

      if (m_editMode)
	drawPointPairs();
    }

  drawInfo();

  if (Global::playFrames() && (m_vboLoadedAll || m_pointClouds.count() == 0))
    {
      if (m_saveSnapshots)
	saveImage();
      else if (m_saveMovie)
	saveMovie();

      if (Global::currentKeyFrame() > -1)
	{
	  captureKeyFrameImage(Global::currentKeyFrame());
	  emit nextKeyFrame();
	}
      else
	emit nextFrame();
    }
}

void
Viewer::fastDraw()
{
  glClearColor(0.2,0.2,0.2,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_npoints <= 0)
    return;

  if (m_trisets.count() > 0)
    drawTrisets();

  if (m_pointClouds.count() > 0)
    {
      drawPointsWithReload();
  
      drawLabels();

      if (m_showBox || m_editMode)
	drawAABB();

      if (m_editMode)
	drawPointPairs();
    }
  
  drawInfo();


  if (m_flyMode)
    {
      int flytime = m_flyTime.elapsed();
      if (flytime > 5000) // generate draw list every sec in fly mode
	{
	  m_flyTime.restart();
	  genDrawNodeList();
	}
    }

  update();
}


void
Viewer::drawInfo()
{
  if (m_saveSnapshots ||
      m_saveMovie)
    return;

  if (!m_showInfo &&
      m_volumeFactory->stackSize() == 0)
    return;
  

  glDisable(GL_DEPTH_TEST);

  QString mesg;

  QFont tfont = QFont("Helvetica", 10);
  tfont.setStyleStrategy(QFont::PreferAntialias);  
  
  startScreenCoordinatesSystem();
  
  if (m_showInfo)
    {
      if (m_maxTime > 0)
	mesg += QString("Time : %1    ").arg(m_currTime);
      
      //mesg += QString("Fps : %1    ").arg(currentFPS());
      //int ptsz = m_pointSize;
      //mesg += QString("PtSz : %1    ").arg(ptsz);
      if (m_pointClouds.count() > 0)
	{
	  mesg += QString("Points : %1 (%2)  ").arg(m_pointBudget).arg(m_vbPoints);
	  mesg += QString("%1").arg(m_vboLoadedAll);
	}
	    
      StaticFunctions::renderText(10, 30,
				  mesg, tfont,
				  Qt::black,
				  Qt::white,
				  true);
    }

  if (m_volumeFactory->stackSize() > 0)
    {
      tfont.setPointSize(20);
      StaticFunctions::renderText(m_backButtonPos.x(), m_backButtonPos.y(),
				  "BACK", tfont,
				  Qt::black,
				  QColor(128, 200, 255),
				  true);
    }

  stopScreenCoordinatesSystem();

  glEnable(GL_DEPTH_TEST);
}

void
Viewer::drawVAO()
{
  glBindVertexArray(m_vertexArrayID);


  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[m_vbID]);  

  if (m_dpv < 5)
    {
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,  // attribute 0
			    m_dpv,  // size
			    GL_FLOAT, // type
			    GL_FALSE, // normalized
			    0, // stride
			    (void*)0 ); // array buffer offset

      glDrawArrays(GL_POINTS, 0, m_vbPoints);  
      
      glDisableVertexAttribArray(0);
    }

  if (m_dpv == 6) // explicit color
    {
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0,  // attribute 0
			    3,  // size
			    GL_FLOAT, // type
			    GL_FALSE, // normalized
			    20, // stride
			    (void*)0 ); // array buffer offset
//      glVertexAttribPointer(0,  // attribute 0
//			    3,  // size
//			    GL_FLOAT, // type
//			    GL_FALSE, // normalized
//			    16, // stride
//			    (void*)0 ); // array buffer offset

      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1,  // attribute 1
			    4,  // size
			    GL_UNSIGNED_SHORT, // type
			    GL_FALSE, // normalized
			    20, // stride
			    (char *)NULL+12 ); // array buffer offset
//      glVertexAttribPointer(1,  // attribute 1
//			    4,  // size
//			    GL_UNSIGNED_BYTE, // type
//			    GL_FALSE, // normalized
//			    16, // stride
//			    (char *)NULL+12 ); // array buffer offset

      glDrawArrays(GL_POINTS, 0, m_vbPoints);  

      glDisableVertexAttribArray(0);
      glDisableVertexAttribArray(1);
    }

  //glFinish();
}

void
Viewer::vboLoaded(int cvp, qint64 npts)
{
  m_vbID = cvp;
  m_vbPoints = npts;
  m_vboLoadedAll = false;

  if (!m_vrMode ||
      !m_vr.vrEnabled())
    {
      update();
      return;
    }

}
void
Viewer::vboLoadedAll(int cvp, qint64 npts)
{
  if (npts > -1)
    {
      m_vbID = cvp;
      m_vbPoints = npts;
    }
  
  m_vboLoadedAll = true;

  if (!m_vrMode ||
      !m_vr.vrEnabled())
    {
      update();
      return;
    }

  if (m_vr.play())
    m_vr.takeNextStep();

  //m_vboLoadedAll = true;
}

void
Viewer::generateFirstImage()
{
  //-------------------
    m_vr.bindMapBuffer();
  //-------------------

  glEnable(GL_DEPTH_TEST);

  //--------------------
  int wd = m_vr.screenWidth();
  int ht = m_vr.screenHeight();

  //float fov = qDegreesToRadians(110.0); // FOV is 110 degrees for HTC Vive
  float fov = m_menuCam.fieldOfView(); // FOV is 110 degrees for HTC Vive
  float slope = qTan(fov/2);
  float projFactor = (0.5*ht)/slope;
  //float scaleFactor = m_vr.scaleFactor();
  float scaleFactor = 1;
  //--------------------

  glPointSize(1.0);
  glEnable(GL_PROGRAM_POINT_SIZE );

  float m[16];

  // model-view matrix
  m_menuCam.getModelViewMatrix(m);
  GLfloat mv[16];
  for(int i=0; i<16; i++) mv[i] = m[i];

  // model-view-projection matrix
  m_menuCam.getModelViewProjectionMatrix(m);
  GLfloat mvp[16];
  for(int i=0; i<16; i++) mvp[i] = m[i];

  
  glEnable(GL_POINT_SPRITE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_colorTex); // colors

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_minmaxTex); // tile min max

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_visibilityTex); // octree visibility

//--------------------------------------------
  // No Shadows
  glUseProgram(m_depthShader);

  // currently using fixed pointsize only
  glUniformMatrix4fv(m_depthParm[0], 1, GL_FALSE, mv);

  glUniformMatrix4fv(m_depthParm[1], 1, GL_FALSE, mvp);

  //glUniform1f(m_depthParm[2], m_vr.pointSize());
  glUniform1f(m_depthParm[2], 2);


  Vec eyepos = Vec(m_hmdPos.x(),m_hmdPos.y(),m_hmdPos.z());
  Vec viewDir = Vec(m_hmdVD.x(),m_hmdVD.y(),m_hmdVD.z());

  glUniform3f(m_depthParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_depthParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_depthParm[5], scaleFactor);

  glUniform1i(m_depthParm[7], 0); // color texture

  glUniform3f(m_depthParm[8], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform1i(m_depthParm[9], wd); // screenWidth

  glUniform1f(m_depthParm[10], projFactor);

  glUniform1i(m_depthParm[11], 2); // tile min max
  glUniform1i(m_depthParm[12], 3); // octree visibility

  glUniform1i(m_depthParm[13], 1); // adaptive pointsize
  
  glUniform1i(m_depthParm[14], 0); // no shadows
  
  glUniform1f(m_depthParm[15], camera()->zFar()); // zfar
  
  glUniform1f(m_depthParm[16], 1); // min point size
  glUniform1f(m_depthParm[17], 15); // max point size

  glUniform1f(m_depthParm[18], -1); // deadRadius
  glUniform3f(m_depthParm[19], -1000, -1000, -1000); // deadPoint

  glUniform1i(m_depthParm[20], 0); // applyXform


  drawVAO();
  
  
  glUseProgram(0);

  glDisable(GL_PROGRAM_POINT_SIZE );
  glDisable(GL_POINT_SPRITE);
  
  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_1D);

  //----------
  m_vr.menuImageFromMapBuffer();
  //----------

  stickLabelsToGround();
}

//--------------------------------------------
// Rendering points with shadows
//--------------------------------------------
void
Viewer::drawPointsWithShadows(vr::Hmd_Eye eye)
{
  if (!m_showPoints || m_vbPoints <= 0)
    return;

  glEnable(GL_DEPTH_TEST);


  int wd = m_vr.screenWidth();
  int ht = m_vr.screenHeight();

  float fov = qDegreesToRadians(110.0); // FOV is 110 degrees for HTC Vive
  float slope = qTan(fov/2);
  float projFactor = (0.5*ht)/slope;
  float scaleFactor = m_vr.scaleFactor();
  //--------------------

  glEnable(GL_PROGRAM_POINT_SIZE );

  // projection matrix
  //QMatrix4x4 mp = m_vr.currentViewProjection(eye);

  QMatrix4x4 mvp = m_vr.viewProjection(eye);

  QMatrix4x4 mv = m_vr.modelView(eye);

  
  glEnable(GL_POINT_SPRITE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_colorTex); // colors

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_minmaxTex); // tile min max

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_visibilityTex); // octree visibility
  

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[m_vbID]);


  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[0],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT1,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[1],
			 0);
  GLenum buffers[2] = { GL_COLOR_ATTACHMENT0_EXT,
			GL_COLOR_ATTACHMENT1_EXT };
  glDrawBuffersARB(2, buffers);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(m_depthShader);

  glUniformMatrix4fv(m_depthParm[0], 1, GL_FALSE, mv.data());

  glUniformMatrix4fv(m_depthParm[1], 1, GL_FALSE, mvp.data());


  glUniform1f(m_depthParm[2], m_vr.pointSize());

  Vec eyepos = Vec(m_hmdPos.x(),m_hmdPos.y(),m_hmdPos.z());
  Vec viewDir = Vec(m_hmdVD.x(),m_hmdVD.y(),m_hmdVD.z());
  Vec rightVec = Vec(m_hmdRV.x(),m_hmdRV.y(),m_hmdRV.z());


  glUniform3f(m_depthParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_depthParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_depthParm[5], scaleFactor);

  glUniform1i(m_depthParm[7], 0); // color texture

  glUniform3f(m_depthParm[8], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform1i(m_depthParm[9], wd); // screenWidth

  glUniform1f(m_depthParm[10], projFactor);

  glUniform1i(m_depthParm[11], 2); // tile min max
  glUniform1i(m_depthParm[12], 3); // octree visibility

  glUniform1i(m_depthParm[13], m_pointType); // pointsize calculation

  glUniform1i(m_depthParm[14], 1); // shadows
  
  glUniform1f(m_depthParm[15], camera()->zFar()); // zfar

  glUniform1f(m_depthParm[16], 2); // max point size
  glUniform1f(m_depthParm[17], 20); // max point size

  glUniform1f(m_depthParm[18], m_vr.deadRadius());
  QVector3D deadPt = m_vr.deadPoint();
  glUniform3f(m_depthParm[19], deadPt.x(), deadPt.y(), deadPt.z());


  glUniform1i(m_depthParm[20], 0); // applyXform


  drawVAO();

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_1D);

  glUseProgram(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

//--------------------------------------------

////--------------------------------------------
//  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
//  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
//			 GL_COLOR_ATTACHMENT0,
//			 GL_TEXTURE_RECTANGLE,
//			 m_depthTex[2],
//			 0);
//  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
//			 GL_COLOR_ATTACHMENT1,
//			 GL_TEXTURE_RECTANGLE,
//			 m_depthTex[3],
//			 0);
//  glDrawBuffersARB(2, buffers);
//
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//  glUseProgram(m_smoothShader);
//
//  glActiveTexture(GL_TEXTURE0);
//  glEnable(GL_TEXTURE_RECTANGLE);
//  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
//
//  glActiveTexture(GL_TEXTURE1);
//  glEnable(GL_TEXTURE_RECTANGLE);
//  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth
//
//
//  mvp.setToIdentity();
//  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
//
//  glUniformMatrix4fv(m_smoothParm[0], 1, GL_FALSE, mvp.data());
//
//  glUniform1i(m_smoothParm[1], 0); // colors
//  glUniform1i(m_smoothParm[2], 1); // depthTex1
//
//  glEnableVertexAttribArray(0);
//  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
//  glVertexAttribPointer(0,  // attribute 0
//			2,  // size
//			GL_FLOAT, // type
//			GL_FALSE, // normalized
//			0, // stride
//			(void*)0 ); // array buffer offset
//  glDrawArrays(GL_QUADS, 0, 8);
//
//  glDisableVertexAttribArray(0);
//
//  glActiveTexture(GL_TEXTURE1);
//  glDisable(GL_TEXTURE_RECTANGLE);
//
//  glActiveTexture(GL_TEXTURE0);
//  glDisable(GL_TEXTURE_RECTANGLE);
//
//  glBindFramebuffer(GL_FRAMEBUFFER, 0);
////--------------------------------------------


//--------------------------------------------
//--------------------------------------------
  m_vr.bindBuffer(eye);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


//--------------------------------------------
  glUseProgram(m_shadowShader);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth


  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);

  glUniformMatrix4fv(m_shadowParm[0], 1, GL_FALSE, mvp.data());

  glUniform1i(m_shadowParm[1], 0); // colors
  glUniform1i(m_shadowParm[2], 1); // depthTex1

  glUniform1i(m_shadowParm[3], m_vr.edges()); // showedges
  glUniform1i(m_shadowParm[4], m_vr.softShadows()); // softShadows

  glUniform1f(m_shadowParm[5], 1);
  glUniform1f(m_shadowParm[6], 0);

  glUniform1i(m_shadowParm[7], m_vr.spheres());

  glUniform1i(m_shadowParm[8], false); // copyOnly false

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glVertexAttribPointer(0,  // attribute 0
			2,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			0, // stride
			(void*)0 ); // array buffer offset
  glDrawArrays(GL_QUADS, 0, 8);

  glDisableVertexAttribArray(0);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_RECTANGLE);

//--------------------------------------------
//--------------------------------------------

  glUseProgram(0);

  glDisable(GL_PROGRAM_POINT_SIZE );
  glDisable(GL_POINT_SPRITE);
  //glDisable(GL_TEXTURE_2D);
}

void
Viewer::drawPoints(vr::Hmd_Eye eye)
{
  if (!m_showPoints || m_vbPoints <= 0)
    return;

  glEnable(GL_DEPTH_TEST);

  //--------------------
  int wd = m_vr.screenWidth();
  int ht = m_vr.screenHeight();

  float fov = qDegreesToRadians(110.0); // FOV is 110 degrees for HTC Vive
  float slope = qTan(fov/2);
  float projFactor = (0.5*ht)/slope;
  float scaleFactor = m_vr.scaleFactor();
  //--------------------

  glPointSize(1.0);


  glEnable(GL_PROGRAM_POINT_SIZE );

//  // projection matrix
//  QMatrix4x4 mp = m_vr.currentViewProjection(eye);

  // model-view matrix
  QMatrix4x4 mv = m_vr.modelView(eye);

  // model-view-projection matrix
  QMatrix4x4 mvp = m_vr.viewProjection(eye);

  
  glEnable(GL_POINT_SPRITE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_colorTex); // colors

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_minmaxTex); // tile min max

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_visibilityTex); // octree visibility

//--------------------------------------------
  // No Shadows
  glUseProgram(m_depthShader);

  // currently using fixed pointsize only
  glUniformMatrix4fv(m_depthParm[0], 1, GL_FALSE, mv.data());

  glUniformMatrix4fv(m_depthParm[1], 1, GL_FALSE, mvp.data());

  //glUniformMatrix4fv(m_depthParm[6], 1, GL_FALSE, mp.data());

  glUniform1f(m_depthParm[2], m_vr.pointSize());


  Vec eyepos = Vec(m_hmdPos.x(),m_hmdPos.y(),m_hmdPos.z());
  Vec viewDir = Vec(m_hmdVD.x(),m_hmdVD.y(),m_hmdVD.z());
  Vec rightVec = Vec(m_hmdRV.x(),m_hmdRV.y(),m_hmdRV.z());

  glUniform3f(m_depthParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_depthParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_depthParm[5], scaleFactor);

  glUniform1i(m_depthParm[7], 0); // color texture

  glUniform3f(m_depthParm[8], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform1i(m_depthParm[9], wd); // screenWidth

  glUniform1f(m_depthParm[10], projFactor);

  glUniform1i(m_depthParm[11], 2); // tile min max
  glUniform1i(m_depthParm[12], 3); // octree visibility

  glUniform1i(m_depthParm[13], m_pointType); // pointsize calculation
  
  glUniform1i(m_depthParm[14], 0); // no shadows
  
  glUniform1f(m_depthParm[15], camera()->zFar()); // zfar
  
  glUniform1f(m_depthParm[16], 2); // max point size
  glUniform1f(m_depthParm[17], 20); // max point size

  glUniform1f(m_depthParm[18], m_vr.deadRadius());
  QVector3D deadPt = m_vr.deadPoint();
  glUniform3f(m_depthParm[19], deadPt.x(), deadPt.y(), deadPt.z());


  glUniform1i(m_depthParm[20], 0); // applyXform


  drawVAO();
  
  
  glUseProgram(0);

  glDisable(GL_PROGRAM_POINT_SIZE );
  glDisable(GL_POINT_SPRITE);
  
  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_1D);
}

void
Viewer::drawTrisets(vr::Hmd_Eye eye)
{
  glEnable(GL_DEPTH_TEST);

  // model-view-projection matrix
  QMatrix4x4 mvp = m_vr.viewProjection(eye);
  Vec eyepos = Vec(m_hmdPos.x(),m_hmdPos.y(),m_hmdPos.z());
  Vec viewDir = Vec(m_hmdVD.x(),m_hmdVD.y(),m_hmdVD.z());

  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT0,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[0],
			 0);
  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			 GL_COLOR_ATTACHMENT1,
			 GL_TEXTURE_RECTANGLE,
			 m_depthTex[1],
			 0);
  GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT,
		       GL_COLOR_ATTACHMENT1_EXT };
  glDrawBuffers(2, buffers);

  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(m_meshShader);

  glUniformMatrix4fv(m_meshParm[0], 1, GL_FALSE, mvp.data());

  glUniform3f(m_meshParm[1], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_meshParm[2], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_meshParm[3], 0.9); // ambient
  glUniform1f(m_meshParm[4], 0.1); // diffuse
  glUniform1f(m_meshParm[5], 1.0); // specular

  glUniform1i(m_meshParm[6], 1); // shadows

  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->time() == -1 ||
	  m_trisets[i]->time() == m_currTime)
	m_trisets[i]->draw();
    }
  
  glUseProgram(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

//--------------------------------------------
//--------------------------------------------

  m_vr.bindBuffer(eye);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//--------------------------------------------
//--------------------------------------------
  glUseProgram(m_shadowShader);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth


  int wd = m_vr.screenWidth();
  int ht = m_vr.screenHeight();
  mvp.setToIdentity();
  mvp.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);

  glUniformMatrix4fv(m_shadowParm[0], 1, GL_FALSE, mvp.data());

  glUniform1i(m_shadowParm[1], 0); // colors
  glUniform1i(m_shadowParm[2], 1); // depthTex1

  glUniform1i(m_shadowParm[3], m_vr.edges()); // showedges
  glUniform1i(m_shadowParm[4], m_vr.softShadows()); // softShadows

  glUniform1f(m_shadowParm[5], 1);
  glUniform1f(m_shadowParm[6], 0);

  glUniform1i(m_shadowParm[7], 0);

  glUniform1i(m_shadowParm[8], false); // copyOnly false

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glVertexAttribPointer(0,  // attribute 0
			2,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			0, // stride
			(void*)0 ); // array buffer offset
  glDrawArrays(GL_QUADS, 0, 8);

  glDisableVertexAttribArray(0);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_RECTANGLE);
//--------------------------------------------
//--------------------------------------------

  glUseProgram(0);
}

void
Viewer::drawTrisets()
{
  glEnable(GL_DEPTH_TEST);

  // model-view-projection matrix
  GLdouble m[16];
  camera()->getModelViewProjectionMatrix(m);
  GLfloat mvp[16];
  for(int i=0; i<16; i++) mvp[i] = m[i];

  Vec eyepos = camera()->position();
  Vec viewDir = camera()->viewDirection();

  if (!m_selectActive)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[0],
			     0);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT1,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[1],
			     0);
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT,
			   GL_COLOR_ATTACHMENT1_EXT };
      glDrawBuffers(2, buffers);
    }

  
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(m_meshShader);

  glUniformMatrix4fv(m_meshParm[0], 1, GL_FALSE, mvp);

  glUniform3f(m_meshParm[1], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_meshParm[2], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_meshParm[3], 0.9); // ambient
  glUniform1f(m_meshParm[4], 0.1); // diffuse
  glUniform1f(m_meshParm[5], 1.0); // specular

  if (m_selectActive)
    glUniform1i(m_meshParm[6], 0); // no shadows
  else
    glUniform1i(m_meshParm[6], 1); // shadows

  for(int i=0; i<m_trisets.count(); i++)
    {
      if (m_trisets[i]->time() == -1 ||
	  m_trisets[i]->time() == m_currTime)
	m_trisets[i]->draw();
    }
  
  glUseProgram(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (m_selectActive)
    return;
//--------------------------------------------
//--------------------------------------------

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

//--------------------------------------------
//--------------------------------------------
  glUseProgram(m_shadowShader);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth


  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();

  QMatrix4x4 mvp4x4;
  mvp4x4.setToIdentity();
  mvp4x4.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);

  glUniformMatrix4fv(m_shadowParm[0], 1, GL_FALSE, mvp4x4.data());

  glUniform1i(m_shadowParm[1], 0); // colors
  glUniform1i(m_shadowParm[2], 1); // depthTex1

  glUniform1i(m_shadowParm[3], m_showEdges); // showedges
  glUniform1i(m_shadowParm[4], m_softShadows); // softShadows

  glUniform1f(m_shadowParm[5], m_nearDist);
  glUniform1f(m_shadowParm[6], m_farDist);

  glUniform1i(m_shadowParm[7], 0);

  glUniform1i(m_shadowParm[8], false); // copyOnly false

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glVertexAttribPointer(0,  // attribute 0
			2,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			0, // stride
			(void*)0 ); // array buffer offset
  glDrawArrays(GL_QUADS, 0, 8);

  glDisableVertexAttribArray(0);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_RECTANGLE);
//--------------------------------------------
//--------------------------------------------

  glUseProgram(0);
}


void
Viewer::drawPointsWithReload()
{
  if (!m_showPoints || m_vbPoints <= 0)
    return;

  glEnable(GL_DEPTH_TEST);


  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();

  float fov = camera()->fieldOfView();
  float slope = qTan(fov/2);
  float projFactor = (0.5*ht)/slope;
  float scaleFactor = m_vr.scaleFactor();
  //--------------------

  glPointSize(1.0);
  glEnable(GL_PROGRAM_POINT_SIZE );

  GLdouble m[16];

  // model-view matrix
  camera()->getModelViewMatrix(m);
  GLfloat mv[16];
  for(int i=0; i<16; i++) mv[i] = m[i];

  // model-view-projection matrix
  camera()->getModelViewProjectionMatrix(m);
  GLfloat mvp[16];
  for(int i=0; i<16; i++) mvp[i] = m[i];


  
  glEnable(GL_POINT_SPRITE);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_1D);
  glBindTexture(GL_TEXTURE_1D, m_colorTex); // colors

  glActiveTexture(GL_TEXTURE2);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_minmaxTex); // tile min max

  glActiveTexture(GL_TEXTURE3);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_visibilityTex); // octree visibility
  

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[m_vbID]);


  if (!m_selectActive)
    {
      glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT0,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[0],
			     0);
      glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
			     GL_COLOR_ATTACHMENT1,
			     GL_TEXTURE_RECTANGLE,
			     m_depthTex[1],
			     0);
      GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT,
			   GL_COLOR_ATTACHMENT1_EXT };
      glDrawBuffers(2, buffers);
    }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(m_depthShader);

  glUniformMatrix4fv(m_depthParm[0], 1, GL_FALSE, mv);

  glUniformMatrix4fv(m_depthParm[1], 1, GL_FALSE, mvp);

  glUniform1f(m_depthParm[2], m_pointSize);

  Vec eyepos = camera()->position();
  Vec viewDir = camera()->viewDirection();


  glUniform3f(m_depthParm[3], eyepos.x, eyepos.y, eyepos.z); // eyepos
  glUniform3f(m_depthParm[4], viewDir.x, viewDir.y, viewDir.z); // viewDir

  glUniform1f(m_depthParm[5], scaleFactor);

  glUniform1i(m_depthParm[7], 0); // color texture

  glUniform3f(m_depthParm[8], viewDir.x, viewDir.y, viewDir.z); // viewDir
  glUniform1i(m_depthParm[9], wd); // screenWidth

  glUniform1f(m_depthParm[10], projFactor);

  glUniform1i(m_depthParm[11], 2); // tile min max
  glUniform1i(m_depthParm[12], 3); // octree visibility

  glUniform1i(m_depthParm[13], m_pointType); // pointsize calculation

  if (m_selectActive)
    glUniform1i(m_depthParm[14], 0); // no shadows
  else
    glUniform1i(m_depthParm[14], 1); // shadows
  
  glUniform1f(m_depthParm[15], camera()->zFar()); // zfar

  glUniform1f(m_depthParm[16], 2); // max point size
  glUniform1f(m_depthParm[17], 20); // max point size

  glUniform1f(m_depthParm[18], -1); // deadRadius
  glUniform3f(m_depthParm[19], -1000, -1000, -1000); // deadPoint


  glUniform1i(m_depthParm[20], m_editMode); // applyXform
  if (m_editMode)
    {
      Vec shift = m_deltaShift - m_pointClouds[1]->globalMin();      
      Vec cmid = m_pointClouds[1]->getXformCen();
      glUniform1i(m_depthParm[21], m_volume->xformTileId());
      glUniform3f(m_depthParm[22], shift.x, shift.y, shift.z);
      glUniform3f(m_depthParm[23], cmid.x, cmid.y, cmid.z);
      glUniform1f(m_depthParm[24], m_deltaScale);
      glUniform4f(m_depthParm[25], m_deltaRot[0], m_deltaRot[1], m_deltaRot[2], m_deltaRot[3]);
    }


  drawVAO();

  glActiveTexture(GL_TEXTURE2);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE3);
  glDisable(GL_TEXTURE_RECTANGLE);
  
  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_1D);

  glUseProgram(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (m_selectActive)
    {
      glDisable(GL_PROGRAM_POINT_SIZE );
      glDisable(GL_POINT_SPRITE);
      
      glActiveTexture(GL_TEXTURE2);
      glDisable(GL_TEXTURE_RECTANGLE);
      
      glActiveTexture(GL_TEXTURE3);
      glDisable(GL_TEXTURE_RECTANGLE);
      
      glActiveTexture(GL_TEXTURE0);
      glDisable(GL_TEXTURE_1D);

      return;
    }   
//--------------------------------------------

//--------------------------------------------
//--------------------------------------------
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


//--------------------------------------------
  glUseProgram(m_shadowShader);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors

  glActiveTexture(GL_TEXTURE1);
  glEnable(GL_TEXTURE_RECTANGLE);
  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[1]); // depth


  QMatrix4x4 mvp4x4;
  mvp4x4.setToIdentity();
  mvp4x4.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);

  glUniformMatrix4fv(m_shadowParm[0], 1, GL_FALSE, mvp4x4.data());

  glUniform1i(m_shadowParm[1], 0); // colors
  glUniform1i(m_shadowParm[2], 1); // depthTex1

  glUniform1i(m_shadowParm[3], m_showEdges); // showedges
  glUniform1i(m_shadowParm[4], m_softShadows); // softShadows

  glUniform1f(m_shadowParm[5], m_nearDist);
  glUniform1f(m_shadowParm[6], m_farDist);

  glUniform1i(m_shadowParm[7], m_showSphere);

  glUniform1i(m_shadowParm[8], false); // copyOnly false

  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
  glVertexAttribPointer(0,  // attribute 0
			2,  // size
			GL_FLOAT, // type
			GL_FALSE, // normalized
			0, // stride
			(void*)0 ); // array buffer offset
  glDrawArrays(GL_QUADS, 0, 8);

  glDisableVertexAttribArray(0);

  glActiveTexture(GL_TEXTURE1);
  glDisable(GL_TEXTURE_RECTANGLE);

  glActiveTexture(GL_TEXTURE0);
  glDisable(GL_TEXTURE_RECTANGLE);

//--------------------------------------------
//--------------------------------------------

  glUseProgram(0);

  //glDisable(GL_PROGRAM_POINT_SIZE );
  glDisable(GL_POINT_SPRITE);
  //glDisable(GL_TEXTURE_2D);
}


void
Viewer::drawLabels()
{
  QMatrix4x4 mvp;
  GLdouble m[16];
  camera()->getModelViewProjectionMatrix(m);
  for(int i=0; i<16; i++) mvp.data()[i] = m[i];

  QVector3D cpos, hup, hvd, hrv;

  Vec cp = camera()->position();
  cpos = QVector3D(cp.x, cp.y, cp.z);

  Vec viewDir = camera()->viewDirection();
  hvd = QVector3D(viewDir.x,viewDir.y,viewDir.z);

  Vec upVec = camera()->upVector();
  hup = QVector3D(upVec.x,upVec.y,upVec.z);

  Vec rtVec = camera()->rightVector();
  hrv = QVector3D(rtVec.x,rtVec.y, rtVec.z);
  
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_pointClouds[d]->visible())
	m_pointClouds[d]->drawLabels(cpos,
				     hvd,
				     hup,
				     hrv,
				     mvp,
				     mvp,
				     mvp,
				     mvp,
				     -1,
				     QVector3D(-1,-1,-1));
    }

//  glDisable(GL_DEPTH_TEST);
//
//  startScreenCoordinatesSystem();
//
//  Vec cpos = camera()->position();
//  for(int d=0; d<m_pointClouds.count(); d++)
//    {
//      if (m_pointClouds[d]->visible())
//	m_pointClouds[d]->drawLabels(camera());
//    }
//
//  stopScreenCoordinatesSystem();
//
//  glEnable(GL_DEPTH_TEST);
}
void
Viewer::drawLabelsForVR(vr::Hmd_Eye eye)
{
//  glDepthMask(GL_FALSE); // enable writing to depth buffer
//  glDisable(GL_DEPTH_TEST);
//
//  glBindFramebuffer(GL_FRAMEBUFFER, m_depthBuffer);
//  glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,
//			 GL_COLOR_ATTACHMENT0,
//			 GL_TEXTURE_RECTANGLE,
//			 m_depthTex[0],
//			 0);
//  GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT };
//  glDrawBuffers(1, buffers);
//
//  glClear(GL_COLOR_BUFFER_BIT);

  QMatrix4x4 mvp = m_vr.viewProjection(eye);
  QMatrix4x4 matR = m_vr.matrixDevicePoseRight();
  QVector3D cpos = m_hmdPos;
  QMatrix4x4 finalxform = m_vr.final_xform();
  QMatrix4x4 finalxformInv = m_vr.final_xformInverted();

//  glEnable(GL_BLEND);
//  glBlendFunc(GL_ONE, GL_ONE);

  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_pointClouds[d]->visible())
	{
	  bool gotHit = m_pointClouds[d]->findNearestLabelHit(matR,
							      finalxformInv,
							      m_vr.deadRadius(),
							      m_vr.deadPoint());

	  m_pointClouds[d]->drawLabels(cpos,
				       m_hmdVD,
				       m_hmdUP,
				       m_hmdRV,
				       mvp, matR,
				       finalxform,
				       finalxformInv,
				       m_vr.deadRadius(),
				       m_vr.deadPoint());

	  if (gotHit)
	    {
	      GLuint texId = m_pointClouds[d]->labelTexture();
	      QSize texSize = m_pointClouds[d]->labelTextureSize();
	      m_vr.showHUD(eye, texId, texSize);
	    }
	}
    }
//  glDisable(GL_BLEND);
//
//  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return;

//  //------------------------------
//
//  m_vr.bindBuffer(eye);
//
//  glUseProgram(m_shadowShader);
//
//  glEnable(GL_BLEND);
//  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
//
//  glActiveTexture(GL_TEXTURE0);
//  glEnable(GL_TEXTURE_RECTANGLE);
//  glBindTexture(GL_TEXTURE_RECTANGLE, m_depthTex[0]); // colors
//
//  int wd = camera()->screenWidth();
//  int ht = camera()->screenHeight();
//
//  QMatrix4x4 mvp4x4;
//  mvp4x4.setToIdentity();
//  mvp4x4.ortho(0.0, wd, 0.0, ht, 0.0, 1.0);
//
//  glUniformMatrix4fv(m_shadowParm[0], 1, GL_FALSE, mvp4x4.data());
//
//  glUniform1i(m_shadowParm[1], 0); // colors
//  glUniform1i(m_shadowParm[8], true);
//
//  glEnableVertexAttribArray(0);
//  glBindBuffer(GL_ARRAY_BUFFER, m_vertexScreenBuffer);
//  glVertexAttribPointer(0,  // attribute 0
//			2,  // size
//			GL_FLOAT, // type
//			GL_FALSE, // normalized
//			0, // stride
//			(void*)0 ); // array buffer offset
//  glDrawArrays(GL_QUADS, 0, 8);
//
//  glDisableVertexAttribArray(0);
//
//  glActiveTexture(GL_TEXTURE0);
//  glDisable(GL_TEXTURE_RECTANGLE);
//
//  glUseProgram(0);
//
//  glDisable(GL_BLEND);
//  glDepthMask(GL_TRUE); // enable writing to depth buffer
//  glEnable(GL_DEPTH_TEST);
//  //------------------------------
}

void
Viewer::stickLabelsToGround()
{
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_pointClouds[d]->visible())
	m_pointClouds[d]->stickLabelsToGround();      
    }
}

void
Viewer::drawPointPairs()
{
  if (!m_editMode || m_pointPairs.count() == 0)
    return;

  glDisable(GL_DEPTH_TEST);
  startScreenCoordinatesSystem();
  for(int i=0; i<m_pointPairs.count(); i++)
    {
      int pcN;
      if (i<3)
	pcN = 1;
      else
	pcN = 2;

      int pn;
      if (i<3)
	pn = i;
      else
	pn = i-2;
      
      Vec v = m_pointPairs[i];
      
      Vec scr = camera()->projectedCoordinatesOf(v);
      int x = scr.x;
      int y = scr.y;
      QFont font = QFont("Helvetica", 10);
      QString text = QString("%1(%2)").arg(pcN).arg(pn);
      StaticFunctions::renderText(x, y, text, font, Qt::black, Qt::white);
    }
  stopScreenCoordinatesSystem();
  glEnable(GL_DEPTH_TEST);
}

void
Viewer::drawAABB()
{
  glDisable(GL_LIGHTING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  

  glColor3f(1,1,1);
  glLineWidth(1.0);

  if (m_editMode)
    {
      for(int d=0; d<m_pointClouds.count(); d++)
	{
	  Vec cmin = m_pointClouds[d]->tightOctreeMin();
	  Vec cmax = m_pointClouds[d]->tightOctreeMax();
	  if (d == 1)
	    { 
	      if (m_editMode && m_deltaChanged)
		{
		  glLineWidth(2.0);
		  glColor3f(0.5,1.0,0.7);
		}

	      Vec gmin = m_pointClouds[1]->globalMin();
	      Vec shift = m_deltaShift - gmin;

	      cmin = m_pointClouds[1]->tightOctreeMinO();
	      cmax = m_pointClouds[1]->tightOctreeMaxO();
	      Vec cmid = m_pointClouds[1]->getXformCen();

	      QList<Vec> v;
	      v << Vec(cmin.x, cmin.y, cmin.z);
	      v << Vec(cmax.x, cmin.y, cmin.z);
	      v << Vec(cmax.x, cmax.y, cmin.z);
	      v << Vec(cmin.x, cmax.y, cmin.z);
	      v << Vec(cmin.x, cmin.y, cmax.z);
	      v << Vec(cmax.x, cmin.y, cmax.z);
	      v << Vec(cmax.x, cmax.y, cmax.z);
	      v << Vec(cmin.x, cmax.y, cmax.z);
	      v << Vec(cmin.x, cmax.y, cmin.z);
	      v << Vec(cmax.x, cmax.y, cmin.z);
	      v << Vec(cmax.x, cmax.y, cmax.z);
	      v << Vec(cmin.x, cmax.y, cmax.z);
	      v << Vec(cmin.x, cmin.y, cmin.z);
	      v << Vec(cmax.x, cmin.y, cmin.z);
	      v << Vec(cmax.x, cmin.y, cmax.z);
	      v << Vec(cmin.x, cmin.y, cmax.z);  

	      for(int ip=0; ip<v.count(); ip++)
		{
		  Vec ov = v[ip]-cmid;
		  ov = m_deltaRot.rotate(ov);		  
		  ov *= m_deltaScale;
		  ov += cmid;  
		  ov += shift;
		  //ov += m_deltaShift;
		  //ov -= gmin;
		  v[ip] = ov;
		}
	      
	      StaticFunctions::drawBox(v);
	    }
	  else
	    StaticFunctions::drawBox(cmin, cmax);
	}
    }
  else
    {
      glColor3f(1,1,0.8);
      glLineWidth(2.0);
      StaticFunctions::drawBox(m_coordMin, m_coordMax);

      glColor3f(1,1,1);
      glLineWidth(1.0);
      for(int d=0; d<m_pointClouds.count(); d++)
	{
	  Vec cmin = m_pointClouds[d]->tightOctreeMin();
	  Vec cmax = m_pointClouds[d]->tightOctreeMax();

	  StaticFunctions::drawBox(cmin, cmax);
	}

//      for(int d=0; d<m_pointClouds.count(); d++)
//	m_pointClouds[d]->drawInactiveTiles();
//      
//      
//      glLineWidth(2.0);
//      for(int d=0; d<m_pointClouds.count(); d++)
//	m_pointClouds[d]->drawActiveNodes();
    }

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glLineWidth(1.0);
}

Vec
Viewer::closestPointOnBox(Vec bmin, Vec bmax, Vec cpos)
{
  Vec closestPtOnBox;
  closestPtOnBox.x = (cpos.x < bmin.x) ? bmin.x : (cpos.x > bmax.x) ? bmax.x : cpos.x;
  closestPtOnBox.y = (cpos.y < bmin.y) ? bmin.y : (cpos.y > bmax.y) ? bmax.y : cpos.y;
  closestPtOnBox.z = (cpos.z < bmin.z) ? bmin.z : (cpos.z > bmax.z) ? bmax.z : cpos.z;

  return closestPtOnBox;
}

bool
Viewer::isVisible(Vec bmin, Vec bmax)
{
  // false if fully outside, true if inside or intersects


  //-------------------------------------------------
  // check box outside/inside of frustum
  bool allInForAllPlanes = true;
  // 6 frustum planes
  for (int i=0; i<6; i++)
    { 
      bool allOut = true;
      for (int c=0; c<8; c++)
	{
	  Vec pos((c&4)?bmin.x:bmax.x, (c&2)?bmin.y:bmax.y, (c&1)?bmin.z:bmax.z);
	  float dtof = pos * Vec(m_planeCoefficients[i]) - m_planeCoefficients[i][3];
	  if (dtof > 0.0)
	    allInForAllPlanes = false;
	  else
	    allOut = false;
	}
      // all 8 points are outside side of this plane
      if (allOut)
	return false;
    }
  //-------------------------------------------------


//  // probably intersects frustum but more check, just in case it is outside
//  //-------------------------------------------------
//  // check frustum outside/inside box
//  int out;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].x > bmax.x)?1:0); if( out==8 ) return false;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].x < bmin.x)?1:0); if( out==8 ) return false;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].y > bmax.y)?1:0); if( out==8 ) return false;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].y < bmin.y)?1:0); if( out==8 ) return false;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].z > bmax.z)?1:0); if( out==8 ) return false;
//  out=0; for(int i=0; i<8; i++) out += ((m_fruCnr[i].z < bmin.z)?1:0); if( out==8 ) return false;
//  //-------------------------------------------------
  
  return true;
}

void
Viewer::calcFrustrumCornerPointsAndPlaneCoeff(float frc)
{
  // get frustum planes for finding visibility of bounding boxes
  //camera()->getFrustumPlanesCoefficients(m_planeCoefficients);

  Camera cam = Camera(*camera());
  cam.setFieldOfView(camera()->fieldOfView()*frc);
  cam.getFrustumPlanesCoefficients(m_planeCoefficients);
}

void
Viewer::orderTilesForCamera()
{
  m_tiles.clear();

  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_editMode ||
	  m_pointClouds.count() == 1)
	{
	  m_pointClouds[d]->setVisible(true);
	  m_tiles += m_pointClouds[d]->tiles();
	}
      else if (m_pointClouds[d]->time() != -1 &&
	       m_pointClouds[d]->time() != m_currTime)
	{
	  m_pointClouds[d]->setVisible(false);
	}
      else
	{
	  m_pointClouds[d]->setVisible(true);
	  m_tiles += m_pointClouds[d]->tiles();
	}
    }


  QMultiMap<float, OctreeNode*> mmTiles;

  Vec cpos = camera()->position();
  Vec viewDir = camera()->viewDirection();
  int ht = camera()->screenHeight();
  float fov = camera()->fieldOfView();
  float slope = qTan(fov/2);
  float projFactor = (0.5f*ht)/slope;

  camera()->getFrustumPlanesCoefficients(m_planeCoefficients);

  for(int d=0; d<m_tiles.count(); d++)
    {
      OctreeNode *node = m_tiles[d];
      Vec bmin = node->bmin();
      Vec bmax = node->bmax(); 

      if (isVisible(bmin, bmax))
	{
	  float screenProjectedSize = StaticFunctions::projectionSize(cpos,
								      bmin, bmax,
								      projFactor);
	  mmTiles.insert(screenProjectedSize, node);
	}
    }

  m_orderedTiles.clear();
  QList<float> keys = mmTiles.keys();
  for(int k=0; k<keys.count(); k++)
    {
      QList<OctreeNode*> values = mmTiles.values(keys[k]);
      m_orderedTiles += values;
    }
}

void
Viewer::genDrawNodeList()
{
//---------------------------
//---------------------------
//
//  genDrawNodeListForVR();
//  return;
//
//---------------------------
//---------------------------

//  emit stopLoading();

//  m_minNodePixelSize = 100;
//  if (m_flyMode)
//    m_minNodePixelSize = 150;

  //-----------------------------
  if (m_trisets.count() > 0)
    {
      Vec cpos = camera()->position();
      float nearDist = -1;
      float farDist = -1;
      for (int l=0; l<m_trisets.count(); l++)
	{
	  for (int c=0; c<8; c++)
	    {
	      Vec bmin = m_trisets[l]->bmin();
	      Vec bmax = m_trisets[l]->bmin();
	      Vec pos((c&4)?bmin.x:bmax.x, (c&2)?bmin.y:bmax.y, (c&1)?bmin.z:bmax.z);
	      
	      float len = (pos - cpos).norm();
	      if (nearDist < 0)
		{
		  nearDist = qMax(0.0f, len);
		  farDist = qMax(0.0f, len);
		}
	      else
		{
		  nearDist = qMax(0.0f, qMin(nearDist, len));
		  farDist = qMax(farDist, len);
		}
	    }    
	}
      setNearFar(nearDist, farDist);
    }
  //-----------------------------

  if (m_pointClouds.count() == 0)
    return;

  QList<OctreeNode*> oldLoadNodes = m_loadNodes;

  orderTilesForCamera();
  

  for(int d=0; d<m_pointClouds.count(); d++)
    {
      QList<OctreeNode*> allNodes = m_pointClouds[d]->allNodes();
      for(int od=0; od<allNodes.count(); od++)
	allNodes[od]->setActive(false);
    }

  m_pointsDrawn = 0;
  m_loadNodes.clear();


  int nsteps = 4;
  float cfrc[4] = {1.0, 0.8, 0.6, 0.4};
  float frc[4] = {5.0, 10.0, 50.0, -1.0};
  int fid = 0;
  for(int i=0; i<nsteps; i++)
    {
      // frustum coefficients for visibility of bounding boxes
      calcFrustrumCornerPointsAndPlaneCoeff(cfrc[fid]);

      // consider nearer areas first
      float mnfrc = qSqrt((float)(i+0.5f)/(float)nsteps);
      if (genDrawNodeList(mnfrc, frc[i]))
        break;

      // change camera frustum only if enough points have been
      // collected in this round
      if (m_pointsDrawn > m_pointBudget*0.1*(i+1))
	fid ++;
    }  

  //-------------------------------
  // sort currload based on distance to the viewer
  Vec cpos = camera()->position();
  QMultiMap<float, OctreeNode*> mmTiles;
  for(int i=0; i<m_loadNodes.count(); i++)
    {
      OctreeNode *node = m_loadNodes[i];
      Vec bmin = node->bmin();
      Vec bmax = node->bmax(); 
      Vec bcen = (bmax+bmin)/2;
      float dist = (bcen-cpos).squaredNorm();
      mmTiles.insert(dist, node);
    }
  m_loadNodes.clear();
  QList<float> keys = mmTiles.keys();
  for(int k=0; k<keys.count(); k++)
    {
      QList<OctreeNode*> values = mmTiles.values(keys[k]);
      m_loadNodes += values;
    }
  //-------------------------------

  //-------------------------------
  if (Global::playFrames())
      //oldLoadNodes.count() == m_loadNodes.count())
    {
      bool allok = true;
      for(int i=0; i<m_loadNodes.count(); i++)
	{
	  if (!oldLoadNodes.contains(m_loadNodes[i]))
	    {
	      allok = false;
	      break;
	    }
	}
      if (allok)
	{
	  m_vboLoadedAll = true;
	  return;
	}
    }
  //-------------------------------
  
  m_volume->setLoadingNodes(m_loadNodes);

  // multi threaded
  if (m_showPoints)
    emit loadPointsToVBO();

  createVisibilityTexture();

  //-----------------
  //Vec cpos = camera()->position();
  Vec viewDir = camera()->viewDirection();

  m_nearDist = -1;
  m_farDist = -1;
  for (int l=0; l<m_loadNodes.count(); l++)
    {
      for (int c=0; c<8; c++)
	{
	  Vec bmin = m_loadNodes[l]->tightOctreeMin();
	  Vec bmax = m_loadNodes[l]->tightOctreeMax();
	  Vec pos((c&4)?bmin.x:bmax.x, (c&2)?bmin.y:bmax.y, (c&1)?bmin.z:bmax.z);
	  
	  float z = (pos - cpos)*viewDir;
	  if (m_nearDist < 0)
	    {
	      m_nearDist = qMax(0.0f, z);
	      m_farDist = qMax(0.0f, z);
	    }
	  else
	    {
	      m_nearDist = qMax(0.0f, qMin(m_nearDist, z));
	      m_farDist = qMax(m_farDist, z);
	    }
	}    
    }
}

bool
Viewer::nearCam(Vec cpos, Vec viewDir, Vec bmin, Vec bmax, float zfar)
{
  for (int c=0; c<8; c++)
    {
      Vec pos((c&4)?bmin.x:bmax.x, (c&2)?bmin.y:bmax.y, (c&1)?bmin.z:bmax.z);

      float z = (pos - cpos)*viewDir;
      //float z = camera()->projectedCoordinatesOf(pos).z;

      if (z < zfar)
	return true;
    }
  return false;
}

bool
Viewer::genDrawNodeList(float mnfrc, float zfar)
{
  bool bufferFull = false;

  Vec cpos = camera()->position();
  float fov = camera()->fieldOfView();
  Vec viewDir = camera()->viewDirection();
  Vec upVec = camera()->upVector();
  Vec rtVec = camera()->rightVector();

  int wd = camera()->screenWidth();
  int ht = camera()->screenHeight();

  float slope = qTan(fov/2);
  float projFactor = (0.5f*ht)/slope;


  QList<OctreeNode*> onl0;

  // consider top-level tile nodes first
  for(int d=0; d<m_orderedTiles.count(); d++)
    {
      OctreeNode *node = m_orderedTiles[d];
      
      Vec bmin = node->bmin(); 
      Vec bmax = node->bmax(); 

//      bool addNode = isVisible(bmin, bmax);
      bool addNode = true;

//      if (addNode && zfar > 0.0)
//	addNode = nearCam(cpos, viewDir, bmin, bmax, zfar);
      
      if (addNode)
	{
	  float screenProjectedSize = StaticFunctions::projectionSize(cpos,
								      bmin, bmax,
								      projFactor);
//	  QMessageBox::information(0, "", QString("%1 : %2").\
//				   arg(node->uid()).\
//				   arg(screenProjectedSize));

	  bool ignoreChildren = (screenProjectedSize < m_minNodePixelSize*mnfrc);

	  if (!ignoreChildren)
	    onl0 << node;
	  
	  if (!node->isActive())
	    {
	      if (m_pointsDrawn + node->numpoints() < m_pointBudget)
		{
		  m_loadNodes << node;
		  node->setActive(true);
		  m_pointsDrawn += node->numpoints();
		}
	      else
		{
		  bufferFull = true;
		  break;
		}
	    }
	}
    }

  if (bufferFull)
    return bufferFull;

//  // take only toplevel nodes
//  return true;
  

  // now consider the nodes within each tile
  while (onl0.count() > 0)
    {
      bool done = false;
      QList<OctreeNode*> onl1;
      onl1.clear();
      for(int i=0; i<onl0.count() && !done; i++)
	{
	  OctreeNode *node = onl0[i];
	  for (int k=0; k<8 && !done; k++)
	    {
	      OctreeNode *cnode = node->getChild(k);
	      if (cnode)
		{     
		  Vec bmin = cnode->bmin();
		  Vec bmax = cnode->bmax();			  
		  
		  bool addNode = isVisible(bmin, bmax);
		  
		  if (addNode && zfar > 0.0)
		    addNode = nearCam(cpos, viewDir, bmin, bmax, zfar);
		  
		  bool ignoreChildren = false;
		  if (addNode)
		    { 
		      //--------------------------
		      float screenProjectedSize = StaticFunctions::projectionSize(cpos,
										  bmin, bmax,
										  projFactor);
		      ignoreChildren = (screenProjectedSize < m_minNodePixelSize*mnfrc);
		      //--------------------------
		    }
		  
		  if (addNode)
		    {
		      if (!ignoreChildren)
			onl1 << cnode;
		      
		      if (!cnode->isActive())
			{
			  if (m_pointsDrawn + cnode->numpoints() < m_pointBudget)
			    {
			      m_loadNodes << cnode;
			      cnode->setActive(true);
			      m_pointsDrawn += cnode->numpoints();
			    }
			  else
			    {
			      bufferFull = true;
			      done = true;
			      break;
			    }
			} // only non-activated nodes
		    } // addPt
		} // valid child
	    } // loop over child nodes
	} // loop over onl0

      if (!done)
	onl0 = onl1;
      else
	break;
    }

  return bufferFull;
}

void
Viewer::savePointsToFile(Vec newp)
{
  QString mesg;
  mesg = QString("%1, %2, %3\n").			\
    arg(newp.x).arg(newp.y).arg(newp.z);

  m_pFile.open(QIODevice::Append | QIODevice::Text);
  m_pFile.write(mesg.toLatin1());
  m_pFile.close();

}

//void
//Viewer::createMinMaxTexture()
//{
//  if (!m_minmaxTex) glGenTextures(1, &m_minmaxTex);
//
//  m_tiles.clear();
//  for(int d=0; d<m_pointClouds.count(); d++)
//    m_tiles += m_pointClouds[d]->tiles();
//
//
//  QList<uchar> visibleLevel;
//  for(int d=0; d<m_pointClouds.count(); d++)
//    visibleLevel += m_pointClouds[d]->maxLevelVisible();
//
//
//  int ntiles = m_tiles.count();
//  if (m_minmaxMap) delete [] m_minmaxMap;
//  m_minmaxMap = new float[8*ntiles];
//
//  memset(m_minmaxMap, 0, sizeof(float)*8*ntiles);
//
//  for(int i=0; i<ntiles; i++)
//    {
//      Vec bmin = m_tiles[i]->bmin();
//      Vec bmax = m_tiles[i]->bmax();
//      float spacing = m_tiles[i]->spacing();
//      float maxLevel = visibleLevel[i];
//
//      m_minmaxMap[8*i+0] = bmin.x;
//      m_minmaxMap[8*i+1] = bmin.y;
//      m_minmaxMap[8*i+2] = bmin.z;
//      m_minmaxMap[8*i+3] = spacing;
//      m_minmaxMap[8*i+4] = bmax.x;
//      m_minmaxMap[8*i+5] = bmax.y;
//      m_minmaxMap[8*i+6] = bmax.z;
//      m_minmaxMap[8*i+7] = maxLevel;
//    }
//
//  GLuint target = GL_TEXTURE_RECTANGLE;
//  glActiveTexture(GL_TEXTURE2);
//    glBindTexture(target, m_minmaxTex);
//  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
//  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
//  glTexParameterf(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
//  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//  glTexImage2D(target,
//	       0,
//	       GL_RGBA32F,
//	       2, ntiles, 
//	       0,
//	       GL_RGBA,
//	       GL_FLOAT,
//	       m_minmaxMap);  
//}

void
Viewer::createMinMaxTexture()
{
  if (!m_minmaxTex) glGenTextures(1, &m_minmaxTex);

  m_tiles.clear();
  for(int d=0; d<m_pointClouds.count(); d++)
    m_tiles += m_pointClouds[d]->tiles();

  int ntiles = m_tiles.count();
  if (m_minmaxMap) delete [] m_minmaxMap;
  m_minmaxMap = new float[8*ntiles];

  memset(m_minmaxMap, 0, sizeof(float)*8*ntiles);

  for(int i=0; i<ntiles; i++)
    {
      Vec bmin = m_tiles[i]->bmin();
      Vec bmax = m_tiles[i]->bmax();
      float spacing = m_tiles[i]->spacing();

      m_minmaxMap[8*i+0] = bmin.x;
      m_minmaxMap[8*i+1] = bmin.y;
      m_minmaxMap[8*i+2] = bmin.z;
      m_minmaxMap[8*i+3] = spacing;
      m_minmaxMap[8*i+4] = bmax.x;
      m_minmaxMap[8*i+5] = bmax.y;
      m_minmaxMap[8*i+6] = bmax.z;
      m_minmaxMap[8*i+7] = spacing;
    }

  GLuint target = GL_TEXTURE_RECTANGLE;
  glActiveTexture(GL_TEXTURE2);
    glBindTexture(target, m_minmaxTex);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(target,
	       0,
	       GL_RGBA32F,
	       2, ntiles, 
	       0,
	       GL_RGBA,
	       GL_FLOAT,
	       m_minmaxMap);  
}  

void
Viewer::createVisibilityTexture()
{
  if (!m_visibilityTex) glGenTextures(1, &m_visibilityTex);


  // update visibility for pointcloud tiles
  for(int d=0; d<m_pointClouds.count(); d++)
    m_pointClouds[d]->updateVisibilityData();
  
  QList< QList<uchar> > m_vData;
  m_vData.clear();

  for(int d=0; d<m_pointClouds.count(); d++)
    m_vData += m_pointClouds[d]->vData();


  int ntiles = m_vData.count();

  int maxwd = 0;
  for(int i=0; i<ntiles; i++)
    maxwd = qMax(maxwd, m_vData[i].count());


  if (m_visibilityMap) delete [] m_visibilityMap;
  m_visibilityMap = new uchar[maxwd*ntiles];
  memset(m_visibilityMap, 0, maxwd*ntiles);
    
  for(int i=0; i<ntiles; i++)
    {
      QList<uchar> vS = m_vData[i];
      
      for(int k=0; k<vS.count(); k++)
	m_visibilityMap[i*maxwd + k] = vS[k];
    }


  GLuint target = GL_TEXTURE_RECTANGLE;
  glActiveTexture(GL_TEXTURE3);
    glBindTexture(target, m_visibilityTex);
  glTexParameterf(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
  glTexParameterf(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
  glTexParameterf(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(target,
	       0,
	       GL_RGBA8,
	       maxwd/4, ntiles,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_visibilityMap); 

}

void
Viewer::loadLinkOnTop(QString dirname)
{
  if (!m_volume)
    {
      QMessageBox::information(0, "", "Volume not found - should not be here");
      return;
    }

  //m_volume->setShiftToZero(false);
  
  m_volumeFactory->pushVolume(m_volume);

  m_dataDir = dirname;

  m_vr.setDataDir(dirname);

  if (!m_volume->loadOnTop(dirname))
    {
      QMessageBox::information(0, "Error", "Cannot load tiles from directory");
      return;
    }

  start();

  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
			 QPointF(0,0),
			 Qt::LeftButton,
			 Qt::LeftButton,
			 Qt::NoModifier);
  mouseReleaseEvent(&dummyEvent);
}


void
Viewer::loadLink(QString dirname)
{
  if (m_volume)
    {
      loadLinkOnTop(dirname);
      return;
    }

  m_dataDir = dirname;

  m_vr.setDataDir(dirname);

  // if current volume is not null then push on the stack
  if (m_volume)
    {
      m_volume->setCamera(camera());
      m_volumeFactory->pushVolume(m_volume);
    }

  Volume *vol = m_volumeFactory->checkVolume(dirname);
  if (vol) // if volume was already accessed earlier use it
    {
      m_volumeFactory->pushVolume(vol);
    }
  else
    {
      // create new volume
      // new volume is automatically pushed on the stack
      Volume *newvol = m_volumeFactory->newVolume();

      if (m_vrMode && m_vr.vrEnabled())
	newvol->setShiftToZero(true);
      
      if (!newvol->loadDir(dirname))
	{
	  QMessageBox::information(0, "Error", "Cannot load tiles from directory");
	  m_volumeFactory->popVolume();
	  delete newvol;
	  return;
	}
    }

  start();

  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
			 QPointF(0,0),
			 Qt::LeftButton,
			 Qt::LeftButton,
			 Qt::NoModifier);
  mouseReleaseEvent(&dummyEvent);
}

void
Viewer::loadLink(QStringList dirnames)
{
  // if current volume is not null then push on the stack
  if (!m_volume)
    {
      m_volumeFactory->pushVolume(m_volume);
      m_volume->setCamera(camera());
    }

  // create new volume
  // new volume is automatically pushed on the stack
  Volume *vol = m_volumeFactory->newVolume();
  if (m_vrMode && m_vr.vrEnabled())
    vol->setShiftToZero(true);
      

  if (!vol->loadTiles(dirnames))
    {
      QMessageBox::information(0, "Error", "Cannot load tiles from directory");
      m_volumeFactory->popVolume();
      delete vol;
      return;
    }

  start();

  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
			 QPointF(0,0),
			 Qt::LeftButton,
			 Qt::LeftButton,
			 Qt::NoModifier);
  mouseReleaseEvent(&dummyEvent);
}

bool
Viewer::linkClicked(QMouseEvent *event)
{
  bool found = false;

  QPoint pos = event->pos();

  if (m_volumeFactory->stackSize() > 0)
    {
      if (qAbs(m_backButtonPos.x() - pos.x()) < 100 &&
	  qAbs(m_backButtonPos.y() - pos.y()) < 100)
	{
	  // save camera parameters for this volume
	  // we can use them if we return to this volume again
	  m_volume->setCamera(camera());

	  start();

	  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
				 QPointF(0,0),
				 Qt::LeftButton,
				 Qt::LeftButton,
				 Qt::NoModifier);
	  mouseReleaseEvent(&dummyEvent);

	  return true;
	}
    }

  
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_pointClouds[d]->visible())
	{
	  QString dirname = m_pointClouds[d]->checkLink(camera(), pos);
	  if (!dirname.isEmpty())
	    {
	      found = true;
	      emit loadLinkedData(dirname);
	      break;
	    }
	}
    }

  return found;
}


//-----------------------------------------------------------
//-----------------------------------------------------------
void
Viewer::genDrawNodeListForVR()
{
  //-----------------------------
  if (m_trisets.count() > 0)
    {
      Vec cpos = camera()->position();
      float nearDist = -1;
      float farDist = -1;
      for (int l=0; l<m_trisets.count(); l++)
	{
	  for (int c=0; c<8; c++)
	    {
	      Vec bmin = m_trisets[l]->bmin();
	      Vec bmax = m_trisets[l]->bmin();
	      Vec pos((c&4)?bmin.x:bmax.x, (c&2)?bmin.y:bmax.y, (c&1)?bmin.z:bmax.z);
	      
	      float len = (pos - cpos).norm();
	      if (nearDist < 0)
		{
		  nearDist = qMax(0.0f, len);
		  farDist = qMax(0.0f, len);
		}
	      else
		{
		  nearDist = qMax(0.0f, qMin(nearDist, len));
		  farDist = qMax(farDist, len);
		}
	    }    
	}
      setNearFar(nearDist, farDist);
    }
  //-----------------------------

  m_volume->setNewLoad(true);

  emit updateView();

  emit message("viewer - update view");
}

void
Viewer::saveTopJson(QString jsonfile)
{
  QJsonObject jsonMod;

  QJsonObject jsonInfo;

  qint64 million = 1000000; 
  int pb = m_pointBudget/million;
  jsonInfo["point_budget"] = pb;


  jsonMod["top"] = jsonInfo;

  QJsonDocument saveDoc(jsonMod);

  QFile saveFile(jsonfile);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson());
}

void
Viewer::loadTopJson(QString jsonfile)
{
  QFile loadFile(jsonfile);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  if (data.count() == 0) // empty file
    return;

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonObject jsonMod = jsonDoc.object();

  qint64 million = 1000000; 
  if (jsonMod.contains("top"))
    {
      QJsonObject jsonInfo = jsonMod["top"].toObject();
      
      if (jsonInfo.contains("point_budget"))
	m_pointBudget = million * jsonInfo["point_budget"].toInt();

      if (jsonInfo.contains("headset"))
	{
	  QString hs = jsonInfo["headset"].toString();
	  if (hs == "vive") m_headSetType = 1;
	  if (hs == "oculus") m_headSetType = 2;
	}

    }

  if (m_pointBudget < million)
    m_pointBudget = million;
}

void
Viewer::setKeyFrame(int fno)
{
  Vec pos;
  Quaternion rot;

  pos = camera()->position();
  rot = camera()->orientation();

  QImage image = grabFrameBuffer();
  image = image.scaled(100, 100);

  emit setKeyFrame(pos, rot, fno, image, m_currTime);
}

void
Viewer::updateLookFrom(Vec pos, Quaternion rot, int ct)
{
  camera()->setPosition(pos);
  camera()->setOrientation(rot);
  m_currTime = ct;
  m_currTime = qMin(m_maxTime, m_currTime);
  m_currTime = qMax(m_currTime, 0);

  emit timeStepChanged(m_currTime);

  m_vboLoadedAll = false;
  QMouseEvent dummyEvent(QEvent::MouseButtonRelease,
			 QPointF(0,0),
			 Qt::LeftButton,
			 Qt::LeftButton,
			 Qt::NoModifier);
  mouseReleaseEvent(&dummyEvent);

//  genDrawNodeList();
//
//  if (m_vboLoadedAll)
//    emit updateGL();
}

void
Viewer::saveImage()
{
  QString localImageFileName = m_imageFileName;
  QChar fillChar = '0';
  int fieldWidth = 0;
  QRegExp rx("\\$[0-9]*[f|F]");
  if (rx.indexIn(m_imageFileName) > -1)
    {
      localImageFileName.remove(rx);
      
      QString txt = rx.cap();
      if (txt.length() > 2)
	{
	  txt.remove(0,1);
	  txt.chop(1);
	  fieldWidth = txt.toInt();
	}
    }

  saveMonoImage(localImageFileName, fillChar, fieldWidth);
}

void
Viewer::saveMonoImage(QString localImageFileName,
		      QChar fillChar, int fieldWidth)
{
  QFileInfo f(localImageFileName);	  

  //---------------------------------------------------------
  QString imgFile = f.absolutePath() + QDir::separator() +
	            f.baseName();
  if (m_currFrame >= 0)
    imgFile += QString("%1").arg((int)m_currFrame, fieldWidth, 10, fillChar);
  imgFile += ".";
  imgFile += f.completeSuffix();
  //---------------------------------------------------------

  saveSnapshot(imgFile);
}

void
Viewer::saveSnapshot(QString imgFile)
{
  QImage image = grabFrameBuffer();
  image.save(imgFile);      
}


bool
Viewer::startMovie(QString flnm,
		   int ofps, int quality,
		   bool checkfps)
{
#ifdef USE_GLMEDIA
  int fps = ofps;

  if (checkfps)
    {
      bool ok;
      QString text;
      text = QInputDialog::getText(this,
				   "Set Frame Rate",
				   "Frame Rate",
				   QLineEdit::Normal,
				   "25",
				   &ok);
      
      if (ok && !text.isEmpty())
	fps = text.toInt();
      
      if (fps <=0 || fps >= 100)
	fps = 25;
    }

  //---------------------------------------------------------
  // mono movie or left-eye movie
  m_movieWriter = glmedia_movie_writer_create();
  if (m_movieWriter == NULL) {
    QMessageBox::critical(0, "Movie",
			  "Failed to create writer");
    return false;
  }

  if (glmedia_movie_writer_start(m_movieWriter,
				 flnm.toLatin1().data(),
				 m_imageWidth,
				 m_imageHeight,
				 fps,
				 quality) < 0) {
    QMessageBox::critical(0, "Movie",
			  "Failed to start movie");
    return false;
  }
  //---------------------------------------------------------


  if (m_movieFrame)
    delete [] m_movieFrame;
  m_movieFrame = new unsigned char[4*m_imageWidth*m_imageHeight];

  // change the widget size
  QWidget::resize(m_imageWidth, m_imageHeight);
  //setWidgetSizeToImageSize();

#endif // USE_GLMEDIA
  return true;
}

bool
Viewer::endMovie()
{
#ifdef USE_GLMEDIA
  if (glmedia_movie_writer_end(m_movieWriter) < 0)
    {
      QMessageBox::critical(0, "Movie",
			       "Failed to end movie");
      return false;
    }
  glmedia_movie_writer_free(m_movieWriter);
#endif // USE_GLMEDIA
  return true;
}

void
Viewer::saveMovie()
{
#ifdef USE_GLMEDIA

  QImage image = grabFrameBuffer();
  StaticFunctions::convertFromGLImage(image);

  uchar *p = (uchar*)image.bits();
  memcpy(m_movieFrame, p, 4*image.width()*image.height());

  glmedia_movie_writer_add(m_movieWriter, m_movieFrame);

#endif // USE_GLMEDIA
}

void
Viewer::alignUsingPointPairs()
{
  if (!m_editMode || m_pointPairs.count() == 0)
    {
      QMessageBox::information(0, "Error",
			       "To use point-pair alignment you need to be in Edit Mode.\nYou also need to select 3 points in each of the two point clouds");
      return;
    }

  QList<Vec> pointPair1;
  QList<Vec> pointPair2;

  pointPair1 << m_pointPairs[0];
  pointPair1 << m_pointPairs[1];
  pointPair1 << m_pointPairs[2];

  pointPair2 << m_pointPairs[3];
  pointPair2 << m_pointPairs[4];
  pointPair2 << m_pointPairs[5];

  Vec v1 = pointPair1[1]-pointPair1[0];
  Vec w1 = pointPair1[2]-pointPair1[0];

  // get original point coordinates
  Vec x0 = m_pointClouds[1]->xformPointInverse(pointPair2[0]);
  Vec x1 = m_pointClouds[1]->xformPointInverse(pointPair2[1]);
  Vec x2 = m_pointClouds[1]->xformPointInverse(pointPair2[2]);

  Vec shift = pointPair1[0]-x0;

  Vec v2 = x1 - x0;
  Vec w2 = x2 - x0;

  Quaternion r1 = StaticFunctions::getRotationBetweenVectors(v2, v1);
  w2 = r1.rotate(w2);
  Quaternion r2 = StaticFunctions::getRotationBetweenVectors(w2, w1);
  
  Quaternion q = r2*r1;

  Vec xformCen = x0;
  float scale = m_pointClouds[1]->getScale();


  shift += m_pointClouds[1]->globalMin();
  m_pointClouds[1]->setXform(scale, shift, q, xformCen);
  

  m_pointPairs.clear();
  m_deltaRot = Quaternion();
  m_deltaShift = Vec(0,0,0);
  m_deltaScale = 1.0;
  m_deltaChanged = false;

  genDrawNodeList();
  update();
}
