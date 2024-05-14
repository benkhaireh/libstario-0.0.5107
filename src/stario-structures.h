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

#ifndef _included_stario_structures
#define _included_stario_structures

// StarPrinterStatus - structure
// -----------------
//
// This structure contains a single field for each status member
// available from the various Star printer products.
typedef struct
{
    unsigned char coverOpen;                // 1 -> cover open, 0 -> not
    unsigned char offline;                  // 1 -> offline, 0 -> not
    unsigned char compulsionSwitch;         // 1 -> high, 0 -> not
    unsigned char overTemp;                 // 1 -> over temp, 0 -> not
    unsigned char unrecoverableError;       // 1 -> unrecoverable error, 0 -> not
    unsigned char cutterError;              // 1 -> cutter error, 0 -> not
    unsigned char mechError;                // 1 -> mechanical error, 0 -> not
    unsigned char pageModeCmdError;         // 1 -> page mode command error, 0 -> not
    unsigned char paperSizeError;           // 1 -> paper size error, 0 -> not
    unsigned char presenterPaperJamError;   // 1 -> presenter paper jam error, 0 -> not
    unsigned char headUpError;              // 1 -> head up error, 0 -> not
    unsigned char blackMarkDetectStatus;    // 1 -> black mark detect, 0 -> not
    unsigned char paperEmpty;               // 1 -> paper empty, 0 -> not
    unsigned char paperNearEmptyInner;      // 1 -> paper near empty inner, 0 -> not
    unsigned char paperNearEmptyOuter;      // 1 -> paper near empty outer, 0 -> not
    unsigned char stackerFull;              // 1 -> stacker full, 0 -> not
    unsigned char etbAvailable;             // 1 -> etb counter available, 0 -> not
    unsigned char etbCounter;               // 0 ~ 31 integer value
    unsigned char presenterState;           // 0 ~ 7 integer value
    unsigned char rawLength;                // 0 ~ 63 integer value
    unsigned char raw[63];                  // binary status according to Star ASB specification
} StarPrinterStatus;

// VisualCardCmd - structure
// -------------
//
// This structure encapsulates a Visual Card command request.
// The application populates the command, txData, and txDataLength fields,
// executes the command via the doVisualCardCmd function, and then gets the
// device response in the status, rxData, and rxDataLength fields.
typedef struct
{
    char command;           // application provided command index to execute

    char const * txData;    // data to transmit within the command packet
    long txDataLength;      // length of data provided in bytes

    char status;            // device status from response
    char rxData[128];       // data from response
    char rxDataLength;      // length of data from response in bytes
} VisualCardCmd;

#endif
