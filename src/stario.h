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

/*
    The set of functions exposed by this library are prototyped here.
    Include this header file in your application source and link
    against the stario library (-lstario) to make use of these functions.
*/

#ifndef _included_stario
#define _included_stario

// other stario header files which are required
// included only this file - stario.h - in your source
#include "stario-error.h"
#include "stario-structures.h"

#ifdef __cplusplus
extern "C" {
#endif

// general api - both printers and Visual Card

/*
    openPort
    --------
    This function opens a connection to the port specified.

    Parameters: portName - string of the form "usb:TSP700;sn:12345678", or "/dev/ttyS0", or "/dev/parport0" (usb, serial, and parallel respectively)
                portSettings - string of the form "", or "9600,none,8,1,hdwr", or "" (respective to portName parameter)
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not present, wrong serial number, libusb failure
    Notes:      In the case of USB, the portName parameter can optionally contain
                a serial number.  If a serial number is specified, this function
                will succeed only when the specified device type configured
                with the specified serial number is present on a USB bus.

                In the case of serial, the portSettings string contains the
                following fields:
                    baud: 38400, 19200, 9600, 4800, 2400
                    parity: none, even, odd
                    data-bits: 8, 7
                    stop-bits: 1
                    flow-ctrl: none, hdwr
*/
long openPort (char const * portName, char const * portSettings);

/*
    closePort
    ---------
    This function closes the device connection - no further communications
    are possible via this connection.  Call openPort to re-establish a
    connection

    Parameters: portName - string of the form "usb:TSP700", or ...
    Returns:    STARIO_ERROR_SUCCESS
*/
long closePort (char const * portName);




// printer api

/*
    writePort
    ---------
    This function writes the provided buffer length out to the usb device.
    If the device fails to accept data during the write sequence for
    longer then a fixed timeout period, then this function times out.

    Parameters: portName - string of the form "usb:TSP700", or ...
                writeBuffer - pointer to a char array
                length - length in bytes of writeBuffer
    Returns:    number of bytes written successfully >= 0
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
*/
long writePort (char const * portName, char const * writeBuffer, long length);

/*
    readPort
    --------
    This function the requested (or fewer) number of bytes
    in from the device, placing them in the provided buffer.

    Parameters: portName - string of the form "usb:TSP700", or ...
                readBuffer - pointer to a char array
                length - read request length in bytes
    Returns:    number of bytes read successfully >= 0
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
*/
long readPort (char const * portName, char * readBuffer, long length);

/*
    getStarPrinterStatus
    --------------------
    This function reads in the printers current status and
    parses that status into the provided StarPrinterStatus structure.

    Parameters: portName - string of the form "usb:TSP700", or ...
                status - pointer to a StarPrinterStatus structure
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
*/
long getStarPrinterStatus (const char * portName, StarPrinterStatus * status);

/*
    beginCheckedBlock
    -----------------
    This function begins a checked block by caching the printer's
    current status for use later during processing of the
    endCheckedBlock function.

    Parameters: portName - string of the form "usb:TSP700", or ...
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
                STARIO_ERROR_NOT_AVAILABLE - printer unable to support checked
                                             block functionality (no ETB counter)
*/
long beginCheckedBlock (char const * portName);

/*
    endCheckedBlock
    ---------------
    This function ends a checked block by:
    1. Outputting a single ETB byte to the device
    2. Reading device status in a loop until
        a. Device goes offline - failure
        b. device ETB counter increments - success
    If ETB counter increments successfully, then this function completes with success.
    If the device goes offline, then this function hardware resets that device and returns
    status information given the failure locale.

    Parameters: portName - string of the form "usb:TSP700", or ...
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
                STARIO_ERROR_NOT_AVAILABLE - printer unable to support checked
                                             block functionality (no ETB counter)
*/
long endCheckedBlock (char const * portName, StarPrinterStatus * status);

/*
    hdwrResetDevice
    ---------------
    This function hardware resets the device.

    Parameters: portName - string of the form "usb:TSP700", or ...
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
*/
long hdwrResetDevice (char const * portName);




// visual card api

/*
    doVisualCardCmd
    ---------------
    This function executes the specified visual card command, and returns the
    devices response.  If the device does not respond, this function timesout
    and fails.  Prior to executing the specified command, any device response
    buffered is cleared.

    Parameters: portName - string of the form "usb:TSP700", or ...
                request - pointer to a VisualCardCmd structure populated with command and txData
    Returns:    STARIO_ERROR_SUCCESS
                    or
    Errors:     STARIO_ERROR_NOT_OPEN - device not opened or no longer present
                STARIO_ERROR_IO_FAIL - communications problem
                STARIO_ERROR_DLE - the specified command + data was rejected by the device
                STARIO_ERROR_NAK - command & data packet transmission / receive problem
                STARIO_ERROR_NO_RESPONSE - device did not respond to command - timeout
*/
long doVisualCardCmd (char const * portName, VisualCardCmd * request, long timeoutMillis);

#ifdef __cplusplus
}
#endif

#endif

