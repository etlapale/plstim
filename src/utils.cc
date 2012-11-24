#include "utils.h"

#include <QCoreApplication>
#include <QTextCodec>

namespace plstim
{

  static bool initialised = false;

  bool initialise ()
  {
    if (initialised)
      return true;

    auto utf8_codec = QTextCodec::codecForName ("UTF-8");
    QTextCodec::setCodecForCStrings (utf8_codec);
    QTextCodec::setCodecForTr (utf8_codec);

    // Define the Qt application
    QCoreApplication::setOrganizationName ("Tlapale");
    QCoreApplication::setOrganizationDomain ("tlapale.com");
    QCoreApplication::setApplicationName ("plstim");

    initialised = true;
    return true;
  }
}
