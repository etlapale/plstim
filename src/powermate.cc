#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>

#include <QtGui>

#include "powermate.h"


static bool powermate_event_type_registered = false;
QEvent::Type PowerMateEvent::EventType = (QEvent::Type) 0x3473;

static const char* powermate_name = "Griffin PowerMate";


static void
register_powermate_event_type ()
{
  if ( ! powermate_event_type_registered) {
    PowerMateEvent::EventType = (QEvent::Type) QEvent::registerEventType (0x3473);
    powermate_event_type_registered = true;
  }
}

PowerMateEvent::PowerMateEvent (int s)
  : QEvent (PowerMateEvent::EventType), step (s)
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

    // Process each event
    int num_events = nb / sizeof (struct input_event);
    for (int i = 0; i < num_events; i++) {
      switch (ibuf[i].type) {
      case EV_SYN:
      case EV_MSC:
	break;
      case EV_REL:
	if (ibuf[i].code != REL_DIAL) {
	  qDebug () << "invalid rotation code";
	  break;
	}
	{
	  // Create and send a PowerMate event
	  PowerMateEvent event ((int) ibuf[i].value);
	  // Get the focused object
#if 1
	  auto obj = QGuiApplication::focusObject ();
#else
	  auto obj = QApplication::focusWidget ();
#endif
	  // Emit the event
	  if (obj)
	    QCoreApplication::sendEvent (obj, &event);
	}
	break;
      default:
	qDebug () << "unknown event type:" << hex << ibuf[i].type << dec;
      }
    }
  }
}
