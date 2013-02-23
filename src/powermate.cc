#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>

#include <QtGui>

#include "powermate.h"


static bool powermate_event_type_registered = false;
QEvent::Type PowerMateEvent::Rotation = (QEvent::Type) 0x3473;
QEvent::Type PowerMateEvent::ButtonPress = (QEvent::Type) 0x3474;
QEvent::Type PowerMateEvent::ButtonRelease = (QEvent::Type) 0x3475;
QEvent::Type PowerMateEvent::LED = (QEvent::Type) 0x3476;

static const char* powermate_name = "Griffin PowerMate";


static void
register_powermate_event_type ()
{
  if ( ! powermate_event_type_registered) {
    PowerMateEvent::Rotation = (QEvent::Type) QEvent::registerEventType (PowerMateEvent::Rotation);
    PowerMateEvent::ButtonPress = (QEvent::Type) QEvent::registerEventType (PowerMateEvent::ButtonPress);
    PowerMateEvent::ButtonRelease = (QEvent::Type) QEvent::registerEventType (PowerMateEvent::ButtonRelease);
    PowerMateEvent::LED = (QEvent::Type) QEvent::registerEventType (PowerMateEvent::LED);
    powermate_event_type_registered = true;
  }
}

PowerMateEvent::PowerMateEvent (QEvent::Type eventType)
  : QEvent (eventType)
{
}

void
PowerMateWatcher::watch ()
{
  // Make sure PowerMateEvent is registered
  register_powermate_event_type ();

  // Open the PowerMate device
  int fd = open ("/dev/powermate", O_RDWR);
  if (fd < 0) {
    qDebug () << "unable to open powermate:" << strerror (errno);
    return;
  }

  // Make sure it’s a PowerMate
  char buf[255];
  if (ioctl (fd, EVIOCGNAME (sizeof (buf)), buf) < 0) {
    qDebug () << "unable to get device name:" << strerror (errno);
    return;
  }
  if (strncasecmp (buf, powermate_name, strlen (powermate_name)) != 0) {
    qDebug () << "invalid name for device:" << buf;
    close (fd);
    return;
  }

  // Disable LED lighting
  struct input_event ev;
  memset (&ev, 0, sizeof (struct input_event));
  ev.type = EV_MSC;
  ev.code = MSC_PULSELED;
  if (write (fd, &ev, sizeof (struct input_event)) != sizeof (struct input_event))
    qDebug () << "could disable LED on PowerMate";

  // Wait for PowerMate events
  const int IBUF_SZ = 8;
  struct input_event ibuf[IBUF_SZ];
  for (;;) {
    int nb = read (fd, ibuf, sizeof (struct input_event) * IBUF_SZ);
    if (nb < 0) {
      qDebug () << "reading PowerMate events failed:" << strerror (errno);
      continue;
    }
    
    // Get the focused object
    auto obj = QGuiApplication::focusObject ();
    if (! obj)
	continue;

    // Process each event
    int num_events = nb / sizeof (struct input_event);
    for (int i = 0; i < num_events; i++) {

	switch (ibuf[i].type) {
	case EV_SYN:
	    break;
	case EV_MSC:
	    if (ibuf[i].code == MSC_PULSELED) {
		PowerMateEvent evt (PowerMateEvent::LED);
		evt.luminance = ibuf[i].value & 0xff;
		QCoreApplication::sendEvent (obj, &evt);
	    }
	    break;
	case EV_REL:
	    if (ibuf[i].code == REL_DIAL) {
		// Create and send a PowerMate event
		PowerMateEvent evt (PowerMateEvent::Rotation);
		evt.step = (int) ibuf[i].value;
		// Emit the event
		QCoreApplication::sendEvent (obj, &evt);
	    }
	    break;
	case EV_KEY:
	    if (ibuf[i].code == BTN_0) {
		if (ibuf[i].value) {
		    PowerMateEvent evt (PowerMateEvent::ButtonPress);
		    QCoreApplication::sendEvent (obj, &evt);
		}
		else {
		    PowerMateEvent evt (PowerMateEvent::ButtonRelease);
		    QCoreApplication::sendEvent (obj, &evt);
		}
	    }
	    break;
	}
    }
  }
}
