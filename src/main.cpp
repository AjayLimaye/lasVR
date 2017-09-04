#include <QApplication>
#include "vrmain.h"
#include <QSurfaceFormat>
#include <QSettings>

int main(int argv, char **args)
{
  QCoreApplication::setOrganizationName("NCI");
  QCoreApplication::setOrganizationDomain("NCI");
  QCoreApplication::setApplicationName("VR-PointCloudRenderer");

  QSurfaceFormat glFormat;
  glFormat.setVersion(4, 1);
  glFormat.setProfile(QSurfaceFormat::CoreProfile);
  //glFormat.setSamples(0);
  glFormat.setSwapInterval(0);
  glFormat.setDepthBufferSize(16);
  //glFormat.setOption(QSurfaceFormat::DebugContext);
  
  QSurfaceFormat::setDefaultFormat(glFormat);

  QApplication app(argv, args);

  VrMain mainWindow;
  mainWindow.show();

  return app.exec();
}
