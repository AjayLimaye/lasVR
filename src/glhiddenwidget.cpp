#include "glhiddenwidget.h"
#include "staticfunctions.h"
#include "global.h"

#include <QMessageBox>
#include <QApplication>
#include <QtMath>

GLHiddenWidget::GLHiddenWidget(QGLFormat format,
			       QWidget *parent,
			       QGLWidget *shareWidget):
  QGLWidget(format, parent, shareWidget)
{
//  QGLContext *glContext = new QGLContext(format, this);
//  if (shareWidget != NULL)
//    glContext->create(shareWidget->context());
//  setContext(glContext);
  
  makeCurrent();
  
  setAutoBufferSwap(false);
  
  m_loading = false;
  m_stopLoading = false;
  
  m_vertexBuffer[0] = 0;
  m_vertexBuffer[1] = 0;

  m_visibilityTex = 0;
  m_visibilityMap = 0;

  // emit vboLoaded every time m_pointBlockSize points are uploaded to gpu
  m_pointsDrawn = 0;
  m_pointBlockSize = 50000;
  m_minNodePixelSize = 100;

  m_prevNodes.clear();
  m_newNodes.clear();

  m_vr = 0;
  m_viewer = 0;
  m_volumeFactory = 0;

  m_currTime = 0;
  m_newVisTex = false;

  m_firstLoad = true;
}

void
GLHiddenWidget::setViewer(Viewer *viewer)
{
  m_viewer = viewer;
  m_vr = m_viewer->vrPointer();
  m_volumeFactory = m_viewer->volumeFactory();

  m_pointBudget = m_viewer->pointBudget();

  m_firstLoad = true;
}

void
GLHiddenWidget::setVBOs(GLuint vb0, GLuint vb1)
{
  m_vertexBuffer[0] = vb0;
  m_vertexBuffer[1] = vb1;

  m_pointBudget = m_viewer->pointBudget();
  m_prevNodes.clear();
  m_firstLoad = true;
}

void
GLHiddenWidget::setVisTex(GLuint vt) 
{
  m_visibilityTex = vt;
}

void
GLHiddenWidget::setVolume(Volume *v)
{
  m_volume = v;
  m_prevNodes.clear();
  m_dpv = m_volume->dataPerVertex();

  m_pointClouds = m_volume->pointClouds();
  m_trisets = m_volume->trisets();

  int ht;
  if (m_viewer->vrMode() && m_vr->vrEnabled())
    {
      ht = m_vr->screenHeight();
      m_fov = qDegreesToRadians(110.0); // FOV is 110 degrees for HTC Vive
      //m_fov = qDegreesToRadians(55.0); // FOV is 110 degrees for HTC Vive
    }
  else
    {
      ht = m_viewer->camera()->screenHeight();
      m_fov = m_viewer->camera()->fieldOfView();
    }
  m_slope = qTan(m_fov/2);
  m_projFactor = (0.5f*ht)/m_slope;
  

  m_firstLoad = true;
}

GLHiddenWidget::~GLHiddenWidget()
{
}

void GLHiddenWidget::glInit()
{
}

void GLHiddenWidget::glDraw()
{
}

void GLHiddenWidget::initializeGL()
{
}

void GLHiddenWidget::resizeGL(int width, int height)
{
}

void GLHiddenWidget::paintGL()
{
}

void GLHiddenWidget::paintEvent(QPaintEvent *)
{
}

void GLHiddenWidget::resizeEvent(QResizeEvent *event)
{
}

void
GLHiddenWidget::uploadVisTex()
{
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
	       m_maxwd/4, m_ntiles,
	       0,
	       GL_RGBA,
	       GL_UNSIGNED_BYTE,
	       m_visibilityMap); 
  
  m_newVisTex = false;
}

void
GLHiddenWidget::createVisibilityTexture()
{
  if (m_pointClouds.count() == 0)
    return;
  
  // update visibility for pointcloud tiles
  for(int d=0; d<m_pointClouds.count(); d++)
    m_pointClouds[d]->updateVisibilityData();
  
  QList< QList<uchar> > m_vData;
  m_vData.clear();

  for(int d=0; d<m_pointClouds.count(); d++)
    m_vData += m_pointClouds[d]->vData();


  m_ntiles = m_vData.count();
  m_maxwd = 0;
  for(int i=0; i<m_ntiles; i++)
    m_maxwd = qMax(m_maxwd, m_vData[i].count());
  

  if (m_visibilityMap) delete [] m_visibilityMap;
  m_visibilityMap = new uchar[m_maxwd*m_ntiles];
  memset(m_visibilityMap, 0, m_maxwd*m_ntiles);
    
  for(int i=0; i<m_ntiles; i++)
    {
      QList<uchar> vS = m_vData[i];
      
      for(int k=0; k<vS.count(); k++)
	m_visibilityMap[i*m_maxwd + k] = vS[k];
    }

  m_newVisTex = true;
}

void
GLHiddenWidget::switchVolume()
{
  m_volume = m_volumeFactory->popVolume();
  m_prevNodes.clear();
  m_dpv = m_volume->dataPerVertex();

  m_currVBO = 0;

  m_pointClouds = m_volume->pointClouds();
  m_trisets = m_volume->trisets();

  int ht;
  if (m_viewer->vrMode() && m_vr->vrEnabled())
    {
      ht = m_vr->screenHeight();
      m_fov = qDegreesToRadians(110.0); // FOV is 110 degrees for HTC Vive
    }
  else
    {
      ht = m_viewer->camera()->screenHeight();
      m_fov = m_viewer->camera()->fieldOfView();
    }

  m_slope = qTan(m_fov/2);
  m_projFactor = (0.5f*ht)/m_slope;

  m_firstLoad = true;
}

void
GLHiddenWidget::stopLoading()
{
  if (m_loading)
    {
      m_mutex.lock();
      m_stopLoading = true;
      m_mutex.unlock();
    }
}

void
GLHiddenWidget::removeEditedNodes()
{
  if (m_prevNodes.count() > 0)
    {
      QMap<int, QPair<qint64, qint64> > newLoad;
      int xid = m_volume->xformNodeId();
      QList<int> keys = m_prevNodes.keys();
      for(int i=0; i<keys.count(); i++)
	{
	  if (keys[i] < xid)
	    newLoad[keys[i]] = m_prevNodes[keys[i]];
	}

      m_prevNodes = newLoad;
    }
}

void
GLHiddenWidget::loadPointsToVBO()
{
  if (m_currVBO < 0) return;
  if (m_vertexBuffer[0] <= 0) return;

  //----------------------
  // copy relevant data from vbo1 to vbo2
  GLuint vbo1 = (m_currVBO+1)%2;
  GLuint vbo2 = m_currVBO;

  qint64 lpoints = 0;

  // read data from vbo1 
  glBindBuffer(GL_COPY_READ_BUFFER, m_vertexBuffer[vbo1]);

  // write data into vbo1 
  glBindBuffer(GL_COPY_WRITE_BUFFER, m_vertexBuffer[vbo2]);

  // get current node data to load
  QList<OctreeNode*> currload;
  if (m_viewer->vrMode() && m_vr->vrEnabled())
    currload = m_newNodes;
  else
    currload = m_volume->getLoadingNodes();
  QList<OctreeNode*> loadNodes;
  
  if (m_viewer->editMode())
    {
      removeEditedNodes();
      //m_prevNodes.clear();
    }
  
  //-------------------------------
  if (m_prevNodes.count() > 0)
    {
      for(int i=0; i<currload.count(); i++)
	{
	  int nodeId = currload[i]->uid();
	  if (!m_prevNodes.contains(nodeId)) // save it to load next
	    loadNodes << currload[i];
	}

      // no nodes to load,
      // check if we need to unload some nodes
      if (loadNodes.count() == 0)
	{
	  if (m_prevNodes.count() == currload.count())
	    {
	      if (Global::playFrames())
		emit vboLoadedAll(m_currVBO, -1);
	      return;
	    }
	  // else we need to unload some nodes
	}
      
//      // no nodes to load, so just return
//      if (loadNodes.count() == 0)
//	{
//	  if (Global::playFrames())
//	    emit vboLoadedAll(m_currVBO, -1);
//	  return;
//	}
    }
  else
    loadNodes = currload;
  //-------------------------------
  
  if (m_volume->newLoad() && !m_firstLoad)
    {
      return;
    }


  QMap<int, QPair<qint64, qint64> > newLoad;


  //-------------------------------
  for(int i=0; i<currload.count(); i++)
    {
      if (m_volume->newLoad() && !m_firstLoad)
	{
	  glFinish();
	  emit vboLoaded(m_currVBO, lpoints);
	  m_currVBO = (m_currVBO+1)%2;
	  m_prevNodes = newLoad;

	  if (m_newVisTex)
	    uploadVisTex();

	  return;
	}

      int nodeId = currload[i]->uid();
      if (m_prevNodes.contains(nodeId))
	{
	  QPair<qint64, qint64> qp = m_prevNodes[nodeId];
	  qint64 start = qp.first;
	  qint64 npts = qp.second;

	  if (m_dpv == 3)
	    glCopyBufferSubData(GL_COPY_READ_BUFFER,
				GL_COPY_WRITE_BUFFER,
				m_dpv*start*sizeof(float), // read offset
				m_dpv*lpoints*sizeof(float), // write offset
				m_dpv*npts*sizeof(float)); // size			      
	  else
	    glCopyBufferSubData(GL_COPY_READ_BUFFER,
				GL_COPY_WRITE_BUFFER,
				20*start, // read offset
				20*lpoints, // write offset
				20*npts); // size			      
//	    glCopyBufferSubData(GL_COPY_READ_BUFFER,
//				GL_COPY_WRITE_BUFFER,
//				16*start, // read offset
//				16*lpoints, // write offset
//				16*npts); // size			      


	  //-----------------
	  // save info for next frame load
	  qp = qMakePair(lpoints, npts);
	  newLoad[nodeId] = qp;
	  //-----------------

	  lpoints += npts;
	}
    }
  currload.clear();
  //-------------------------------

  
  //-------------------------------
  if (lpoints > 0)
    {
      glFinish();
      emit vboLoaded(m_currVBO, lpoints);

      if (m_newVisTex)
	uploadVisTex();

    }
  //----------------------


  // emit vboLoaded every time m_pointBlockSize points are uploaded to gpu
  int blk = 1;
  qint64 blkpts = 0;
      
  glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer[m_currVBO]);

  // load data for remaining OctreeNodes
  for(int i=0; i<loadNodes.count(); i++)
    {
      if (m_volume->newLoad() && !m_firstLoad)
	{
	  break;
	}

      loadNodes[i]->loadData();
      qint64 npts = loadNodes[i]->numpoints();
      
      if (npts > 0)
	{
	  if (m_dpv == 3)
	    glBufferSubData(GL_ARRAY_BUFFER,
			    m_dpv*lpoints*sizeof(float),
			    m_dpv*npts*sizeof(float),
			    loadNodes[i]->coords());
	  else
	    glBufferSubData(GL_ARRAY_BUFFER,
			    20*lpoints,
			    20*npts,
			    loadNodes[i]->coords());
//	    glBufferSubData(GL_ARRAY_BUFFER,
//			    16*lpoints,
//			    16*npts,
//			    loadNodes[i]->coords());
	}
      

      //-----------------
      // save to info for next load
      if (!m_viewer->editMode() ||
	  loadNodes[i]->id() < m_volume->xformNodeId())
	{
	  int nodeId = loadNodes[i]->uid();
	  newLoad[nodeId] = qMakePair(lpoints, npts);
	}
      //-----------------

      lpoints += npts;
      
      blkpts += npts;
      if (blkpts > blk*m_pointBlockSize)
	{
	  glFinish();
	  emit vboLoaded(m_currVBO, lpoints);
	  blk ++;

	  if (m_newVisTex)
	    uploadVisTex();

	}
    }

  if (m_newVisTex)
    uploadVisTex();

  glFinish();
  emit vboLoadedAll(m_currVBO, lpoints);

  m_firstLoad = false;
    
  m_currVBO = (m_currVBO+1)%2;


  // update prevNodes
  m_prevNodes = newLoad;
}


//--------------------------------------------
// generate list of nodes to upload
//--------------------------------------------
void
GLHiddenWidget::updateView()
{
  m_currTime = m_viewer->currentTime();

  if (m_trisets.count() > 0)
    {
      bool loadedAll = true;
      for(int i=0; i<m_trisets.count(); i++)
	{
	  if (m_trisets[i]->time() == -1 ||
	      m_trisets[i]->time() == m_currTime)
	    {
	      GLuint va, vb, ib;
	      va = Global::trisetVBO(i,0);
	      vb = Global::trisetVBO(i,1);
	      ib = Global::trisetVBO(i,2);
	      if (!m_trisets[i]->loadVertexBufferData(va, vb, ib))
		loadedAll = false;
	    }
	}
      if (loadedAll)
	emit meshLoadedAll();
    }
  
  if (m_pointClouds.count() == 0)
    return;
  
  genDrawNodeList();

  m_volume->setNewLoad(false);

  createVisibilityTexture();

  makeCurrent();
  loadPointsToVBO();
  doneCurrent();
}

void
GLHiddenWidget::genDrawNodeList()
{
  if (m_pointClouds.count() == 0)
    return;
  
  Vec cpos;

  QVector3D hp;
  if (m_viewer->vrMode() && m_vr->vrEnabled())
    {
      QVector3D hp = m_vr->vrHmdPosition();
      cpos = Vec(hp.x(),hp.y(),hp.z());

      if (m_firstLoad)
	cpos = m_viewer->menuCamPos();
    }
  else
    cpos = m_viewer->camera()->position();


  
  orderTiles(cpos);


  for(int d=0; d<m_pointClouds.count(); d++)
    {
      QList<OctreeNode*> allNodes = m_pointClouds[d]->allNodes();
      for(int od=0; od<allNodes.count(); od++)
	allNodes[od]->setActive(false);
    }


  genPriorityQueue(cpos);


  //-------------------------------
  // sort based on projection size
  m_pointsDrawn = 0;
  m_newNodes.clear();
  QList<float> keys = m_priorityQueue.keys();

  bool bufferFull = false;
  for(int k=keys.count()-1; k>=0 && !bufferFull; k--)
    {
      QList<OctreeNode*> values = m_priorityQueue.values(keys[k]);
      for(int v=0; v<values.count() && !bufferFull; v++)
	{
	  if (m_pointsDrawn + values[v]->numpoints() < m_pointBudget)
	    {
	      m_pointsDrawn += values[v]->numpoints();
	      m_newNodes << values[v];
	    }
	  else
	    bufferFull = true;
	}
    }
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      QList<OctreeNode*> allNodes = m_pointClouds[d]->allNodes();
      for(int od=0; od<allNodes.count(); od++)
	allNodes[od]->setActive(false);
    }
  for(int i=0; i<m_newNodes.count(); i++)
    m_newNodes[i]->setActive(true);
  //-------------------------------



  //-------------------------------
  float nearDist = -1;
  float farDist = -1;
  for (int l=0; l<m_newNodes.count(); l++)
    {
      for (int c=0; c<8; c++)
	{
	  Vec bmin = m_newNodes[l]->tightOctreeMin();
	  Vec bmax = m_newNodes[l]->tightOctreeMax();
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
  m_viewer->setNearFar(nearDist, farDist);
}

//void
//GLHiddenWidget::genPriorityQueue(Vec cpos)
//{
//  QMatrix4x4 MV;
//
//  if (m_viewer->vrMode() && m_vr->vrEnabled())
//    {
//      MV = m_vr->modelView();
//    }
//  else
//    {
//      float m[16];
//      m_viewer->camera()->getModelViewMatrix(m);
//      for(int i=0; i<16; i++)
//	MV.data()[i] = m[i];
//      MV = MV.transposed();
//    }
//
//		  
//  m_pointsDrawn = 0;
//  m_newNodes.clear();
//  m_priorityQueue.clear();
//
//  // consider all top-level tile nodes first
//  for(int d=0; d<m_orderedTiles.count(); d++)
//    {
//      OctreeNode *node = m_orderedTiles[d];
//      
//      Vec bmin = node->bmin();
//      Vec bmax = node->bmax(); 
//
//      QVector4D bminV = MV * QVector4D(bmin.x, bmin.y, bmin.z, 1);
//      QVector4D bmaxV = MV * QVector4D(bmax.x, bmax.y, bmax.z, 1);
//      bmin = Vec(bminV.x(), bminV.y(), bminV.z());
//      bmax = Vec(bmaxV.x(), bmaxV.y(), bmaxV.z());
//      
//      float screenProjectedSize = 100000;
//      screenProjectedSize = StaticFunctions::projectionSize(cpos,
//							    bmin, bmax,
//							    m_projFactor);
//      
//      m_priorityQueue.insert(screenProjectedSize, node);
//    }
//
//  QStack<OctreeNode*> nodeStack;
//  QList<float> keys = m_priorityQueue.keys();
//  for(int k=0; k<keys.count(); k++)
//    {
//      QList<OctreeNode*> values = m_priorityQueue.values(keys[k]);
//      for(int v=0; v<values.count(); v++)
//	{
//	  nodeStack.push(values[k]);
//	}
//    }
//
//  // now consider the nodes within each tile
//  while (!nodeStack.isEmpty() > 0)
//    {
//      OctreeNode *node = nodeStack.pop();
//      Vec bmin = node->bmin();
//      Vec bmax = node->bmax();			  
//      
//      QVector4D bminV = MV * QVector4D(bmin.x, bmin.y, bmin.z, 1);
//      QVector4D bmaxV = MV * QVector4D(bmax.x, bmax.y, bmax.z, 1);
//      bmin = Vec(bminV.x(), bminV.y(), bminV.z());
//      bmax = Vec(bmaxV.x(), bmaxV.y(), bmaxV.z());
//      
//      float screenProjectedSize = StaticFunctions::projectionSize(cpos,
//								  bmin, bmax,
//								  m_projFactor);		  
//      if (screenProjectedSize > m_minNodePixelSize)
//	{
//	  if (m_pointsDrawn + node->numpoints() < m_pointBudget)
//	    {
//	      node->setActive(true);
//	      m_newNodes << node;
//	      m_pointsDrawn += node->numpoints();
//
//	      for (int k=0; k<8; k++)
//		{
//		  OctreeNode *cnode = node->getChild(k);
//		  if (cnode)
//		    {
//		      nodeStack.push(cnode);
//		    }
//		}
//	    }
//	  else
//	    {
//	      nodeStack.clear();
//	      break;
//	    }
//	}
//
//    }
//}

void
GLHiddenWidget::genPriorityQueue(Vec cpos)
{
  QMatrix4x4 MV;

  if (m_viewer->vrMode() && m_vr->vrEnabled())
    {
      MV = m_vr->modelView();
    }
  else
    {
      float m[16];
      m_viewer->camera()->getModelViewMatrix(m);
      for(int i=0; i<16; i++)
	MV.data()[i] = m[i];
      MV = MV.transposed();
    }

  MV = MV.inverted();
  QVector4D cpMV = MV * QVector4D(cpos.x, cpos.y, cpos.z, 1);
  Vec cposMV = Vec(cpMV.x(), cpMV.y(), cpMV.z());
		  
  m_pointsDrawn = 0;
  m_priorityQueue.clear();

  QList<OctreeNode*> onl0;

  // consider all top-level tile nodes first
  for(int d=0; d<m_orderedTiles.count(); d++)
    {
      OctreeNode *node = m_orderedTiles[d];
      
      Vec bmin = node->bmin();
      Vec bmax = node->bmax(); 
      
      float screenProjectedSize = StaticFunctions::projectionSize(cposMV,
								  bmin, bmax,
								  m_projFactor);
      bool ignoreChildren = (screenProjectedSize < m_minNodePixelSize);
      
      if (!ignoreChildren)
	onl0 << node;
      
      if (!node->isActive())
	{
	  m_priorityQueue.insert(screenProjectedSize, node);
	  node->setActive(true);
	}
    }

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

		  float screenProjectedSize = StaticFunctions::projectionSize(cposMV,
									      bmin, bmax,
									      m_projFactor);
		  
//		  bool ignoreChildren = (screenProjectedSize < m_minNodePixelSize);
//		  
//		  if (!ignoreChildren)
		    onl1 << cnode;
		  
		  if (!cnode->isActive())
		    {
		      m_priorityQueue.insert(screenProjectedSize, cnode);
		      cnode->setActive(true);
		    }
		} // valid child
	    } // loop over child nodes
	} // loop over onl0
      
      if (!done)
	onl0 = onl1;
      else
	break;
    }
}

void
GLHiddenWidget::orderTiles(Vec cpos)
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

  m_orderedTiles = m_tiles;
}
