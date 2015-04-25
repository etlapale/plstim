// src/powermate.cc – Handle events from PowerMate devices
//
// Copyright © 2012–2015 University of California, Irvine
// Licensed under the Simplified BSD License.

#ifdef HAVE_WIN32
#include <windows.h>
#include <setupapi.h>
#include <ddk/hidsdi.h>
// PowerMate identification on Windows
#define GRIFFIN_TECHNOLOGY_VENDOR_ID 0x77d
#define POWERMATE_PRODUCT_ID 0x410
#else // HAVE_WIN32
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <unistd.h>
#endif // HAVE_WIN32

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

#ifdef HAVE_WIN32
    GUID hidGuid;
    HidD_GetHidGuid (&hidGuid);
    qDebug () << "HID GUID is:" << hidGuid;


    HDEVINFO hinfo = SetupDiGetClassDevs (&hidGuid, NULL, NULL,
            DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

    HANDLE fd;
    bool powerMateFound = false;

    int idx = 0;
    SP_DEVICE_INTERFACE_DATA devInfo;
    PSP_DEVICE_INTERFACE_DETAIL_DATA details = NULL;
    devInfo.cbSize = sizeof (devInfo);
    while (SetupDiEnumDeviceInterfaces (hinfo, 0, &hidGuid, idx, &devInfo)) {
        // Get detail data size
        DWORD dataSize;
        SetupDiGetDeviceInterfaceDetail (hinfo, &devInfo, NULL, 0,
                                         &dataSize, NULL);
        details = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc (dataSize);
        details->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);

        // Get device details
        bool ans = SetupDiGetDeviceInterfaceDetail (hinfo, &devInfo,
                details, dataSize, NULL, NULL);
        if (! ans)
            qDebug () << "Could not get device details";

        // Open the device
        fd = CreateFile (details->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                NULL);
        if (fd == INVALID_HANDLE_VALUE)
            qDebug () << "Could not open the device";

        // Get device vendor and product IDs
        HIDD_ATTRIBUTES hattribs;
        hattribs.Size = sizeof (hattribs);
        HidD_GetAttributes (fd, &hattribs);
        if (hattribs.VendorID == GRIFFIN_TECHNOLOGY_VENDOR_ID
                && hattribs.ProductID == POWERMATE_PRODUCT_ID) {
            free (details);
            powerMateFound = true;
            break;
        }
        CloseHandle (fd);
        free (details);
        idx++;
    }

    SetupDiDestroyDeviceInfoList (hinfo);
    if (! powerMateFound) {
        qDebug () << "PowerMate device not found";
        return;
    }

#else
    // Open the PowerMate device
    int fd = open ("/dev/input/powermate", O_RDWR);
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
#endif

    // Disable LED lighting
#if HAVE_WIN32
    HANDLE hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);

    OVERLAPPED overlap;
    ZeroMemory (&overlap, sizeof (overlap));
    overlap.hEvent = hEvent;

    UCHAR reportBuffer[8];
    reportBuffer[0] = 0;
    reportBuffer[1] = 0;        // Brightness

    DWORD nbytes = 0;
    BOOL res = WriteFile (fd, reportBuffer, 2, &nbytes, &overlap);

    WaitForSingleObject (hEvent, 1000);
    CloseHandle (hEvent);
#else
    struct input_event ev;
    memset (&ev, 0, sizeof (struct input_event));
    ev.type = EV_MSC;
    ev.code = MSC_PULSELED;
    if (write (fd, &ev, sizeof (struct input_event)) != sizeof (struct input_event))
	qDebug () << "could disable LED on PowerMate";
#endif

    // Wait for PowerMate events
#if HAVE_WIN32
    hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    ZeroMemory (&overlap, sizeof (overlap));
    overlap.hEvent = hEvent;
#else
    const int IBUF_SZ = 8;
    struct input_event ibuf[IBUF_SZ];
#endif
    for (;;) {

#if HAVE_WIN32
        res = ReadFile (fd, reportBuffer, 8, &nbytes, &overlap);
        if (WaitForSingleObject (hEvent, INFINITE) == WAIT_OBJECT_0) {
	    // Get the focused object
	    auto obj = QGuiApplication::focusObject ();
	    if (! obj) continue;
            // Button pushed
            if (reportBuffer[1] == 1) {
                auto evt = new PowerMateEvent (PowerMateEvent::ButtonPress);
                QCoreApplication::postEvent (obj, evt);
            }
            // Button released (TODO: or pushed and turned!)
            else if (reportBuffer[1] == 0 && reportBuffer[2] == 0) {
                auto evt = new PowerMateEvent (PowerMateEvent::ButtonRelease);
                QCoreApplication::postEvent (obj, evt);
            }
            // Rotation
            if (reportBuffer[2] != 0) {
                auto evt = new PowerMateEvent (PowerMateEvent::Rotation);
                evt->step = (int) (signed char) reportBuffer[2];
                QCoreApplication::postEvent (obj, evt);
            }
        }
#else
        int nb = read (fd, ibuf, sizeof (struct input_event) * IBUF_SZ);
        if (nb < 0) {
            qDebug () << "reading PowerMate events failed:" << strerror (errno);
            continue;
        }
	// Get the focused object
	auto obj = QGuiApplication::focusObject ();
	if (! obj) continue;

        // Process each event
        int num_events = nb / sizeof (struct input_event);
        for (int i = 0; i < num_events; i++) {
            switch (ibuf[i].type) {
            case EV_SYN:
            break;
            case EV_MSC:
            if (ibuf[i].code == MSC_PULSELED) {
                auto evt = new PowerMateEvent (PowerMateEvent::LED);
                evt->luminance = ibuf[i].value & 0xff;
                QCoreApplication::postEvent (obj, evt);
            }
            break;
            case EV_REL:
            if (ibuf[i].code == REL_DIAL) {
                // Create and send a PowerMate event
                auto evt = new PowerMateEvent (PowerMateEvent::Rotation);
                evt->step = (int) ibuf[i].value;
                // Emit the event
                QCoreApplication::postEvent (obj, evt);
            }
            break;
            case EV_KEY:
            if (ibuf[i].code == BTN_0) {
                if (ibuf[i].value) {
                    auto evt = new PowerMateEvent (PowerMateEvent::ButtonPress);
                    QCoreApplication::postEvent (obj, evt);
                }
                else {
                    auto evt = new PowerMateEvent (PowerMateEvent::ButtonRelease);
                    QCoreApplication::postEvent (obj, evt);
                }
            }
            break;
            }
        }
#endif
    }
}
