#include "utils.h"

#include <QCoreApplication>

namespace plstim
{

  static bool initialised = false;

  bool initialise ()
  {
    if (initialised)
      return true;

    // Define the Qt application
    QCoreApplication::setOrganizationName ("Tlapale");
    QCoreApplication::setOrganizationDomain ("tlapale.com");
    QCoreApplication::setApplicationName ("plstim");

    initialised = true;
    return true;
  }
}
