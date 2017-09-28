#include "vrmain.h"
#include "global.h"
#include <QtGui>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPushButton>


VrMain::VrMain(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);

  setAcceptDrops(true);

  QGLFormat qglFormat;
  qglFormat.setProfile(QGLFormat::CoreProfile);

  m_viewer = new Viewer(qglFormat, this);

  centralWidget()->layout()->addWidget(m_viewer);

  setMouseTracking(true);

  m_viewer->setVolumeFactory(&m_volumeFactory);


  QTimer::singleShot(2000, m_viewer, SLOT(GlewInit()));

  //============================
  m_thread = new QThread();

  m_lt = new LoaderThread();
  m_lt->moveToThread(m_thread);
  m_lt->start(qglFormat, 0, m_viewer);

  m_hiddenGL = m_lt->gl();
  m_hiddenGL->context()->moveToThread(m_thread);

  m_hiddenGL->setViewer(m_viewer);

  connect(m_viewer, SIGNAL(switchVolume()),
	  m_hiddenGL, SLOT(switchVolume()));

  connect(m_viewer, SIGNAL(setVBOs(GLuint, GLuint)),
	  m_hiddenGL, SLOT(setVBOs(GLuint, GLuint)));

  connect(m_viewer, SIGNAL(setVisTex(GLuint)),
	  m_hiddenGL, SLOT(setVisTex(GLuint)));
  
  connect(m_viewer, SIGNAL(stopLoading()),
	      m_lt, SLOT(stopLoading()));

  connect(m_viewer, SIGNAL(loadPointsToVBO()),
	      m_lt, SLOT(loadPointsToVBO()));

  connect(m_viewer, SIGNAL(updateView()),
	      m_lt, SLOT(updateView()));

  connect(m_lt, SIGNAL(vboLoaded(int, qint64)),
	  m_viewer, SLOT(vboLoaded(int, qint64)));
  connect(m_lt, SIGNAL(vboLoadedAll(int, qint64)),
	  m_viewer, SLOT(vboLoadedAll(int, qint64)));
  connect(m_lt, &LoaderThread::message,
	  this, &VrMain::showMessage);

  m_thread->start();
  //============================

  connect(m_viewer, &Viewer::framesPerSecond,
	  this, &VrMain::showFramerate);
  connect(m_viewer, &Viewer::message,
	  this, &VrMain::showMessage);
}

void
VrMain::closeEvent(QCloseEvent*)
{
  on_actionQuit_triggered();
}

void
VrMain::dragEnterEvent(QDragEnterEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QList<QUrl> urls = data->urls();

	  QUrl url = urls[0];
	  QFileInfo info(url.toLocalFile());
	  if (info.exists() && info.isDir())
	    event->acceptProposedAction();
	  else if (info.exists() && info.isFile())
	    {
	      QString flnm = (data->urls())[0].toLocalFile();
	      if (flnm.endsWith(".las") || flnm.endsWith(".laz") ||
		  flnm.endsWith(".LAS") || flnm.endsWith(".LAZ"))
		event->acceptProposedAction();
	    }
	}
    }
}


void
VrMain::dropEvent(QDropEvent *event)
{
  if (event && event->mimeData())
    {
      const QMimeData *data = event->mimeData();
      if (data->hasUrls())
	{
	  QUrl url = data->urls()[0];
	  QFileInfo info(url.toLocalFile());
	  if (info.exists() && info.isDir())
	    {
	      QStringList flist;
	      QList<QUrl> urls = data->urls();
	      for(int i=0; i<urls.count(); i++)
		flist.append(urls[i].toLocalFile());

	      loadTiles(flist);
	    }
	}
    }
}


void
VrMain::keyPressEvent(QKeyEvent*)
{
}

void
VrMain::on_actionQuit_triggered()
{
  m_thread->quit();

  close();
}

void
VrMain::on_actionLoad_LAS_triggered()
{
  QStringList flnms = QFileDialog::getOpenFileNames(0,
						    "Load Directories",
						    "",
						    "Files",
						    0,
						    QFileDialog::DontUseNativeDialog);
  
  if (flnms.isEmpty())
    return;

  loadTiles(flnms);
}

void
VrMain::loadTiles(QStringList flnms)
{
  if (flnms.count() == 1)
    m_viewer->loadLink(flnms[0]);
  else
    {
      QMessageBox::information(0, "", "Only single directory allowed");
    }

//  Volume *vol = m_volumeFactory.newVolume();
//
//  if (flnms.count() == 1)
//    {
//      //if (!m_volume->loadDir(flnms[0]))
//      if (!vol->loadDir(flnms[0]))
//	{
//	  QMessageBox::information(0, "Error", "Cannot load tiles from directory");
//	  return;
//	}
//    }
//  else
//    {
//      //if (!m_volume->loadTiles(flnms))
//      if (!vol->loadTiles(flnms))
//	{
//	  QMessageBox::information(0, "Error", "Cannot load the tiles");
//	  return;
//	}
//    }    
//
//  m_viewer->start();
}

void
VrMain::showFramerate(float f)
{
  setWindowTitle(QString("frame rate %1").arg(f));
}
void
VrMain::showMessage(QString mesg)
{
  setWindowTitle(mesg);
}
