#include "vrmain.h"
#include "global.h"
#include <QtGui>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QWidgetAction>


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

  connect(m_viewer, SIGNAL(removeEditedNodes()),
	  m_hiddenGL, SLOT(removeEditedNodes()));

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



  //============================
  m_pointBudget = new PopUpSlider(this, Qt::Horizontal);
  m_pointBudget->setText("Point Budget (Mil)");
  m_pointBudget->setRange(1, 20);

  qint64 million = 1000000; 
  int pb = m_viewer->pointBudget()/million;
  m_pointBudget->setValue(pb);

  connect(m_pointBudget, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setPointBudget(int)));

  QWidgetAction *ptBudget = new QWidgetAction(ui.menuOptions);
  ptBudget->setDefaultWidget(m_pointBudget);
  ui.menuOptions->addAction(ptBudget);
  //============================
  
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
VrMain::on_actionFly_triggered()
{
  m_viewer->setFlyMode(ui.actionFly->isChecked());
}

void
VrMain::on_actionLoad_LAS_triggered()
{
//  QStringList flnms = QFileDialog::getOpenFileNames(0,
//						    "Load PoTree Directory",
//						    "",
//						    "Dir",
//						    0,
//						    QFileDialog::DontUseNativeDialog);
//  
//  if (flnms.isEmpty())
//    return;
//

  QString dirname = QFileDialog::getExistingDirectory(0,
						      "Load PoTree Directory",
						      "",
						      QFileDialog::ShowDirsOnly |
						      QFileDialog::DontResolveSymlinks);

  if (dirname.isEmpty())
    return;

  
  QStringList flnms;
  flnms << dirname;

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
  QStringList slst = mesg.split(" ");
  if (slst[0] == "VRMode")
    {
      if (slst[1].toInt() == 0)
	{
	  ui.actionVRMode->setEnabled(false);
	  ui.actionVRMode->setChecked(false);
	  ui.toolBar->show();
	}
      else
	{
	  ui.actionVRMode->setEnabled(true);
	  ui.actionVRMode->setChecked(true);
	  ui.toolBar->hide();
	}

      setWindowTitle("Start");
    }
  else
    setWindowTitle(mesg);
}

void VrMain::on_actionEditMode_triggered() { m_viewer->toggleEditMode(); }
void VrMain::on_actionCamMode_triggered() { m_viewer->toggleCamMode(); }
void VrMain::on_actionAlign_triggered() { m_viewer->centerPointClouds(); }
void VrMain::on_actionUndo_triggered() { m_viewer->undo(); }
void VrMain::on_actionSaveInfo_triggered() { m_viewer->saveModInfo(); }
void
VrMain::on_actionVRMode_triggered()
{
  m_viewer->setVRMode(ui.actionVRMode->isChecked());

  if (ui.actionVRMode->isChecked())
    {
      if (ui.actionEditMode->isChecked())
	{
	  ui.actionEditMode->setChecked(false);
	  m_viewer->toggleEditMode();
	}

      if (ui.actionCamMode->isChecked())
	{
	  ui.actionCamMode->setChecked(false);
	  m_viewer->toggleCamMode();
	}

      ui.toolBar->hide();
    }
  else
    ui.toolBar->show();
}
