#include "volumefactory.h"

VolumeFactory::VolumeFactory()
{
  m_volumes.clear();
  m_volumeStack.clear();
}

VolumeFactory::~VolumeFactory()
{
  for (int i=0; i<m_volumes.count(); i++)
    {
      m_volumes[i]->reset();
      delete m_volumes[i];
    }
  m_volumes.clear();

  m_volumeStack.clear();
}

Volume*
VolumeFactory::newVolume()
{
  Volume *vol = new Volume();
  m_volumes << vol;

  m_volumeStack.push(vol);

  return vol;
}

Volume*
VolumeFactory::topVolume()
{
  return m_volumeStack.top();
}

void
VolumeFactory::pushVolume(Volume* v)
{
  m_volumeStack.push(v);
}

Volume*
VolumeFactory::popVolume()
{
  return m_volumeStack.pop();
}

Volume*
VolumeFactory::checkVolume(QString dirname)
{
  for(int i=0; i<m_volumes.count(); i++)
    {
      if (m_volumes[i]->filenames()[0] == dirname)
	return m_volumes[i];
    }

  return NULL;
}
