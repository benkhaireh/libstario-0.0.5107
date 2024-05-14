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
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include <linux/parport.h>

#include "stario-error.h"
#include "stario-parallel.h"

static long parMatchPortName        (char const * portName);
static long parOpenPort             (char const * portName, char const * portSettings);
static long parWritePort            (char const * portName, char const * writeBuffer, long length);
static long parReadPort             (char const * portName, char * readBuffer, long length);
static long parGetStarPrinterStatus (char const * portName, StarPrinterStatus * status);
static long parBeginCheckedBlock    (char const * portName);
static long parEndCheckedBlock      (char const * portName, StarPrinterStatus * status);
static long parHdwrResetDevice      (char const * portName);
static long parClosePort            (char const * portName);
static void parReleaseImpl          ();

#define MAX_NUM_PORTS 20

typedef struct
{
    unsigned char set;
    char portName[100];
    int port;

    StarPrinterStatus statusCache;
} ParPort;

ParPort parPorts[MAX_NUM_PORTS];

PortImpl getParPortImpl()
{
    memset(parPorts, 0x00, sizeof(parPorts));

    PortImpl impl;

    impl.matchPortName          = parMatchPortName;
    impl.openPort               = parOpenPort;
    impl.writePort              = parWritePort;
    impl.readPort               = parReadPort;
    impl.getStarPrinterStatus   = parGetStarPrinterStatus;
    impl.beginCheckedBlock      = parBeginCheckedBlock;
    impl.endCheckedBlock        = parEndCheckedBlock;
    impl.hdwrResetDevice        = parHdwrResetDevice;
    impl.doVisualCardCmd        = NULL;
    impl.closePort              = parClosePort;
    impl.releaseImpl            = parReleaseImpl;

    return impl;
}

static ParPort * parFindPort(char const * portName)
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (parPorts[i].set != 0)
            if (strcmp(parPorts[i].portName, portName) == 0)
                break;
    }

    if (i == MAX_NUM_PORTS)
    {
        return NULL;
    }

    return &parPorts[i];
}

static long parMatchPortName (char const * portName)
{
    if (strncmp(portName, "/dev/parport", 12) == 0)
    {
        return STARIO_ERROR_SUCCESS;
    }

    return STARIO_ERROR_NOT_AVAILABLE;
}

static long parOpenPort (char const * portName, char const * portSettings)
{
    ParPort * oldParPort = parFindPort(portName);
    if (oldParPort != NULL)
    {
        return STARIO_ERROR_SUCCESS;
    }

    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (parPorts[i].set == 0)
            break;
    }
    if ( i == MAX_NUM_PORTS)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    ParPort parPort;

    memset(&parPort, 0x00, sizeof(ParPort));

    parPort.set = 1;

    strcpy(parPort.portName, portName);

    parPort.port = open(parPort.portName, O_RDWR | O_NONBLOCK);
    if (parPort.port == -1)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    if (ioctl(parPort.port, PPCLAIM))
    {
        close(parPort.port);

        return STARIO_ERROR_NOT_OPEN;
    }

    memcpy(&parPorts[i], &parPort, sizeof(ParPort));

    return STARIO_ERROR_SUCCESS;
}

static unsigned char getOnlineStatus(ParPort * parPort)
{
    int mode = IEEE1284_MODE_COMPAT;
    if (ioctl(parPort->port, PPNEGOT, &mode))
    {
        return 0;
    }

    unsigned char portError = 0;
    if (ioctl(parPort->port, PPRSTATUS, &portError))
    {
        return 0;
    }

    return (((portError & PARPORT_STATUS_ERROR) != 0) && ((portError & PARPORT_STATUS_BUSY) != 0) )     ?1:0;
}

static long parWritePort (char const * portName, char const * writeBuffer, long length)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long reqWriteLength             = length;
    long subWriteLength             = 0;
    long writeLength                = 0;
    char const * writeBufferPtr     = NULL;
    long timeout                    = 5000;

    writeBufferPtr = writeBuffer;

    if (getOnlineStatus(parPort))
    {
        if ((subWriteLength = write(parPort->port, writeBufferPtr, (long) reqWriteLength)) != -1)
            writeLength = subWriteLength;
    }

    while ((writeLength < (long) reqWriteLength) && (timeout > 0))
    {
        if (subWriteLength <= 0)
        {
            struct timeval sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_usec = 50 * 1000;

            select(0, NULL, NULL, NULL, &sleepTime);

            if (timeout < 50)
                timeout = 0;
            else
                timeout -= 50;
        }
        else
        {
            timeout = 5000;
        }

        subWriteLength = 0;

        if (getOnlineStatus(parPort) == 0)
        {
            continue;
        }

        writeBufferPtr = (char *) &writeBuffer[writeLength];
        if ((subWriteLength = write(parPort->port, writeBufferPtr, (long) (reqWriteLength - writeLength))) != -1)
            writeLength += subWriteLength;
    }

    return writeLength;
}

static long parReadPort (char const * portName, char * readBuffer, long length)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    int mode = IEEE1284_MODE_COMPAT;
    ioctl(parPort->port, PPNEGOT, &mode);
    mode = IEEE1284_MODE_NIBBLE;
    if (ioctl(parPort->port, PPNEGOT, &mode))
    {
        return STARIO_ERROR_IO_FAIL;
    }

    long readLength = 0;

    if ((readLength = read(parPort->port, readBuffer, length)) < 0)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    return readLength;
}

static long parGetStarPrinterStatus (char const * portName, StarPrinterStatus * status)
{
    memset(status, 0x00, sizeof(StarPrinterStatus));

    long readResult = parReadPort(portName, status->raw, sizeof(status->raw));

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

static long parBeginCheckedBlock (char const * portName)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long ioResult = parGetStarPrinterStatus(portName, &parPort->statusCache);

    if (ioResult != STARIO_ERROR_SUCCESS)
    {
        return ioResult;
    }

    if (parPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return STARIO_ERROR_SUCCESS;
}

static long parEndCheckedBlock (char const * portName, StarPrinterStatus * status)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    if (parPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    memset(status, 0x00, sizeof(StarPrinterStatus));
    status->offline = 1;

    long ioResult = STARIO_ERROR_SUCCESS;

    do
    {
        char etb[1] = {0x17};

        ioResult = parWritePort(portName, etb, 1);

        if (ioResult == 1)
        {
            unsigned char nextEtbCounter = (parPort->statusCache.etbCounter + 1) % 32;

            do
            {
                ioResult = parGetStarPrinterStatus(portName, status);

                if (ioResult == STARIO_ERROR_IO_FAIL)
                {
                    ioResult = STARIO_ERROR_SUCCESS;

                    status->offline = getOnlineStatus(parPort)?0:1;
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

        if (status->offline)
        {
            ioResult = parHdwrResetDevice(portName);

            if (ioResult != STARIO_ERROR_SUCCESS)
            {
                return ioResult;
            }
        }
    } while (0);

    return STARIO_ERROR_SUCCESS;
}

static long parHdwrResetDevice (char const * portName)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    int mode = IEEE1284_MODE_COMPAT;
    if (ioctl(parPort->port, PPNEGOT, &mode))
    {
        return STARIO_ERROR_IO_FAIL;
    }

    do
    {
        struct ppdev_frob_struct frob = {PARPORT_CONTROL_INIT, 0};
        if (ioctl(parPort->port, PPFCONTROL, &frob))
            break;

        struct timeval sleepTime;
        sleepTime.tv_sec = 0;
        sleepTime.tv_usec = 1000;

        select(0, NULL, NULL, NULL, &sleepTime);

        frob.val = PARPORT_CONTROL_INIT;
        if (ioctl(parPort->port, PPFCONTROL, &frob))
            break;

        return STARIO_ERROR_SUCCESS;
    } while (0);

    return STARIO_ERROR_IO_FAIL;
}

static long parClosePort (char const * portName)
{
    ParPort * parPort = parFindPort(portName);
    if (parPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    ioctl(parPort->port, PPRELEASE);

    close(parPort->port);

    memset(parPort, 0x00, sizeof(ParPort));

    return STARIO_ERROR_SUCCESS;
}

static void parReleaseImpl ()
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (parPorts[i].set != 0)
        {
            parClosePort(parPorts[i].portName);
        }
    }
}

