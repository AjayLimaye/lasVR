#include "vrmain.h"
#include "global.h"
#include "staticfunctions.h"
#include "saveimageseqdialog.h"
#include "savemoviedialog.h"

#include <QtGui>
#include <QMessageBox>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPushButton>
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

VrMain::VrMain(QWidget *parent) :
  QMainWindow(parent)
{
  ui.setupUi(this);

  setAcceptDrops(true);

  QGLFormat qglFormat;
  qglFormat.setProfile(QGLFormat::CoreProfile);


  Global::setStatusBar(ui.statusbar);
  ui.statusbar->setEnabled(true);
  ui.statusbar->setSizeGripEnabled(true);
  ui.statusbar->addPermanentWidget(Global::progressBar());
  Global::progressBar()->show();

  m_viewer = new Viewer(qglFormat, this);


  centralWidget()->layout()->addWidget(m_viewer);

  setMouseTracking(true);

  m_viewer->setVolumeFactory(&m_volumeFactory);


  //-------------------------
  m_dockTimeStamps = new QDockWidget("Information Editor", this);
  m_dockTimeStamps->setAllowedAreas(Qt::RightDockWidgetArea | 
				    Qt::LeftDockWidgetArea);
  
  m_tableWidget = new QTableWidget();
  m_tableWidget->verticalHeader()->setSectionsMovable(true);
  //m_tableWidget->verticalHeader()->setSectionsClickable(true);
  //m_tableWidget->verticalHeader()->hide();
  m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
  m_tableWidget->setShowGrid(true);
  m_tableWidget->setAlternatingRowColors(false);
  m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
  m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  QString str;
  str = "QHeaderView {font-size : 12pt; }";
  m_tableWidget->setStyleSheet(str);


  QWidget *wdg = new QWidget();
  
  QVBoxLayout *vbox = new QVBoxLayout();
  QHBoxLayout *hbox = new QHBoxLayout();

  QPushButton *sTs = new QPushButton("Save");
  connect(sTs, SIGNAL(clicked()),
	  this, SLOT(saveTimeStamps()));

  QPushButton *rld = new QPushButton("Reload");
  connect(rld, SIGNAL(clicked()),
	  this, SLOT(reloadData()));

  hbox->addWidget(sTs);
  hbox->addWidget(rld);
  vbox->addLayout(hbox);
  vbox->addWidget(m_tableWidget);

  wdg->setLayout(vbox);

  m_dockTimeStamps->setWidget(wdg);
  m_dockTimeStamps->hide();

  addDockWidget(Qt::LeftDockWidgetArea,m_dockTimeStamps);
  ui.menuProcess->addAction(m_dockTimeStamps->toggleViewAction());
  //-------------------------


  //-------------------------
  m_keyFrame = new KeyFrame();

  m_dockKeyframe = new QDockWidget("KeyFrame Editor", this);
  m_dockKeyframe->setAllowedAreas(Qt::BottomDockWidgetArea | 
        			  Qt::TopDockWidgetArea);
  m_keyFrameEditor = new KeyFrameEditor();
  m_dockKeyframe->setWidget(m_keyFrameEditor);
  m_dockKeyframe->hide();

  addDockWidget(Qt::BottomDockWidgetArea,m_dockKeyframe);

  ui.menuProcess->addAction(m_dockKeyframe->toggleViewAction());

  connect(m_viewer, SIGNAL(nextFrame()),
	  m_keyFrameEditor, SLOT(nextFrame()));
  connect(m_viewer, SIGNAL(nextKeyFrame()),
	  m_keyFrameEditor, SLOT(nextKeyFrame()));

  connect(m_keyFrame, SIGNAL(updateLookFrom(Vec, Quaternion, int)),
	  m_viewer, SLOT(updateLookFrom(Vec, Quaternion, int)));

  connect(m_keyFrame, SIGNAL(currentFrameChanged(int)),
	  m_viewer, SLOT(setCurrentFrame(int)));

  connect(m_keyFrame, SIGNAL(updateGL()),
	  m_viewer, SLOT(updateGL())); // 

  connect(m_keyFrame, SIGNAL(setImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));

  connect(m_keyFrame, SIGNAL(loadKeyframes(QList<int>, QList<QImage>)),
	  m_keyFrameEditor, SLOT(loadKeyframes(QList<int>, QList<QImage>)));  


  connect(m_keyFrame, SIGNAL(replaceKeyFrameImage(int)),
	  m_viewer, SLOT(captureKeyFrameImage(int)));
  connect(m_viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrameEditor, SLOT(setImage(int, QImage)));
  connect(m_viewer, SIGNAL(replaceKeyFrameImage(int, QImage)),
	  m_keyFrame, SLOT(replaceKeyFrameImage(int, QImage)));


  connect(m_keyFrameEditor, SIGNAL(updateGL),
	  m_viewer, SLOT(updateGL));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrame(int)),
	  m_viewer, SLOT(setKeyFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(endPlay()),
	  m_viewer, SLOT(endPlay()));

  connect(m_viewer, SIGNAL(setKeyFrame(Vec, Quaternion, int, QImage, int)),
	  m_keyFrame, SLOT(setKeyFrame(Vec, Quaternion, int, QImage, int)));

  connect(m_keyFrameEditor, SIGNAL(checkKeyFrameNumbers()),
	  m_keyFrame, SLOT(checkKeyFrameNumbers()));

  connect(m_keyFrameEditor, SIGNAL(reorder(QList<int>)),
	  m_keyFrame, SLOT(reorder(QList<int>)));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrameNumbers(QList<int>)),
	  m_keyFrame, SLOT(setKeyFrameNumbers(QList<int>)));

  connect(m_keyFrameEditor, SIGNAL(playFrameNumber(int)),
	  m_keyFrame, SLOT(playFrameNumber(int)));

  connect(m_keyFrameEditor, SIGNAL(setKeyFrameNumber(int, int)),
	  m_keyFrame, SLOT(setKeyFrameNumber(int, int)));

  connect(m_keyFrameEditor, SIGNAL(removeKeyFrame(int)),
	  m_keyFrame, SLOT(removeKeyFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(removeKeyFrames(int, int)),
	  m_keyFrame, SLOT(removeKeyFrames(int, int)));

  connect(m_keyFrameEditor, SIGNAL(copyFrame(int)),
	  m_keyFrame, SLOT(copyFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(pasteFrame(int)),
	  m_keyFrame, SLOT(pasteFrame(int)));

  connect(m_keyFrameEditor, SIGNAL(pasteFrameOnTop(int)),
	  m_keyFrame, SLOT(pasteFrameOnTop(int)));

  connect(m_keyFrameEditor, SIGNAL(pasteFrameOnTop(int, int)),
	  m_keyFrame, SLOT(pasteFrameOnTop(int, int)));

  connect(m_keyFrameEditor, SIGNAL(editFrameInterpolation(int)),
	  m_keyFrame, SLOT(editFrameInterpolation(int)));

  //-------------------------


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

  connect(m_lt, SIGNAL(meshLoadedAll()),
	  m_viewer, SIGNAL(meshLoadedAll()));

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
  
  //============================
  m_timeStep = new PopUpSlider(this, Qt::Horizontal);
  m_timeStep->setText("Time Step");
  m_timeStep->setRange(0, 0);

  m_timeStep->setValue(0);

  connect(m_timeStep, SIGNAL(valueChanged(int)),
	  m_viewer, SLOT(setTimeStep(int)));
  connect(m_viewer, SIGNAL(timeStepChanged(int)),
	  m_timeStep, SLOT(setValue(int)));

  QWidgetAction *timeStep = new QWidgetAction(ui.menuOptions);
  timeStep->setDefaultWidget(m_timeStep);
  ui.menuOptions->addAction(timeStep);
  //============================

  connect(m_viewer, SIGNAL(showToolbar()),
	  this, SLOT(showToolbar()));

    ui.toolBar->hide();
}

void VrMain::showToolbar() { ui.toolBar->show(); }

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
  if (!GlewInit::initialised())
    {
      QMessageBox::information(0, "", "Not Ready Yet");
      return;
    }

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
  QList<QDockWidget*> dw = findChildren<QDockWidget *>();
  for(int i=0; i<dw.count(); i++)
    {
      dw[i]->setFloating(false);
    }

  m_keyFrameEditor->setPlayFrames(false);

  m_thread->quit();

  delete m_keyFrameEditor;

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
    {
      ui.toolBar->hide();

      m_viewer->loadLink(flnms[0]);

      QFileInfo f(flnms[0]);
      Global::setPreviousDirectory(f.absolutePath());
    }
  else
    {
      QMessageBox::information(0, "", "Only single directory allowed");
    }

  m_timeStep->setRange(0, m_viewer->maxTimeStep());
  m_timeStep->setValue(0);

  modifyTableWidget();
}

void
VrMain::modifyTableWidget()
{
  
  while(m_tableWidget->rowCount() > 0)
    {
      m_tableWidget->removeRow(0);
    }
  
  m_tableWidget->setColumnCount(5);

  QStringList item;
  item << "Time";
  item << "Name";
  item << "Color";
  item << "Color Min Ht";
  item << "Color Max Ht";

  m_tableWidget->setHorizontalHeaderLabels(item);

  m_tableWidget->setColumnWidth(0, 45);
  //m_tableWidget->setColumnWidth(1, 70);
  m_tableWidget->setColumnWidth(2, 45);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
  m_tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Interactive);

  QList<PointCloud*> pcl = m_viewer->pointCloudList();
  for(int i=0; i<pcl.count(); i++)
    {
      m_tableWidget->insertRow(i);
      m_tableWidget->setRowHeight(i, 30);

      QString name = pcl[i]->name();
      int time = pcl[i]->time();
      float minHt, maxHt;
      pcl[i]->zBounds(minHt, maxHt);
      
      QTableWidgetItem *wi = new QTableWidgetItem(QString("%1").arg(time));
      wi->setTextAlignment(Qt::AlignCenter);
      m_tableWidget->setItem(i, 0, wi);
      
      wi = new QTableWidgetItem(name);
      m_tableWidget->setItem(i, 1, wi);

      wi = new QTableWidgetItem();
      if (pcl[i]->colorPresent())
	wi->setCheckState(Qt::Checked);
      else
	wi->setCheckState(Qt::Unchecked);
      m_tableWidget->setItem(i, 2, wi);

      wi = new QTableWidgetItem(QString("%1").arg(minHt));
      wi->setTextAlignment(Qt::AlignCenter);
      m_tableWidget->setItem(i, 3, wi);

      wi = new QTableWidgetItem(QString("%1").arg(maxHt));
      wi->setTextAlignment(Qt::AlignCenter);
      m_tableWidget->setItem(i, 4, wi);
    }
}


void
VrMain::saveTimeStamps()
{
  QList<PointCloud*> pcl = m_viewer->pointCloudList();
  for(int i=0; i<pcl.count(); i++)
    {
      int time = m_tableWidget->item(i, 0)->text().toInt();
      QString name = m_tableWidget->item(i, 1)->text();
      Qt::CheckState cp = m_tableWidget->item(i, 2)->checkState();
      float minHt = m_tableWidget->item(i, 3)->text().toFloat();
      float maxHt = m_tableWidget->item(i, 4)->text().toFloat();
      pcl[i]->setTime(time);
      pcl[i]->setName(name);
      pcl[i]->setColorPresent(cp == Qt::Checked);
      pcl[i]->setZBounds(minHt, maxHt);
      pcl[i]->saveModInfo("",false);
    }
  QMessageBox::information(0, "Save mod.json", "Saved information to respective mod.json files.");


  int maxTime = 0;
  for(int d=0; d<pcl.count(); d++)
    maxTime = qMax(maxTime, pcl[d]->time());

  m_timeStep->setRange(0, maxTime);
  m_timeStep->setValue(0);
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

      setWindowTitle("SRISHTI");
    }
  else
    setWindowTitle(mesg);
}

void VrMain::on_actionEditMode_triggered()
{
  ui.actionCamMode->setChecked(ui.actionEditMode->isChecked());
  m_viewer->setEditMode(ui.actionEditMode->isChecked());
  m_viewer->setCamMode(ui.actionCamMode->isChecked());
}
void VrMain::on_actionCamMode_triggered() { m_viewer->setCamMode(ui.actionCamMode->isChecked()); }
void VrMain::on_actionAlign_triggered() { m_viewer->alignUsingPointPairs(); }
void VrMain::on_actionCenter_triggered() { m_viewer->centerPointClouds(); }
void VrMain::on_actionUndo_triggered() { m_viewer->undo(); }
void VrMain::on_actionSaveInfo_triggered() { m_viewer->saveModInfo(); }
void VrMain::on_actionSoftShadow_triggered() { m_viewer->setSoftShadows(ui.actionSoftShadow->isChecked()); }
void VrMain::on_actionEdges_triggered() { m_viewer->setEdges(ui.actionEdges->isChecked()); }
void VrMain::on_actionSpheres_triggered() { m_viewer->setSpheres(ui.actionSpheres->isChecked()); }


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

void
VrMain::on_actionSave_ImageSequence_triggered()
{
  QSize imgSize = StaticFunctions::getImageSize(m_viewer->size().width(),
						m_viewer->size().height());
  SaveImageSeqDialog saveImg(0,
			     Global::previousDirectory(),
			     m_keyFrameEditor->startFrame(),
			     m_keyFrameEditor->endFrame(),
			     1);

  saveImg.move(QCursor::pos());

  if (saveImg.exec() == QDialog::Accepted)
    {
      QString flnm = saveImg.fileName();
      int imgMode = saveImg.imageMode();


      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      m_viewer->setImageMode(imgMode);
      m_viewer->setImageFileName(flnm);
      m_viewer->setSaveSnapshots(true);
      
      m_viewer->setImageSize(imgSize.width(), imgSize.height());

      //m_viewer->dummydraw();

      connect(this, SIGNAL(playKeyFrames(int,int,int)),
	      m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
      emit playKeyFrames(saveImg.startFrame(),
			 saveImg.endFrame(),
			 saveImg.stepFrame());
      qApp->processEvents();
      disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		 m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
    }
}

void
VrMain::on_actionSave_Movie_triggered()
{
  QSize imgSize = StaticFunctions::getImageSize(m_viewer->size().width(),
						m_viewer->size().height());

  m_viewer->setImageSize(imgSize.width(), imgSize.height());

#if defined(Q_OS_WIN32)
  if (imgSize.width()%16 > 0 ||
      imgSize.height()%16 > 0)
    {
      QMessageBox::information(0, "",
		       QString("For wmv movies, the image size must be multiple of 16. Current size is %1 x %2"). \
		       arg(imgSize.width()).
			       arg(imgSize.height()));
      return;
    }
#endif

  SaveMovieDialog saveImg(0,
			  Global::previousDirectory(),
			  m_keyFrameEditor->startFrame(),
			  m_keyFrameEditor->endFrame(),
			  1);

  saveImg.move(QCursor::pos());

  if (saveImg.exec() == QDialog::Accepted)
    {
      QString flnm = saveImg.fileName();
      //bool movieMode = saveImg.movieMode();
      QFileInfo f(flnm);
      Global::setPreviousDirectory(f.absolutePath());

      if (m_viewer->startMovie(flnm, 25, 100, true) == false)
	return;

      m_viewer->setSaveSnapshots(false);
      m_viewer->setSaveMovie(true);

      connect(this, SIGNAL(playKeyFrames(int,int,int)),
	      m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
      emit playKeyFrames(saveImg.startFrame(),
			 saveImg.endFrame(),
			 saveImg.stepFrame());
      qApp->processEvents();
      disconnect(this, SIGNAL(playKeyFrames(int,int,int)),
		 m_keyFrameEditor, SLOT(playKeyFrames(int,int,int)));
    }
}

void
VrMain::on_actionLoad_Keyframes_triggered()
{
  QString flnm;
  flnm = QFileDialog::getOpenFileName(0,
				      "Load Keyframes",
				      Global::previousDirectory(),
				      "Srishti Files (*.json)",
				      0,
				      QFileDialog::DontUseNativeDialog);


  if (flnm.isEmpty())
    return;

  QFile loadFile(flnm);
  loadFile.open(QIODevice::ReadOnly);

  QByteArray data = loadFile.readAll();

  QJsonDocument jsonDoc(QJsonDocument::fromJson(data));

  QJsonArray jsonArray = jsonDoc.array();

  m_keyFrame->load(jsonArray);

  m_keyFrameEditor->playKeyFramesForImages();
}

void
VrMain::on_actionSave_Keyframes_triggered()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Save Keyframes",
				      Global::previousDirectory(),
				      "Srishti Keyframes (*.json)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  if (flnm.isEmpty())
    return;

  if (!StaticFunctions::checkExtension(flnm, ".json"))
    flnm += ".json";

  QJsonArray keyframes = m_keyFrame->save();

  QJsonDocument saveDoc(keyframes);
  
  QFile saveFile(flnm);
  saveFile.open(QIODevice::WriteOnly);
  saveFile.write(saveDoc.toJson()); 
}

void
VrMain::reloadData()
{
  QList<PointCloud*> pcl = m_viewer->pointCloudList();
  for(int i=0; i<pcl.count(); i++)
    {
      pcl[i]->reload();
    }

  m_hiddenGL->clearPrevNodes();
  m_viewer->clearLoadNodeList();
  m_viewer->update();
}
