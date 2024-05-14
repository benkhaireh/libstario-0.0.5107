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

#ifndef _included_stario_prvstructures
#define _included_stario_prvstructures

#include "stario-structures.h"

#define GET_TIME(time) (gettimeofday(&time, NULL))
#define TIME_DIFF(start,finish) (((finish.tv_sec  - start.tv_sec ) * 1e3) + ((finish.tv_usec - start.tv_usec) / 1e3))

#define MAX_NUM_PORTS          20

// Serial, Parallel, and USB supported - 3 impls
#define USB_IMPL_IDX            0
#define PAR_IMPL_IDX            1
#define SER_IMPL_IDX            2
#define NUM_IMPLS               3

typedef struct
{
    long (* matchPortName)          (char const * portName);
    long (* openPort)               (char const * portName, char const * portSettings);

    // printer api
    long (* writePort)              (char const * portName, char const * writeBuffer, long length);
    long (* readPort)               (char const * portName, char * readBuffer, long length);
    long (* getStarPrinterStatus)   (const char * portName, StarPrinterStatus * status);
    long (* beginCheckedBlock)      (char const * portName);
    long (* endCheckedBlock)        (char const * portName, StarPrinterStatus * status);
    long (* hdwrResetDevice)        (char const * portName);

    // visual card api
    long (* doVisualCardCmd)        (char const * portName, VisualCardCmd * request, long timeoutMillis);

    long (* closePort)              (char const * portName);
    void (* releaseImpl)            ();
} PortImpl;

#endif
