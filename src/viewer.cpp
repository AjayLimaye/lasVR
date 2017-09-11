#include "staticfunctions.h"
#include "viewer.h"
#include "global.h"
#include "shaderfactory.h"

#include <QMessageBox>
#include <QtMath>

#include <QGLViewer/manipulatedCameraFrame.h>
using namespace qglviewer;

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#define VECDIVIDE(a, b) Vec(a.x/b.x, a.y/b.y, a.z/b.z)

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

  m_nearDist = 0;
  m_farDist = 1;

  m_volume = 0;
  m_volumeFactory = 0;

  m_backButtonPos = QPoint(10, 100);

  reset();

  setMinimumSize(100,100);

  connect(this, SIGNAL(loadLinkedData(QString)),
	  this, SLOT(loadLink(QString)));

  connect(this, SIGNAL(loadLinkedData(QStringList)),
	  this, SLOT(loadLink(QStringList)));


  QTimer *fpsTimer = new QTimer(this);
  connect(fpsTimer, &QTimer::timeout,
	  this, &Viewer::updateFramerate);
  m_frames = 0;
  fpsTimer->start(1000);
}

Viewer::~Viewer()
{
  reset();

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
  m_firstImageDone = 0;
  m_vboLoadedAll = false;
  m_fastDraw = false;

  m_pointType = true; // adaptive
  m_pointSize = 2;

  qint64 million = 1000000; 
  m_pointBudget = 10*million;

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

  m_nearDist = 0;
  m_farDist = 1;
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

  m_vr.initVR();

  emit message("Start");
}

void
Viewer::genColorMap()
{
  if (m_colorMap)
    return;

//  QList<Vec> colgrad;
//  colgrad << 255*Vec(0.0, 1.000, 0.500);
//  colgrad << 255*Vec(0.3, 1.000, 0.767);
//  colgrad << 255*Vec(0.6, 1.000, 0.933);
//  colgrad << 255*Vec(0.8, 1.000, 1.000);
//  colgrad << 255*Vec(1.0, 1.000, 0.800);
//  colgrad << 255*Vec(1.0, 0.933, 0.600);
//  colgrad << 255*Vec(1.0, 0.767, 0.300);
//  colgrad << 255*Vec(1.0, 0.500, 0.000);
  
//  m_colorGrad << Vec(0.0, 0.500, 1.000);
//  m_colorGrad << Vec(0.6, 1.000, 0.933);
//  m_colorGrad << Vec(0.8, 1.000, 1.000);
//  m_colorGrad << Vec(0.3, 1.000, 0.767);
//  m_colorGrad << Vec(1.0, 1.000, 0.800);
//  m_colorGrad << Vec(1.0, 0.933, 0.600);
//  m_colorGrad << Vec(1.0, 0.767, 0.300);
//  m_colorGrad << Vec(1.0, 0.500, 0.000);

  m_colorGrad << Vec(1,102, 94)/255.0f;
  m_colorGrad << Vec(53,151,143)/255.0f;
  m_colorGrad << Vec(128,205,193)/255.0f;
  m_colorGrad << Vec(199,234,229)/255.0f;
  m_colorGrad << Vec(246,232,195)/255.0f;
  m_colorGrad << Vec(223,194,125)/255.0f;
  m_colorGrad << Vec(191,129,45)/255.0f;
  m_colorGrad << Vec(140,81,10)/255.0f;

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
Viewer::start()
{
  m_tiles.clear();
  m_orderedTiles.clear();
  m_pointClouds.clear();

  genColorMap();

  m_volume = m_volumeFactory->topVolume();

  emit switchVolume();

  m_dpv = m_volume->dataPerVertex();
  m_maxTime = m_volume->maxTime();

  m_pointClouds = m_volume->pointClouds();

  if (m_vr.vrEnabled())
    {
      if (m_maxTime > 0)
	m_vr.setShowTimeseriesMenu(true);
      else
	m_vr.setShowTimeseriesMenu(false);
    }

  m_showSphere = m_pointClouds[0]->showSphere();
  m_pointType = m_pointClouds[0]->pointType();
  
  createMinMaxTexture();

  for(int d=0; d<m_pointClouds.count(); d++)
    {
      QList<OctreeNode*> allNodes = m_pointClouds[d]->allNodes();
      for(int od=0; od<allNodes.count(); od++)
	allNodes[od]->setColorMap(m_colorGrad);
    }

  m_coordMin = m_volume->coordMin();
  m_coordMax = m_volume->coordMax();
  m_npoints = m_volume->numPoints();

  //--------------------
  QVector3D cmin, cmax;
  cmin = QVector3D(m_coordMin.x,m_coordMin.y,m_coordMin.z);
  cmax = QVector3D(m_coordMax.x,m_coordMax.y,m_coordMax.z);
  if (m_vr.vrEnabled())
    m_vr.initModel(cmin, cmax);
  //--------------------

  if (m_vr.vrEnabled())
    {
      m_vr.setShowMap(m_pointClouds[0]->showMap());
      m_vr.setGravity(m_pointClouds[0]->gravity());
      m_vr.setGroundHeight(m_pointClouds[0]->groundHeight());
      m_vr.setTeleportScale(m_pointClouds[0]->teleportScale());
      //  if (m_volume->timeseries())
      //    m_vr.setShowTimeseriesMenu(true);
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
  float flyspeed = sceneRadius()*0.0001;
  camera()->setFlySpeed(flyspeed);

  camera()->setZNearCoefficient(0.01);
  camera()->setZClippingCoefficient(1);


  m_firstImageDone = 0;
  m_vboLoadedAll = false;

  if (m_vr.vrEnabled())
    {
      m_menuCam.setScreenWidthAndHeight(m_vr.screenWidth(), m_vr.screenHeight());
      m_menuCam.setSceneBoundingBox(m_coordMin, m_coordMax);
      m_menuCam.setType(Camera::ORTHOGRAPHIC);
      m_menuCam.showEntireScene();

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

  if (m_vr.vrEnabled())
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
      QMessageBox::information(0, "", "..");
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
  //--------------------------
}

void
Viewer::keyPressEvent(QKeyEvent *event)
{
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

//----------------------------
  if (event->key() == Qt::Key_F)
    {
      m_pointType = !m_pointType;
      update();
      return;
    }  
//----------------------------

  if (event->key() == Qt::Key_1)
    {
      m_showEdges = !m_showEdges;
      //createShaders();
      update();
      return;
    }  

  if (event->key() == Qt::Key_2)
    {
      m_softShadows = !m_softShadows;
      update();
      return;
    }  

  if (event->key() == Qt::Key_3)
    {
      m_smoothDepth = !m_smoothDepth;
      update();
      return;
    }  

  if (event->key() == Qt::Key_4)
    {
      m_showSphere = !m_showSphere;
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
  if (m_vr.vrEnabled())
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
  if (m_vr.vrEnabled())
    return;

  if (m_pointClouds.count() == 0)
    return;
  
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
      if (found)
	{
	  if (!m_savePointsToFile)
	    {
	      QFileInfo fi(m_volume->filenames()[0]);
	      QString flnmOut = QFileDialog::getSaveFileName(0,
							     "Save CSV",
							     fi.absolutePath(),
							     "Files (*.csv)",
							     0,
							     QFileDialog::DontUseNativeDialog);
	      
	      if (!flnmOut.isEmpty())
		{
		  m_savePointsToFile = true;
		  m_pFile.setFileName(flnmOut);
		}
	    }

	  if (m_savePointsToFile)
	    savePointsToFile(newp);
	}
      m_selectActive = false;
    }
  //------------------------------------------

  QGLViewer::mousePressEvent(event);

  //if (m_flyMode)
  m_flyTime.restart();
}

void
Viewer::mouseReleaseEvent(QMouseEvent *event)
{
  if (m_pointClouds.count() == 0)
    return;

  if (m_vr.vrEnabled())
    {
      genDrawNodeListForVR();
      return;
    }
  
  m_fastDraw = false;

  QGLViewer::mouseReleaseEvent(event);

  // generate new node list only when mouse released
  genDrawNodeList();
}

void
Viewer::mouseMoveEvent(QMouseEvent *event)
{
  if (m_vr.vrEnabled())
    return;

  if (m_pointClouds.count() == 0)
    return;


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
Viewer::dummydraw()
{
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  drawAABB();
}

void
Viewer::paintGL()
{
  glClearColor(0,0,0,0);

  if (!m_vr.vrEnabled() ||
      m_pointClouds.count() == 0)
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

  m_vr.preDraw();

  if (m_vr.genDrawList())
    {
      if (m_vr.nextStep() != 0)
	{
	  m_currTimeChanged = true;
	  m_currTime = m_currTime + m_vr.nextStep();
	  if (m_currTime > m_maxTime) m_currTime = 0;
	  if (m_currTime < 0) m_currTime = m_maxTime;	  
	  m_vr.resetNextStep();

	  m_vr.setTimeStep(QString("Step %1").arg(m_currTime));

	  if (m_pointClouds[0]->showMap())
	    m_firstImageDone = 0;
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


  if (m_pointClouds[0]->showMap())
    m_vr.sendCurrPosToMenu();

  //---------------------------

  if (m_vbPoints == 0)
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
      //drawPoints(vr::Eye_Left);
      drawPointsWithShadows(vr::Eye_Left);
      drawLabelsForVR(vr::Eye_Left);
      //if (m_showBox)
      //  drawAABB();
      //drawInfo();	
      m_vr.postDrawLeftBuffer();
      
      
      // right eye
      m_vr.bindRightBuffer();
      //drawPoints(vr::Eye_Right);
      drawPointsWithShadows(vr::Eye_Right);
      drawLabelsForVR(vr::Eye_Right);
      //if (m_showBox)
      //  drawAABB();
      //drawInfo();
      m_vr.postDrawRightBuffer();
    }

  m_vr.postDraw();

  m_frames++;

  //---------------------------
  if (m_vboLoadedAll &&
      m_firstImageDone < 2 &&
      m_pointClouds[0]->showMap())
    {
      generateFirstImage();
      m_firstImageDone++;
    }  
  //---------------------------

  update();
}

void
Viewer::draw()
{
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_npoints <= 0)
    return;

  if (m_fastDraw)
    {
      fastDraw();
      m_fastDraw = false;
      return;
    }

  drawPointsWithReload();
  drawLabels();
  if (m_showBox)
    drawAABB();
  drawInfo();
}

void
Viewer::fastDraw()
{
  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (m_npoints <= 0)
    return;

  drawPointsWithReload();
  
  drawLabels();

  if (m_showBox)
    drawAABB();

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
  if (!m_showInfo &&
      m_volumeFactory->stackSize() == 0)
    return;
  

  glDisable(GL_DEPTH_TEST);

  int ptsz = m_pointSize;

  QString mesg;

  QFont tfont = QFont("Helvetica", 10);
  tfont.setStyleStrategy(QFont::PreferAntialias);  
  
  startScreenCoordinatesSystem();
  
  if (m_showInfo)
    {
      if (m_maxTime > 0)
	mesg += QString("Time : %1    ").arg(m_currTime);
      
      mesg += QString("Fps : %1    ").arg(currentFPS());
      mesg += QString("PtSz : %1    ").arg(ptsz);
      mesg += QString("Points : %1 (%2)  ").arg(m_pointBudget).arg(m_vbPoints);
      mesg += QString("mNPS : %1").arg(m_minNodePixelSize);
      mesg += QString(" : %1 %2").arg(m_nearDist).arg(m_farDist);

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
Viewer::loadNodeData()
{
  m_vbID = 0;

  qint64 lpoints = 0;  

  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[m_vbID]);
  for(int i=0; i<m_loadNodes.count(); i++)
    {
      m_loadNodes[i]->loadData();
      qint64 npts = m_loadNodes[i]->numpoints();
      
      glBufferSubData(GL_ARRAY_BUFFER,
		      m_dpv*lpoints*sizeof(float),
		      m_dpv*npts*sizeof(float),
		      m_loadNodes[i]->coords());

      lpoints += npts;
    }

  m_vbPoints = lpoints;
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
}
void
Viewer::vboLoadedAll(int cvp, qint64 npts)
{
  if (npts > -1)
    {
      m_vbID = cvp;
      m_vbPoints = npts;
    }
  
  if (!m_vr.vrEnabled())
    {
      update();
      return;
    }

  if (m_vr.play())
    m_vr.gotoNextStep();

  m_vboLoadedAll = true;
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

  //glUniform1i(m_shadowParm[3], m_showEdges); // showedges
  //glUniform1i(m_shadowParm[4], m_softShadows); // softShadows
  glUniform1i(m_shadowParm[3], m_vr.edges()); // showedges
  glUniform1i(m_shadowParm[4], m_vr.softShadows()); // softShadows

  glUniform1f(m_shadowParm[5], m_nearDist);
  glUniform1f(m_shadowParm[6], m_farDist);

  glUniform1i(m_shadowParm[7], m_showSphere);

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

  glUniform1i(m_depthParm[14], 1); // shadows
  
  glUniform1f(m_depthParm[15], camera()->zFar()); // zfar

  glUniform1f(m_depthParm[16], 2); // max point size
  glUniform1f(m_depthParm[17], 20); // max point size

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
				     mvp);
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
  QMatrix4x4 mvp = m_vr.viewProjection(eye);
  QMatrix4x4 matR = m_vr.matrixDevicePoseRight();
  QVector3D cpos = m_hmdPos;
  QMatrix4x4 finalxform = m_vr.final_xform();
  
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_pointClouds[d]->visible())
	m_pointClouds[d]->drawLabels(cpos,
				     m_hmdVD,
				     m_hmdUP,
				     m_hmdRV,
				     mvp, matR,
				     finalxform);
    }
}

void
Viewer::drawAABB()
{
  //glDisable(GL_DEPTH_TEST);
  glDisable(GL_LIGHTING);
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  

  glColor3f(1,1,1);
  glLineWidth(1.0);
  StaticFunctions::drawBox(m_coordMin, m_coordMax);

  for(int d=0; d<m_pointClouds.count(); d++)
    m_pointClouds[d]->drawInactiveTiles();
  

  glLineWidth(2.0);
  for(int d=0; d<m_pointClouds.count(); d++)
    m_pointClouds[d]->drawActiveNodes();

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glLineWidth(1.0);
  glEnable(GL_DEPTH_TEST);
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
      if (m_pointClouds[d]->time() != -1 &&
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

  // single threaded
  //loadNodeData();

  //createMinMaxTexture();

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

      bool addNode = isVisible(bmin, bmax);

      if (addNode && zfar > 0.0)
	addNode = nearCam(cpos, viewDir, bmin, bmax, zfar);
      
      if (addNode)
	{
	  float screenProjectedSize = StaticFunctions::projectionSize(cpos,
								      bmin, bmax,
								      projFactor);
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
Viewer::loadLink(QString dirname)
{
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
  m_volume->setNewLoad(true);

  emit updateView();

  emit message("viewer - update view");
}
