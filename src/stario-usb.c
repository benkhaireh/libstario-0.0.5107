/*
    libstario.so
    ------------
    Library providing USB, serial, and parallel communications support for
    Star Micronics devices.

    Copyright (C) 2004 Star Micronics Co., Ltd.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <memory.h>
#include <usb.h>
#include <sys/time.h>

#ifdef RPMBUILD

#include <dlfcn.h>

static void * libusb = NULL;

typedef usb_dev_handle *    (*usb_open_fndef)               (struct usb_device *dev);
typedef int                 (*usb_close_fndef)              (usb_dev_handle *dev);
typedef int                 (*usb_bulk_write_fndef)         (usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
typedef int                 (*usb_bulk_read_fndef)          (usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
typedef int                 (*usb_control_msg_fndef)        (usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);
//typedef int                 (*usb_set_configuration_fndef)  (usb_dev_handle *dev, int configuration);
typedef int                 (*usb_claim_interface_fndef)    (usb_dev_handle *dev, int interface);
typedef int                 (*usb_release_interface_fndef)  (usb_dev_handle *dev, int interface);
//typedef int                 (*usb_set_altinterface_fndef)   (usb_dev_handle *dev, int alternate);
//typedef int                 (*usb_resetep_fndef)            (usb_dev_handle *dev, unsigned int ep);
typedef int                 (*usb_clear_halt_fndef)         (usb_dev_handle *dev, unsigned int ep);
//typedef int                 (*usb_reset_fndef)              (usb_dev_handle *dev);
//typedef char *              (*usb_strerror_fndef)           (void);
typedef void                (*usb_init_fndef)               (void);
//typedef void                (*usb_set_debug_fndef)          (int level);
typedef int                 (*usb_find_busses_fndef)        (void);
typedef int                 (*usb_find_devices_fndef)       (void);
//typedef struct usb_device * (*usb_device_fndef)             (usb_dev_handle *dev);
typedef struct usb_bus *    (*usb_get_busses_fndef)         (void);

static usb_open_fndef               usb_open_fn;
static usb_close_fndef              usb_close_fn;
static usb_bulk_write_fndef         usb_bulk_write_fn;
static usb_bulk_read_fndef          usb_bulk_read_fn;
static usb_control_msg_fndef        usb_control_msg_fn;
//static usb_set_configuration_fndef  usb_set_configuration_fn;
static usb_claim_interface_fndef    usb_claim_interface_fn;
static usb_release_interface_fndef  usb_release_interface_fn;
//static usb_set_altinterface_fndef   usb_set_altinterface_fn;
//static usb_resetep_fndef            usb_resetep_fn;
static usb_clear_halt_fndef         usb_clear_halt_fn;
//static usb_reset_fndef              usb_reset_fn;
//static usb_strerror_fndef           usb_strerror_fn;
static usb_init_fndef               usb_init_fn;
//static usb_set_debug_fndef          usb_set_debug_fn;
static usb_find_busses_fndef        usb_find_busses_fn;
static usb_find_devices_fndef       usb_find_devices_fn;
//static usb_device_fndef             usb_device_fn;
static usb_get_busses_fndef         usb_get_busses_fn;

#define USB_OPEN                (*usb_open_fn)
#define USB_CLOSE               (*usb_close_fn)
#define USB_BULK_WRITE          (*usb_bulk_write_fn)
#define USB_BULK_READ           (*usb_bulk_read_fn)
#define USB_CONTROL_MSG         (*usb_control_msg_fn)
//#define USB_SET_CONFIGURATION   (*usb_set_configuration_fn)
#define USB_CLAIM_INTERFACE     (*usb_claim_interface_fn)
#define USB_RELEASE_INTERFACE   (*usb_release_interface_fn)
//#define USB_SET_ALTINTERFACE    (*usb_set_altinterface_fn)
//#define USB_RESETEP             (*usb_resetep_fn)
#define USB_CLEAR_HALT          (*usb_clear_halt_fn)
//#define USB_RESET               (*usb_reset_fn)
//#define USB_STRERROR            (*usb_strerror_fn)
#define USB_INIT                (*usb_init_fn)
//#define USB_SET_DEBUG           (*usb_set_debug_fn)
#define USB_FIND_BUSSES         (*usb_find_busses_fn)
#define USB_FIND_DEVICES        (*usb_find_devices_fn)
//#define USB_DEVICE              (*usb_device_fn)
#define USB_GET_BUSSES          (*usb_get_busses_fn)

#else

#define USB_OPEN                usb_open
#define USB_CLOSE               usb_close
#define USB_BULK_WRITE          usb_bulk_write
#define USB_BULK_READ           usb_bulk_read
#define USB_CONTROL_MSG         usb_control_msg
//#define USB_SET_CONFIGURATION   usb_set_configuration
#define USB_CLAIM_INTERFACE     usb_claim_interface
#define USB_RELEASE_INTERFACE   usb_release_interface
//#define USB_SET_ALTINTERFACE    usb_set_altinterface
//#define USB_RESETEP             usb_resetep
#define USB_CLEAR_HALT          usb_clear_halt
//#define USB_RESET               usb_reset
//#define USB_STRERROR            usb_strerror
#define USB_INIT                usb_init
//#define USB_SET_DEBUG           usb_set_debug
#define USB_FIND_BUSSES         usb_find_busses
#define USB_FIND_DEVICES        usb_find_devices
//#define USB_DEVICE              usb_device
#define USB_GET_BUSSES          usb_get_busses

#endif

// project headers
#include "stario-error.h"
#include "stario-usb.h"

// forward declarations
static long usbMatchPortName        (char const * portName);
static long usbOpenPort             (char const * portName, char const * portSettings);
static long usbWritePort            (char const * portName, char const * writeBuffer, long length);
static long usbReadPort             (char const * portName, char * readBuffer, long length);
static long usbGetStarPrinterStatus (char const * portName, StarPrinterStatus * status);
static long usbBeginCheckedBlock    (char const * portName);
static long usbEndCheckedBlock      (char const * portName, StarPrinterStatus * status);
static long usbHdwrResetDevice      (char const * portName);
static long usbDoVisualCardCmd      (char const * portName, VisualCardCmd * request, long timeoutMillis);
static long usbClosePort            (char const * portName);
static void usbReleaseImpl          ();

// Star's USB vendor and product ID numbers
#define STAR_VENDOR_ID              0x0519
#define VENDORCLASS_PRODUCT_ID      0x0002

// fixed timeout values
// timeout defined as maximum time without any device response
// i.e. 10000 milliseconds with device not accepting any output data
// i.e. 200 milliseconds without device transmitting data for reading
#define USB_CONTROL_MSG_TIMEOUT     500
#define USB_BULK_WRITE_TIMEOUT      10000
#define USB_BULK_READ_TIMEOUT       200

typedef struct
{
    unsigned char set;                  // if 0, not set
    char portName[100];                 // string port name - i.e. "usb:TSP700" or "usb:TCP300"

    struct usb_bus *bus;                // pointer to USB bus
    struct usb_device *dev;             // pointer to the device
    struct usb_dev_handle *udev;        // handle to use the device

    int config;                         // configuration index
    int interface;                      // interface index
    int altsetting;                     // altsetting index

    int inep;                           // bulk-in endpoint index
    int outep;                          // bulk-out endpoint index

    StarPrinterStatus statusCache;      // status cache populated during beginCheckedBlock fn
} USBPort;

static USBPort usbPorts[MAX_NUM_PORTS]; // array storing USBPort structures

PortImpl getUsbPortImpl(void)
{
    memset(usbPorts, 0x00, sizeof(usbPorts));

    PortImpl impl;

    memset(&impl, 0x00, sizeof(PortImpl));

#ifdef RPMBUILD

    libusb = dlopen("libusb.so", RTLD_NOW | RTLD_GLOBAL);
    if (! libusb)
    {
        return impl;
    }

    int allSymbolsFound = 0;

    do
    {
        if (! (usb_open_fn              = dlsym(libusb, "usb_open"              ))) break;
        if (! (usb_close_fn             = dlsym(libusb, "usb_close"             ))) break;
        if (! (usb_bulk_write_fn        = dlsym(libusb, "usb_bulk_write"        ))) break;
        if (! (usb_bulk_read_fn         = dlsym(libusb, "usb_bulk_read"         ))) break;
        if (! (usb_control_msg_fn       = dlsym(libusb, "usb_control_msg"       ))) break;
        //if (! (usb_set_configuration_fn = dlsym(libusb, "usb_set_configuration" ))) break;
        if (! (usb_claim_interface_fn   = dlsym(libusb, "usb_claim_interface"   ))) break;
        if (! (usb_release_interface_fn = dlsym(libusb, "usb_release_interface" ))) break;
        //if (! (usb_set_altinterface_fn  = dlsym(libusb, "usb_set_altinterface"  ))) break;
        //if (! (usb_resetep_fn           = dlsym(libusb, "usb_resetep"           ))) break;
        if (! (usb_clear_halt_fn        = dlsym(libusb, "usb_clear_halt"        ))) break;
        //if (! (usb_reset_fn             = dlsym(libusb, "usb_reset"             ))) break;
        //if (! (usb_strerror_fn          = dlsym(libusb, "usb_strerror"          ))) break;
        if (! (usb_init_fn              = dlsym(libusb, "usb_init"              ))) break;
        //if (! (usb_set_debug_fn         = dlsym(libusb, "usb_set_debug"         ))) break;
        if (! (usb_find_busses_fn       = dlsym(libusb, "usb_find_busses"       ))) break;
        if (! (usb_find_devices_fn      = dlsym(libusb, "usb_find_devices"      ))) break;
        //if (! (usb_device_fn            = dlsym(libusb, "usb_device"            ))) break;
        if (! (usb_get_busses_fn        = dlsym(libusb, "usb_get_busses"        ))) break;

        allSymbolsFound = 1;
    } while (0);

    if (! allSymbolsFound)
    {
        dlclose(libusb);
        libusb = NULL;

        return impl;
    }
#endif

    impl.matchPortName          = usbMatchPortName;
    impl.openPort               = usbOpenPort;
    impl.writePort              = usbWritePort;
    impl.readPort               = usbReadPort;
    impl.getStarPrinterStatus   = usbGetStarPrinterStatus;
    impl.beginCheckedBlock      = usbBeginCheckedBlock;
    impl.endCheckedBlock        = usbEndCheckedBlock;
    impl.hdwrResetDevice        = usbHdwrResetDevice;
    impl.doVisualCardCmd        = usbDoVisualCardCmd;
    impl.closePort              = usbClosePort;
    impl.releaseImpl            = usbReleaseImpl;

    return impl;
}

static USBPort * usbFindPort(char const * portName)
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (usbPorts[i].set != 0)
            if (strcmp(usbPorts[i].portName, portName) == 0)
                break;
    }

    if (i == MAX_NUM_PORTS)
    {
        return NULL;
    }

    return &usbPorts[i];
}

static int find_ep(struct usb_device *dev, int config, int interface, int altsetting, int direction, int type)
{
    struct usb_interface_descriptor *intf;
    int i;

    if (dev->config == NULL)
    {
        return -1;
    }

    intf = &dev->config[config].interface[interface].altsetting[altsetting];

    for (i = 0; i < intf->bNumEndpoints; i++)
    {
        if (((intf->endpoint[i].bEndpointAddress & USB_ENDPOINT_DIR_MASK) == direction) && ((intf->endpoint[i].bmAttributes & USB_ENDPOINT_TYPE_MASK) == type))
            return intf->endpoint[i].bEndpointAddress;
    }

    return -1;
}

static int find_first_altsetting(struct usb_device *dev, int * config, int * interface, int * altsetting)
{
    int i;
    int i1;
    int i2;

    if (dev->config == NULL)
    {
        return -1;
    }

    for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
    {
        for (i1 = 0; i1 < dev->config[i].bNumInterfaces; i1++)
        {
            for (i2 = 0; i2 < dev->config[i].interface[i1].num_altsetting; i2++)
            {
                if (dev->config[i].interface[i1].altsetting[i2].bNumEndpoints)
                {
                    *config = i;
                    *interface = i1;
                    *altsetting = i2;

                    return 0;
                }
            }
        }
    }

    return -1;
}

static long usbGetPortSignals(char const * portName)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    unsigned char portSignals = 0;

    if (USB_CONTROL_MSG(usbPort->udev, (char) 0xc0, (char) 1, (short) 0, (short) 0, &portSignals, (short) 1, USB_CONTROL_MSG_TIMEOUT) < 0)
    {
        if (errno == ENODEV)
        {
            usbClosePort(portName);

            return STARIO_ERROR_NOT_OPEN;
        }

        return STARIO_ERROR_IO_FAIL;
    }

    return portSignals;
}

static long usbMatchPortName (char const * portName)
{
    if (strncmp(portName, "usb:", 4) == 0)
    {
        return STARIO_ERROR_SUCCESS;
    }

    return STARIO_ERROR_NOT_AVAILABLE;
}

static long usbOpenPort (char const * portName, char const * portSettings)
{
    USBPort * oldUsbPort = usbFindPort(portName);
    if (oldUsbPort != NULL)
    {
        return STARIO_ERROR_SUCCESS;
    }

    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (usbPorts[i].set == 0)
            break;
    }
    if ( i == MAX_NUM_PORTS)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    USBPort usbPort;

    memset(&usbPort, 0x00, sizeof(USBPort));

    usbPort.set = 1;

    strcpy(usbPort.portName, portName);

    char * model = &usbPort.portName[4];
    char * serial = strstr(usbPort.portName, ";sn:");

    int modelLen = 0;
    int serialLen = 0;

    if (serial != NULL)
    {
        modelLen = serial - model;
        model[modelLen] = 0;    // temporarily NULL terminate the model string, restore ';' later on

        serial += 4;

        serialLen = strlen(serial);
    }
    else
    {
        modelLen = strlen(model);
    }

    short productID = 0;

    productID = VENDORCLASS_PRODUCT_ID;

    struct usb_bus *bus;
    struct usb_device *dev;
    struct usb_dev_handle *udev;

    USB_INIT();

    USB_FIND_BUSSES();
    USB_FIND_DEVICES();

    unsigned char found = 0;

    for (bus = USB_GET_BUSSES(); bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {
            if (dev->descriptor.idVendor != STAR_VENDOR_ID)
            {
                continue;
            }

            if (dev->descriptor.idProduct != productID)
            {
                continue;
            }

            int config;
            int interface;
            int altsetting;

            if (find_first_altsetting(dev, &config, &interface, &altsetting) == -1)
            {
                continue;
            }

            usbPort.config = dev->config[config].bConfigurationValue;
            usbPort.interface = dev->config[config].interface[interface].altsetting[altsetting].bInterfaceNumber;
            usbPort.altsetting = dev->config[config].interface[interface].altsetting[altsetting].bAlternateSetting;

            usbPort.inep = find_ep(dev, config, interface, altsetting, USB_ENDPOINT_IN, USB_ENDPOINT_TYPE_BULK);
            usbPort.outep = find_ep(dev, config, interface, altsetting, USB_ENDPOINT_OUT, USB_ENDPOINT_TYPE_BULK);

            if ((usbPort.inep == -1) || (usbPort.outep == -1))
            {
                continue;
            }

            udev = USB_OPEN(dev);
            if (udev == NULL)
            {
                continue;
            }

            USB_CLAIM_INTERFACE(udev, usbPort.interface);

            // the following two calls do not need to be made, and libusb docs state that not calling them is better
            //usb_set_configuration(udev, usbPort.config);
            //usb_set_altinterface(udev, usbPort.altsetting);

            usbPort.bus = bus;
            usbPort.dev = dev;
            usbPort.udev = udev;

            char deviceID[257];
            memset(deviceID, 0x00, sizeof(deviceID));

            int deviceIDLen = 0;
            if ((deviceIDLen = USB_CONTROL_MSG(usbPort.udev, (char) 0xc0, (char) 0, (short) 0, (short) 0, deviceID, (short) 256, USB_CONTROL_MSG_TIMEOUT)) < 0)
            {
                USB_RELEASE_INTERFACE(usbPort.udev, usbPort.interface);
                USB_CLOSE(usbPort.udev);

                continue;
            }

            if (strstr(&deviceID[2], model) == 0)
            {
                USB_RELEASE_INTERFACE(usbPort.udev, usbPort.interface);
                USB_CLOSE(usbPort.udev);

                continue;
            }

            if (serial != NULL)
            {
                unsigned char snMatch = 0;

                do
                {
                    if (! usbPort.dev->descriptor.iSerialNumber)
                    {
                        break;
                    }

                    char rawSN[256];

                    if (USB_CONTROL_MSG(usbPort.udev,
                                        USB_ENDPOINT_IN,
                                        USB_REQ_GET_DESCRIPTOR,
                                        (USB_DT_STRING << 8) + usbPort.dev->descriptor.iSerialNumber,
                                        0,
                                        rawSN,
                                        sizeof(rawSN),
                                        USB_CONTROL_MSG_TIMEOUT) < (2 + 8 * 2))
                    {
                        break;
                    }

                    if (rawSN[0] != (2 + 8 * 2))
                    {
                        break;
                    }

                    if (rawSN[1] != USB_DT_STRING)
                    {
                        break;
                    }

                    char sn[256];

                    int snIdx = 0;
                    int rawSNIdx = 2;
                    for (; rawSNIdx < rawSN[0]; rawSNIdx += 2)
                    {
                        if (snIdx >= (256 - 1))
                            break;

                        if (rawSN[rawSNIdx + 1])
                            sn[snIdx++] = '?';
                        else
                            sn[snIdx++] = rawSN[rawSNIdx];
                    }
                    sn[snIdx] = 0;

                    if (strcmp(sn, serial) != 0)
                    {
                        break;
                    }

                    model[modelLen]=';';    // restore ';' for future string comapares

                    snMatch = 1;
                } while (0);

                if (snMatch == 0)
                {
                    USB_RELEASE_INTERFACE(usbPort.udev, usbPort.interface);
                    USB_CLOSE(usbPort.udev);

                    continue;
                }
            }

            found = 1;
            break;
        }
        if (found == 1)
            break;
    }

    if (found == 0)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    memcpy(&usbPorts[i], &usbPort, sizeof(USBPort));

    return STARIO_ERROR_SUCCESS;
}

static long usbWritePort (char const * portName, char const * writeBuffer, long length)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long lengthSent = 0;

    while (lengthSent < length)
    {
        int writeAttemptLength = ((length - lengthSent)>4096)?4096:(length - lengthSent);

        int partialWriteLength = USB_BULK_WRITE(usbPort->udev, usbPort->outep, (char *) &writeBuffer[lengthSent], writeAttemptLength, USB_BULK_WRITE_TIMEOUT);

        if (partialWriteLength < 0)
        {
            if (errno == ENODEV)
            {
                usbClosePort(portName);

                return STARIO_ERROR_NOT_OPEN;
            }
            else
            {
                // i.e.  ETIMEDOUT
                partialWriteLength = 0;
            }
        }

        lengthSent += partialWriteLength;

        if (partialWriteLength != writeAttemptLength)
        {
            if (USB_CLEAR_HALT(usbPort->udev, usbPort->outep) < 0)
            {
                if (errno == ENODEV)
                {
                    usbClosePort(portName);

                    return STARIO_ERROR_NOT_OPEN;
                }
            }

            return lengthSent;
        }
    }

    return lengthSent;
}

static long usbReadPort (char const * portName, char * readBuffer, long length)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    short availableReadLength = 0;

    if (USB_CONTROL_MSG(usbPort->udev, (char) 0xc0, (char) 3, (short) length, (short) 0, (char *) &availableReadLength, (short) 2, USB_CONTROL_MSG_TIMEOUT) < 0)
    {
        if (errno == ENODEV)
        {
            usbClosePort(portName);

            return STARIO_ERROR_NOT_OPEN;
        }

        return STARIO_ERROR_IO_FAIL;
    }

    if (availableReadLength == 0)
    {
        return 0;
    }

    if (length > ((int) availableReadLength))
    {
        length = (int) availableReadLength;
    }

    int lengthReceived = USB_BULK_READ(usbPort->udev, usbPort->inep, readBuffer, length, USB_BULK_READ_TIMEOUT);
    if (lengthReceived <= 0)
    {
        lengthReceived = 0;

        if (errno == ENODEV)
        {
            usbClosePort(portName);

            return STARIO_ERROR_NOT_OPEN;
        }

        return STARIO_ERROR_IO_FAIL;
    }

    return lengthReceived;
}

static long usbGetStarPrinterStatus (char const * portName, StarPrinterStatus * status)
{
    memset(status, 0x00, sizeof(StarPrinterStatus));

    long readResult = usbReadPort(portName, status->raw, sizeof(status->raw));

    if (readResult < STARIO_ERROR_SUCCESS)
    {
        return readResult;
    }

    if (readResult < 7)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    status->rawLength = (unsigned char) readResult;

    // printer status 1
    status->coverOpen           = ((status->raw[2] & 0x20)!=0)?1:0;
    status->offline             = ((status->raw[2] & 0x08)!=0)?1:0;
    status->compulsionSwitch    = ((status->raw[2] & 0x04)!=0)?1:0;

    // printer status 2
    status->overTemp            = ((status->raw[3] & 0x40)!=0)?1:0;
    status->unrecoverableError  = ((status->raw[3] & 0x20)!=0)?1:0;
    status->cutterError         = ((status->raw[3] & 0x08)!=0)?1:0;
    status->mechError           = ((status->raw[3] & 0x04)!=0)?1:0;

    // printer status 3
    status->pageModeCmdError        = ((status->raw[4] & 0x20)!=0)?1:0;
    status->paperSizeError          = ((status->raw[4] & 0x08)!=0)?1:0;
    status->presenterPaperJamError  = ((status->raw[4] & 0x04)!=0)?1:0;
    status->headUpError             = ((status->raw[4] & 0x02)!=0)?1:0;

    // printer status 4
    status->blackMarkDetectStatus   = ((status->raw[5] & 0x20)!=0)?1:0;
    status->paperEmpty              = ((status->raw[5] & 0x08)!=0)?1:0;
    status->paperNearEmptyInner     = ((status->raw[5] & 0x04)!=0)?1:0;
    status->paperNearEmptyOuter     = ((status->raw[5] & 0x02)!=0)?1:0;

    // printer status 5
    status->stackerFull             = ((status->raw[6] & 0x02)!=0)?1:0;

    // printer status 6
    status->etbAvailable            = (status->rawLength >= 9)?1:0;
    status->etbCounter              = (((status->raw[7] & 0x40) >> 2) |
                                       ((status->raw[7] & 0x20) >> 2) |
                                       ((status->raw[7] & 0x08) >> 1) |
                                       ((status->raw[7] & 0x04) >> 1) |
                                       ((status->raw[7] & 0x02) >> 1));

    // printer status 7
    status->presenterState          = (((status->raw[8] & 0x08) >> 1) |
                                       ((status->raw[8] & 0x04) >> 1) |
                                       ((status->raw[8] & 0x02) >> 1));

    return STARIO_ERROR_SUCCESS;
}

static long usbBeginCheckedBlock (char const * portName)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long ioResult = usbGetStarPrinterStatus(portName, &usbPort->statusCache);

    if (ioResult != STARIO_ERROR_SUCCESS)
    {
        return ioResult;
    }

    if (usbPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return STARIO_ERROR_SUCCESS;
}

static long usbEndCheckedBlock (char const * portName, StarPrinterStatus * status)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    if (usbPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    memset(status, 0x00, sizeof(StarPrinterStatus));
    status->offline = 1;

    long ioResult = STARIO_ERROR_SUCCESS;

    do
    {
        char etb[1] = {0x17};

        ioResult = usbWritePort(portName, etb, 1);

        if (ioResult == 1)
        {
            unsigned char nextEtbCounter = (usbPort->statusCache.etbCounter + 1) % 32;

            do
            {
                ioResult = usbGetStarPrinterStatus(portName, status);

                if (ioResult == STARIO_ERROR_IO_FAIL)
                {
                    ioResult = usbGetPortSignals(portName);

                    if (ioResult >= STARIO_ERROR_SUCCESS)
                    {
                        if ((((unsigned char) ioResult) & 0x10) == 0)
                        {
                            status->offline = 1;
                        }
                        else
                        {
                            status->offline = 0;
                        }
                    }
                }

                if (ioResult == STARIO_ERROR_NOT_OPEN)
                {
                    return ioResult;
                }

                if ((ioResult >= STARIO_ERROR_SUCCESS) &&
                    (status->offline == 0) &&
                    (status->etbCounter != nextEtbCounter))
                {
                    struct timeval sleepTime = {0, 200 * 1000};

                    select(0, NULL, NULL, NULL, &sleepTime);

                    continue;
                }

                break;
            } while (1);
        }

        if (ioResult == STARIO_ERROR_NOT_OPEN)
        {
            return ioResult;
        }

        if (status->offline)
        {
            ioResult = usbHdwrResetDevice(portName);

            if (ioResult != STARIO_ERROR_SUCCESS)
            {
                return ioResult;
            }
        }
    } while (0);

    return STARIO_ERROR_SUCCESS;
}

static long usbHdwrResetDevice (char const * portName)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    if (USB_CONTROL_MSG(usbPort->udev, (char) 0x40, (char) 2, (short) 0, (short) 0, NULL, (short) 0, USB_CONTROL_MSG_TIMEOUT) < 0)
    {
        if (errno == ENODEV)
        {
            usbClosePort(portName);

            return STARIO_ERROR_NOT_OPEN;
        }

        return STARIO_ERROR_IO_FAIL;
    }

    return STARIO_ERROR_SUCCESS;
}

static long usbDoVisualCardCmd (char const * portName, VisualCardCmd * request, long timeoutMillis)
{
    long ioResult       = STARIO_ERROR_SUCCESS;
    long timeRemaining  = timeoutMillis;
    long i              = 0;
    char const stx      = 0x02;
    char const etx      = 0x03;
    char const ack      = 0x06;
    char const dle      = 0x10;
    char const nak      = 0x15;
    char bcc            = 0;

    struct timeval timeS;
    struct timeval timeF;
    long interval = 0;

    // send ACK to confirm any packets received after previous timeout
    //printf("tx leading ack\n");
    ioResult = usbWritePort(portName, &ack, 1);
    if (ioResult == STARIO_ERROR_NOT_OPEN)
    {
        //printf("no dev\n");
        return STARIO_ERROR_NOT_OPEN;
    }

    if (ioResult != 1)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    // clear usb input buffer
    //printf("clear serial in buf\n");
    do
    {
        char bitBucket[1 + 1 + 1 + 128 + 1 + 1];
        ioResult = usbReadPort(portName, bitBucket, sizeof(bitBucket));
        if (ioResult == STARIO_ERROR_NOT_OPEN)
        {
            //printf("no dev\n");
            return STARIO_ERROR_NOT_OPEN;
        }
        //printf("cleared %d bytes\n", (int) ioResult);
    } while (ioResult > 0);

    long txCmdLength = 1 + 1 + request->txDataLength + 1 + 1;
    char * txCmd = malloc(txCmdLength);
    if (txCmd == NULL)
    {
        return STARIO_ERROR_RUNTIME;
    }

    long cmdIdx = 0;

    txCmd[cmdIdx++]             = stx;
    bcc = txCmd[cmdIdx++]       = request->command;
    for (i = 0; i < request->txDataLength; i++)
    {
        bcc ^= txCmd[cmdIdx++]  = request->txData[i];
    }
    bcc ^= txCmd[cmdIdx++]      = etx;
    txCmd[cmdIdx++]             = bcc;

    unsigned char txSuccess = 0;

    // transmit command loop
    //printf("enter tx cmd loop\n");
    while (timeRemaining > 0)
    {
        //printf("timeRemaining = %d\n", (int) timeRemaining);

        //printf("tx cmd\n");
        ioResult = usbWritePort(portName, txCmd, txCmdLength);
        if (ioResult == STARIO_ERROR_NOT_OPEN)
        {
            //printf("no dev\n");
            free(txCmd);
            return STARIO_ERROR_NOT_OPEN;
        }

        if (ioResult != txCmdLength)
        {
            //printf("fail - could not tx cmd\n");
            free(txCmd);
            return STARIO_ERROR_IO_FAIL;
        }

        char response[1];

        // device ACK / NAK response read loop
        //printf("enter ack/nak resp loop\n");
        while (timeRemaining > 0)
        {
            //printf("timeRemaining = %d\n", (int) timeRemaining);

            //printf("rx resp\n");
            GET_TIME(timeS);
            ioResult = usbReadPort(portName, response, 1);
            GET_TIME(timeF);
            interval = TIME_DIFF(timeS,timeF);
            //printf("rx took %d millisec\n", (int) interval);

            if (ioResult == STARIO_ERROR_NOT_OPEN)
            {
                //printf("no dev\n");
                free(txCmd);
                return STARIO_ERROR_NOT_OPEN;
            }

            if (ioResult < 1)
            {
                //printf("no resp\n");
                if (timeRemaining > interval)
                {
                    timeRemaining -= interval;
                }
                else
                {
                    timeRemaining = 0;
                }

                //printf("20 millisec sleep\n");
                if (timeRemaining > 20)
                {
                    struct timeval sleepTime = {0, 20 * 1000};
                    select(0, NULL, NULL, NULL, &sleepTime);
                    timeRemaining -= 20;
                }
                else
                {
                    timeRemaining = 0;
                }

                continue;
            }

            if (response[0] == ack)
            {
                //printf("got ack\n");
                txSuccess = ack;
                break;
            }
            else if (response[0] == nak)
            {
                //printf("got nak\n");
                txSuccess = nak;
                break;
            }
            else if (response[0] == dle)
            {
                //printf("got dle\n");
                free(txCmd);
                return STARIO_ERROR_DLE;
            }
            else
            {
                //printf("got other\n");
                free(txCmd);
                return STARIO_ERROR_IO_FAIL;
            }
        }
        //printf("exit ack/nak resp loop\n");

        if (txSuccess == ack)
        {
            break;
        }
    }
    //printf("exit tx cmd loop\n");
    //printf("timeRemaining = %d\n", (int) timeRemaining);

    free(txCmd);

    if (timeRemaining == 0)
    {
        if (txSuccess == nak)
        {
            return STARIO_ERROR_NAK;
        }
        else
        {
            return STARIO_ERROR_NO_RESPONSE;
        }
    }

    timeRemaining = timeoutMillis;

    // read status loop - outer
    //printf("enter read status loop outer\n");
    while (timeRemaining > 0)
    {
        //printf("timeRemaining = %d\n", (int) timeRemaining);

        long rxCmdLength = 0;
        char rxCmd[1 + 1 + 1 + 128 + 1 + 1];

        // read status loop - inner
        //printf("enter read status loop inner\n");
        while (timeRemaining > 0)
        {
            //printf("timeRemaining = %d\n", (int) timeRemaining);

            //printf("rx resp\n");

            GET_TIME(timeS);
            ioResult = usbReadPort(portName, &rxCmd[rxCmdLength], sizeof(rxCmd) - rxCmdLength);
            GET_TIME(timeF);
            interval = TIME_DIFF(timeS,timeF);
            //printf("rx took %d millisec\n", (int) interval);

            if (ioResult == STARIO_ERROR_NOT_OPEN)
            {
                //printf("no dev\n");
                return STARIO_ERROR_NOT_OPEN;
            }

            if (ioResult > STARIO_ERROR_SUCCESS)
            {
                //printf("read %d bytes\n", (int) ioResult);
                rxCmdLength += ioResult;
            }

            if (rxCmdLength >= 5)
            {
                if (rxCmd[rxCmdLength - 2] == etx)
                {
                    //printf("found etx\n");
                    break;
                }
            }

            if (ioResult < 1)
            {
                if (timeRemaining > interval)
                {
                    timeRemaining -= interval;
                }
                else
                {
                    timeRemaining = 0;
                }

                //printf("20 millisecond sleep\n");
                if (timeRemaining > 20)
                {
                    struct timeval sleepTime = {0, 20 * 1000};
                    select(0, NULL, NULL, NULL, &sleepTime);
                    timeRemaining -= 20;
                }
                else
                {
                    timeRemaining = 0;
                }
            }
        }
        //printf("exit read status loop inner\n");
        //printf("timeRemaining = %d\n", (int) timeRemaining);

        if (timeRemaining == 0)
        {
            return STARIO_ERROR_NO_RESPONSE;
        }

        bcc = rxCmd[1];
        for (i = 2; i < rxCmdLength - 1; i++)
        {
            bcc ^= rxCmd[i];
        }

        if (bcc == rxCmd[rxCmdLength - 1])
        {
            //printf("resp OK - tx ack\n");

            ioResult = usbWritePort(portName, &ack, 1);
            if (ioResult == STARIO_ERROR_NOT_OPEN)
            {
                return STARIO_ERROR_NOT_OPEN;
            }

            if (ioResult != 1)
            {
                return STARIO_ERROR_IO_FAIL;
            }

            request->status = rxCmd[2];
            memcpy(request->rxData, &rxCmd[3], rxCmdLength - 5);
            request->rxDataLength = rxCmdLength - 5;

            break;
        }

        //printf("resp malformed - tx nak\n");

        ioResult = usbWritePort(portName, &nak, 1);
        if (ioResult == STARIO_ERROR_NOT_OPEN)
        {
            return STARIO_ERROR_NOT_OPEN;
        }

        if (ioResult != 1)
        {
            return STARIO_ERROR_IO_FAIL;
        }

        if (timeRemaining > 10)
        {
            //printf("10 millisec sleep\n");

            struct timeval sleepTime = {0, 10 * 1000};
            select(0, NULL, NULL, NULL, &sleepTime);

            timeRemaining -= 10;
        }
        else
        {
            timeRemaining = 0;
        }
    }
    //printf("exit read status loop outer\n");
    //printf("timeRemaining = %d\n", (int) timeRemaining);

    if (timeRemaining == 0)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    return STARIO_ERROR_SUCCESS;
}

static long usbClosePort (char const * portName)
{
    USBPort * usbPort = usbFindPort(portName);
    if (usbPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    USB_RELEASE_INTERFACE(usbPort->udev, usbPort->interface);
    USB_CLOSE(usbPort->udev);

    memset(usbPort, 0x00, sizeof(USBPort));

    return STARIO_ERROR_SUCCESS;
}

static void usbReleaseImpl ()
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (usbPorts[i].set != 0)
        {
            usbClosePort(usbPorts[i].portName);
        }
    }

#ifdef RPMBUILD
    dlclose(libusb);
    libusb = NULL;
#endif
}

