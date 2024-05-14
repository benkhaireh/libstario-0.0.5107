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
#include <fcntl.h>
#include <unistd.h>
#include <memory.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "stario-error.h"
#include "stario-serial.h"

static long serMatchPortName        (char const * portName);
static long serOpenPort             (char const * portName, char const * portSettings);
static long serWritePort            (char const * portName, char const * writeBuffer, long length);
static long serReadPortPrv          (char const * portName, char * readBuffer, long length, long minLength, long timeMillis);
static long serReadPort             (char const * portName, char * readBuffer, long length);
static long serGetStarPrinterStatus (char const * portName, StarPrinterStatus * status);
static long serBeginCheckedBlock    (char const * portName);
static long serEndCheckedBlock      (char const * portName, StarPrinterStatus * status);
static long serHdwrResetDevice      (char const * portName);
static long serDoVisualCardCmd      (char const * portName, VisualCardCmd * request, long timeoutMillis);
static long serClosePort            (char const * portName);
static void serReleaseImpl          ();

#define MAX_NUM_PORTS 20
#define MAX_COM_PORT  99

typedef struct
{
    long baud;
    char parity;
    char dataBits;
    char stopBits;
    char flowControl;
} SerPortSettings;

typedef struct
{
    unsigned char set;
    char portName[100];
    char portSettings[100];
    int port;

    SerPortSettings originalPortSettings;

    StarPrinterStatus statusCache;
} SerPort;

SerPort serPorts[MAX_NUM_PORTS];
unsigned char portHasBeenInitialized[MAX_COM_PORT];

PortImpl getSerPortImpl()
{
    memset(serPorts, 0x00, sizeof(serPorts));

    PortImpl impl;

    impl.matchPortName          = serMatchPortName;
    impl.openPort               = serOpenPort;
    impl.writePort              = serWritePort;
    impl.readPort               = serReadPort;
    impl.getStarPrinterStatus   = serGetStarPrinterStatus;
    impl.beginCheckedBlock      = serBeginCheckedBlock;
    impl.endCheckedBlock        = serEndCheckedBlock;
    impl.hdwrResetDevice        = serHdwrResetDevice;
    impl.doVisualCardCmd        = serDoVisualCardCmd;
    impl.closePort              = serClosePort;
    impl.releaseImpl            = serReleaseImpl;

    return impl;
}

static SerPort * serFindPort(char const * portName)
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (serPorts[i].set != 0)
            if (strcmp(serPorts[i].portName, portName) == 0)
                break;
    }

    if (i == MAX_NUM_PORTS)
    {
        return NULL;
    }

    return &serPorts[i];
}

static long serMatchPortName (char const * portName)
{
    if (strncmp(portName, "/dev/ttyS", 9) == 0)
    {
        return STARIO_ERROR_SUCCESS;
    }

    return STARIO_ERROR_NOT_AVAILABLE;
}


static long serConfigurePort(SerPort * serPort, unsigned char saveSettings, SerPortSettings * settings)
{
    struct termios options;

    if (tcgetattr(serPort->port, &options) == -1)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    if (settings->baud != 0)
    {
        speed_t baudRate = B9600;
        switch (settings->baud)
        {
        case 38400:	baudRate = B38400;	break;
        case 19200:	baudRate = B19200;	break;
        case 9600:	baudRate = B9600;	break;
        case 4800:	baudRate = B4800;	break;
        case 2400:	baudRate = B2400;	break;
        }

        if ((cfsetispeed(&options, baudRate) == -1) || (cfsetospeed(&options, baudRate) == -1))
        {
            return STARIO_ERROR_IO_FAIL;
        }

        options.c_cflag |= (CLOCAL | CREAD);
    }

    if (settings->parity != 0)
    {
        if (settings->parity == 'n')
        {
            options.c_cflag &= ~PARENB;
        }
        else if (settings->parity == 'e')
        {
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_iflag |= (INPCK | ISTRIP);
        }
        else if (settings->parity == 'o')
        {
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            options.c_iflag |= (INPCK | ISTRIP);
        }
    }

    if (settings->dataBits != 0)
    {
        if (settings->dataBits == 8)
        {
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS8;
        }
        else if (settings->dataBits == 7)
        {
            options.c_cflag &= ~CSIZE;
            options.c_cflag |= CS7;
        }
    }

    if (settings->stopBits != 0)
    {
        if (settings->stopBits == 1)
        {
            options.c_cflag &= ~CSTOPB;
        }
        else if (settings->stopBits == 2)
        {
            options.c_cflag |= CSTOPB;
        }
    }

    if (settings->flowControl != 0)
    {
        if (settings->flowControl == 'n')
        {
            options.c_cflag &= ~CRTSCTS;
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
        }
        else if (settings->flowControl == 'h')
        {
            // DSR signal checked manually during transmission
            options.c_cflag &= ~CRTSCTS;
            options.c_iflag &= ~(IXON | IXOFF | IXANY);
        }
    }

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    tcflush(serPort->port, TCIFLUSH);

    if (tcsetattr(serPort->port, TCSANOW, &options) == -1)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    if (saveSettings != 0)
    {
        memcpy(&serPort->originalPortSettings, settings, sizeof(SerPortSettings));
    }

    return STARIO_ERROR_SUCCESS;
}

static long serOpenPort (char const * portName, char const * portSettings)
{
    SerPort * oldSerPort = serFindPort(portName);
    if (oldSerPort != NULL)
    {
        return STARIO_ERROR_SUCCESS;
    }

    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (serPorts[i].set == 0)
            break;
    }
    if ( i == MAX_NUM_PORTS)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    SerPort serPort;

    memset(&serPort, 0x00, sizeof(SerPort));

    serPort.set = 1;

    strcpy(serPort.portSettings, portSettings);

    SerPortSettings settings;
    memset(&settings, 0x00, sizeof(SerPortSettings));

    char * baudToken = serPort.portSettings;
    char * parityToken = NULL;
    char * dataBitsToken = NULL;
    char * stopBitsToken = NULL;
    char * flowControlToken = NULL;

    do
    {
        baudToken = serPort.portSettings;
        if ((parityToken        = strstr(baudToken,         ",")) == NULL) break;
        if ((dataBitsToken      = strstr(++parityToken,     ",")) == NULL) break;
        if ((stopBitsToken      = strstr(++dataBitsToken,   ",")) == NULL) break;
        if ((flowControlToken   = strstr(++stopBitsToken,   ",")) == NULL) break;
        ++flowControlToken;
    } while (0);

    if ((baudToken == NULL) || (parityToken == NULL) || (dataBitsToken == NULL) || (stopBitsToken == NULL) || (flowControlToken == NULL))
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    settings.baud = atol(baudToken);
    if ((settings.baud != 38400) &&
        (settings.baud != 19200) &&
        (settings.baud !=  9600) &&
        (settings.baud !=  4800) &&
        (settings.baud !=  2400))
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (strncmp(parityToken, "none", strlen("none")) == 0)
    {
        settings.parity = 'n';
    }
    else if (strncmp(parityToken, "even", strlen("even")) == 0)
    {
        settings.parity = 'e';
    }
    else if (strncmp(parityToken, "odd", strlen("odd")) == 0)
    {
        settings.parity = 'o';
    }
    else
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    settings.dataBits = atol(dataBitsToken);
    if ((settings.dataBits != 8) &&
        (settings.dataBits != 7))
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    settings.stopBits = atol(stopBitsToken);
    if ((settings.stopBits != 1) &&
        (settings.stopBits != 2))
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (strncmp(flowControlToken, "none", strlen("none")) == 0)
    {
        settings.flowControl = 'n';
    }
    else if (strncmp(flowControlToken, "hdwr", strlen("hdwr")) == 0)
    {
        settings.flowControl = 'h';
    }
    else
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    strcpy(serPort.portName, portName);

    serPort.port = open(serPort.portName, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serPort.port == -1)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long configureSuccess = serConfigurePort(&serPort, 1, &settings);

    if (configureSuccess != STARIO_ERROR_SUCCESS)
    {
        close(serPort.port);

        return configureSuccess;
    }

    memcpy(&serPorts[i], &serPort, sizeof(SerPort));

    if (portHasBeenInitialized[atoi(&serPort.portName[9])] == 0)
    {
        portHasBeenInitialized[atoi(&serPort.portName[9])] = 1;

        serHdwrResetDevice(portName);
    }

    return STARIO_ERROR_SUCCESS;
}

static long serWritePort (char const * portName, char const * writeBuffer, long length)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long timeout = 5 * 1000;

    long writeLength = (long) length;
    long totalWriteLength = 0;
    long partialWriteLength = 0;

    char const * bufferElements = writeBuffer;

    while ((totalWriteLength < writeLength) && (timeout > 0))
    {
        if (serPort->originalPortSettings.flowControl == 'h')
        {
            while (timeout > 0)
            {
                int portStatus = 0;
                if (ioctl(serPort->port, TIOCMGET, &portStatus) == 0)
                {
                    if ((portStatus & TIOCM_DSR) != 0)
                    {
                        break;
                    }
                }
                else
                {
                    return STARIO_ERROR_IO_FAIL;
                }

                struct timeval sleepTime = {0, 20 * 1000};

                select(0, NULL, NULL, NULL, &sleepTime);

                if (timeout > 20)
                    timeout -= 20;
                else
                    timeout = 0;
            }
            if (timeout == 0)
            {
                break;
            }
        }

        if (serPort->originalPortSettings.flowControl == 'h')
        {
            partialWriteLength = write(serPort->port, &bufferElements[totalWriteLength], ((writeLength - totalWriteLength) < 256)?(writeLength - totalWriteLength):256);
        }
        else
        {
            partialWriteLength = write(serPort->port, &bufferElements[totalWriteLength], (writeLength - totalWriteLength));
        }

        if (partialWriteLength == -1)
        {
            return STARIO_ERROR_IO_FAIL;
        }

        totalWriteLength += partialWriteLength;

        if (tcdrain(serPort->port) != 0)
        {
            return STARIO_ERROR_IO_FAIL;
        }

        if (partialWriteLength > 0)
        {
            timeout = 5 * 1000;
        }
    }

    return totalWriteLength;
}

static long serReadPortPrv (char const * portName, char * readBuffer, long length, long minLength, long timeMillis)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    int availableReadLength = 0;
    int timeout = timeMillis;

    while (timeout > 0)
    {
        int startingAvailableReadLength = availableReadLength;

        if (ioctl(serPort->port, FIONREAD, &availableReadLength) != 0)
        {
            return STARIO_ERROR_IO_FAIL;
        }

        if (availableReadLength >= minLength)
        {
            break;
        }

        struct timeval sleepTime = {0, 20 * 1000};

        select(0, NULL, NULL, NULL, &sleepTime);

        if (startingAvailableReadLength != availableReadLength)
        {
            timeout = timeMillis;
        }
        else
        {
            if (timeout > 20)
                timeout -= 20;
            else
                timeout = 0;
        }
    }

    if (availableReadLength < minLength)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    if (availableReadLength == 0)
    {
        return 0;
    }

    if (availableReadLength < length)
    {
        length = availableReadLength;
    }

    length = read(serPort->port, readBuffer, length);

    if (length == -1)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    return length;
}

static long serReadPort (char const * portName, char * readBuffer, long length)
{
    if (length == 0)
    {
        return 0;
    }

    return serReadPortPrv(portName, readBuffer, length, 1, 200);
}

static long serGetStarPrinterStatus (char const * portName, StarPrinterStatus * status)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    memset(status, 0x00, sizeof(StarPrinterStatus));

    long ioResult = STARIO_ERROR_SUCCESS;

    char const statusReqCmd[] = {0x1b, 0x06, 0x01};
    if (write(serPort->port, statusReqCmd, 3) != 3)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    ioResult = serReadPortPrv(portName, status->raw, sizeof(status->raw), 7, 200);

    if (ioResult < STARIO_ERROR_SUCCESS)
    {
        return ioResult;
    }

    if (ioResult < 7)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    long statusLength = 0;

    switch (status->raw[0])
    {
        case 0x0f:  statusLength =  7; break;
        case 0x21:  statusLength =  8; break;
        case 0x23:  statusLength =  9; break;
        case 0x25:  statusLength = 10; break;
        case 0x27:  statusLength = 11; break;
        case 0x29:  statusLength = 12; break;
        case 0x2b:  statusLength = 13; break;
        case 0x2d:  statusLength = 14; break;
        case 0x2f:  statusLength = 15; break;
    }

    ioResult += serReadPortPrv(portName, &status->raw[ioResult], statusLength - ioResult, statusLength - ioResult, 200);

    if (ioResult < STARIO_ERROR_SUCCESS)
    {
        return ioResult;
    }

    if (ioResult != statusLength)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    status->rawLength = (unsigned char) ioResult;

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

static long serBeginCheckedBlock (char const * portName)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    long ioResult = serGetStarPrinterStatus(portName, &serPort->statusCache);

    if (ioResult != STARIO_ERROR_SUCCESS)
    {
        return ioResult;
    }

    if (serPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return STARIO_ERROR_SUCCESS;
}

static long serEndCheckedBlock (char const * portName, StarPrinterStatus * status)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    if (serPort->statusCache.etbAvailable == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    memset(status, 0x00, sizeof(StarPrinterStatus));
    status->offline = 1;

    long ioResult = STARIO_ERROR_SUCCESS;

    do
    {
        char etb[1] = {0x17};

        ioResult = write(serPort->port, etb, 1);

        if (ioResult == 1)
        {
            unsigned char nextEtbCounter = (serPort->statusCache.etbCounter + 1) % 32;

            do
            {
                ioResult = serGetStarPrinterStatus(portName, status);

                if (ioResult != STARIO_ERROR_SUCCESS)
                {
                    ioResult = STARIO_ERROR_SUCCESS;

                    int portStatus = 0;
                    if (ioctl(serPort->port, TIOCMGET, &portStatus) == 0)
                    {
                        status->offline = ((portStatus & TIOCM_DSR) == 0)?1:0;
                    }
                    else
                    {
                        ioResult = STARIO_ERROR_IO_FAIL;
                    }
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
            ioResult = serHdwrResetDevice(portName);

            if (ioResult != STARIO_ERROR_SUCCESS)
            {
                return ioResult;
            }
        }
    } while (0);

    return STARIO_ERROR_SUCCESS;
}

static long serHdwrResetDevice (char const * portName)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    do
    {
        unsigned int mstat;

        if (ioctl(serPort->port, TIOCMGET, &mstat))
            break;

        mstat &= ~TIOCM_DTR;

        if (ioctl(serPort->port, TIOCMSET, &mstat))
            break;

        struct timeval sleepTime = {0, 10 * 1000};

        select(0, NULL, NULL, NULL, &sleepTime);

        mstat |= TIOCM_DTR;

        if (ioctl(serPort->port, TIOCMSET, &mstat))
            break;

        return STARIO_ERROR_SUCCESS;
    } while (0);

    return STARIO_ERROR_IO_FAIL;
}

static long serDoVisualCardCmd (char const * portName, VisualCardCmd * request, long timeoutMillis)
{
    long ioResult = STARIO_ERROR_SUCCESS;
    long timeRemaining = timeoutMillis;
    long i = 0;
    char const stx = 0x02;
    char const etx = 0x03;
    char const ack = 0x06;
    char const dle = 0x10;
    char const nak = 0x15;
    char bcc = 0;

    struct timeval timeS;
    struct timeval timeF;
    long interval = 0;

    // send ACK to confirm any packets received after previous timeout
    //printf("tx leading ack\n");
    ioResult = serWritePort(portName, &ack, 1);
    if (ioResult != 1)
    {
        return STARIO_ERROR_IO_FAIL;
    }

    // clear serial input buffer
    //printf("clear serial in buf\n");
    do
    {
        char bitBucket[1 + 1 + 1 + 128 + 1 + 1];
        ioResult = serReadPortPrv(portName, bitBucket, sizeof(bitBucket), 1, 10);
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
        ioResult = serWritePort(portName, txCmd, txCmdLength);

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
            ioResult = serReadPortPrv(portName, response, 1, 1, timeRemaining);
            GET_TIME(timeF);
            interval = TIME_DIFF(timeS,timeF);
            //printf("rx took %d millisec\n", (int) interval);

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
            if (rxCmdLength < 5)
            {
                ioResult = serReadPortPrv(portName, &rxCmd[rxCmdLength], sizeof(rxCmd) - rxCmdLength, 5 - rxCmdLength, timeRemaining);
            }
            else
            {
                ioResult = serReadPortPrv(portName, &rxCmd[rxCmdLength], sizeof(rxCmd) - rxCmdLength, 1, timeRemaining);
            }
            GET_TIME(timeF);
            interval = TIME_DIFF(timeS,timeF);
            //printf("rx took %d millisec\n", (int) interval);

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
                //printf("no resp\n");

                if (timeRemaining > interval)
                {
                    timeRemaining -= interval;
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

            ioResult = serWritePort(portName, &ack, 1);

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

        ioResult = serWritePort(portName, &nak, 1);

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

static long serClosePort (char const * portName)
{
    SerPort * serPort = serFindPort(portName);
    if (serPort == NULL)
    {
        return STARIO_ERROR_NOT_OPEN;
    }

    tcflush(serPort->port, TCIOFLUSH);

    close(serPort->port);

    memset(serPort, 0x00, sizeof(SerPort));

    return STARIO_ERROR_SUCCESS;
}

static void serReleaseImpl ()
{
    int i = 0;
    for (; i < MAX_NUM_PORTS; i++)
    {
        if (serPorts[i].set != 0)
        {
            serClosePort(serPorts[i].portName);
        }
    }
}

