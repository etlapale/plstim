#include <QCoreApplication>
#include <QTextCodec>

#include "qtypes.h"
#include "setup.h"

namespace plstim
{

  static bool initialised = false;

  bool initialise ()
  {
    if (initialised)
      return true;

    // Sources encoding
    //auto utf8_codec = QTextCodec::codecForName ("UTF-8");
    //QTextCodec::setCodecForCStrings (utf8_codec);
    //QTextCodec::setCodecForTr (utf8_codec);

    // Define the Qt application
    QCoreApplication::setOrganizationName ("Tlapale");
    QCoreApplication::setOrganizationDomain ("tlapale.com");
    QCoreApplication::setApplicationName ("plstim");

    // Register QML types for GUI
    qmlRegisterType<plstim::Error> ("PlStim", 1, 0, "Error");

    // Register QML types for experiments
    qmlRegisterType<plstim::Setup> ("PlStim", 1, 0, "Setup");
    qmlRegisterType<plstim::Subject> ("PlStim", 1, 0, "Subject");
    qmlRegisterType<plstim::Vector> ("PlStim", 1, 0, "Vector");
    qmlRegisterType<plstim::Pen> ("PlStim", 1, 0, "Pen");
    qmlRegisterType<plstim::PainterPath> ("PlStim", 1, 0, "PainterPath");
    qmlRegisterUncreatableType<plstim::Painter> ("PlStim", 1, 0, "Painter", "Painter objects cannot be created from QML");
    qmlRegisterType<plstim::Page> ("PlStim", 1, 0, "Page");
    qmlRegisterType<plstim::Experiment> ("PlStim", 1, 0, "Experiment");

    initialised = true;
    return true;
  }
}
