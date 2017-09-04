#include "volumeloaderthread.h"
#include <QMessageBox>

VolumeLoaderThread::VolumeLoaderThread() : QObject()
{
  m_loading = false;
  m_stopLoading = false;
}

void
VolumeLoaderThread::setPointClouds(QList<PointCloud*> pc)
{
  if(m_loading)
    m_stopLoading = true;
  
  m_pointClouds = pc;
}

void
VolumeLoaderThread::startLoading()
{
  m_loading = true;
  for(int d=0; d<m_pointClouds.count(); d++)
    {
      if (m_stopLoading)
	break;

      m_pointClouds[d]->loadAll();
    }

  m_loading = false;
  m_stopLoading = false;
}

void
VolumeLoaderThread::stopLoading()
{
  if(m_loading)
    m_stopLoading = true;
}
