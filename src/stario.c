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

#include "stario.h"
#include "stario-error.h"
#include "stario-prvstructures.h"
#include "stario-usb.h"
#include "stario-parallel.h"
#include "stario-serial.h"

static PortImpl impls[NUM_IMPLS];

void __attribute__ ((constructor)) libConstructor(void)
{
    impls[USB_IMPL_IDX] = getUsbPortImpl();
    impls[PAR_IMPL_IDX] = getParPortImpl();
    impls[SER_IMPL_IDX] = getSerPortImpl();
}

void __attribute__ ((destructor)) libDestructor(void)
{
    impls[USB_IMPL_IDX].releaseImpl();
    impls[PAR_IMPL_IDX].releaseImpl();
    impls[SER_IMPL_IDX].releaseImpl();
}

static long getSupportingImplIdx(char const * portName)
{
    if (impls[USB_IMPL_IDX].matchPortName(portName) == STARIO_ERROR_SUCCESS) return USB_IMPL_IDX;
    if (impls[PAR_IMPL_IDX].matchPortName(portName) == STARIO_ERROR_SUCCESS) return PAR_IMPL_IDX;
    if (impls[SER_IMPL_IDX].matchPortName(portName) == STARIO_ERROR_SUCCESS) return SER_IMPL_IDX;

    return STARIO_ERROR_NOT_AVAILABLE;
}

long openPort (char const * portName, char const * portSettings)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].openPort == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].openPort(portName, portSettings);
}

long writePort (char const * portName, char const * writeBuffer, long length)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].writePort == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].writePort(portName, writeBuffer, length);
}

long readPort (char const * portName, char * readBuffer, long length)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].readPort == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].readPort(portName, readBuffer, length);
}

long getStarPrinterStatus (const char * portName, StarPrinterStatus * status)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].getStarPrinterStatus == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].getStarPrinterStatus(portName, status);
}

long beginCheckedBlock (char const * portName)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].beginCheckedBlock == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].beginCheckedBlock(portName);
}

long endCheckedBlock (char const * portName, StarPrinterStatus * status)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].endCheckedBlock == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].endCheckedBlock(portName, status);
}

long hdwrResetDevice (char const * portName)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].hdwrResetDevice == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].hdwrResetDevice(portName);
}

long doVisualCardCmd (char const * portName, VisualCardCmd * request, long timeoutMillis)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].doVisualCardCmd == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].doVisualCardCmd(portName, request, timeoutMillis);
}

long closePort (char const * portName)
{
    long supportingImplIdx = getSupportingImplIdx(portName);
    if (supportingImplIdx == STARIO_ERROR_NOT_AVAILABLE)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    if (impls[supportingImplIdx].closePort == 0)
    {
        return STARIO_ERROR_NOT_AVAILABLE;
    }

    return impls[supportingImplIdx].closePort(portName);
}

