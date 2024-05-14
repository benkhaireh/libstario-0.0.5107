// teststario.c

// this program demonstrates usage of the libstario API (see /usr/include/stario/stario.h for documentation)

// usage: teststario "portName" "portSettings" "action"
//                    |          |              |
//                    |          |              |--> one of "writeport", "checkedblock", "getstatus", "reset", "printcard"
//                    |          |
//                    |          |--> for serial port "38400,none,8,1,hdwr", parallel & usb ""
//                    |
//                    /--> one of "/dev/ttyS0", "/dev/parport0", "usb:TSP700"

// to compile this file, execute the following command
// gcc -Wall -o teststario teststario.c -lstario

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

// the following preprocessor block is required for RPM packaging - ignore it
#ifdef LOCALSTARIOH
#include "stario.h"
#else

// include the following statement in your application code
#include <stario/stario.h>

#endif

long dispRes(char const * fn, long result)
{
    if (result > STARIO_ERROR_SUCCESS)
    {
        printf("Call to %s transfered %d bytes\n", fn, (int) result);
    }
    else
    {
        switch (result)
        {
            case STARIO_ERROR_SUCCESS:          printf("Call to %s returned %s\n", fn, "STARIO_ERROR_SUCCESS");         break;
            case STARIO_ERROR_NOT_OPEN:         printf("Call to %s returned %s\n", fn, "STARIO_ERROR_NOT_OPEN");        break;
            case STARIO_ERROR_IO_FAIL:          printf("Call to %s returned %s\n", fn, "STARIO_ERROR_IO_FAIL");         break;
            case STARIO_ERROR_NOT_AVAILABLE:    printf("Call to %s returned %s\n", fn, "STARIO_ERROR_NOT_AVAILABLE");   break;
            case STARIO_ERROR_DLE:              printf("Call to %s returned %s\n", fn, "STARIO_ERROR_DLE");             break;
            case STARIO_ERROR_NAK:              printf("Call to %s returned %s\n", fn, "STARIO_ERROR_NAK");             break;
            case STARIO_ERROR_NO_RESPONSE:      printf("Call to %s returned %s\n", fn, "STARIO_ERROR_NO_RESPONSE");     break;
            case STARIO_ERROR_RUNTIME:          printf("Call to %s returned %s\n", fn, "STARIO_ERROR_RUNTIME");         break;
        }
    }
    
    return result;
}

void dispStatus(StarPrinterStatus status)
{
    printf("Displaying status...\n");
    printf("\tstatus.coverOpen = %d\n",                 status.coverOpen);
    printf("\tstatus.offline = %d\n",                   status.offline);
    printf("\tstatus.compulsionSwitch = %d\n",          status.compulsionSwitch);
    printf("\tstatus.overTemp = %d\n",                  status.overTemp);
    printf("\tstatus.unrecoverableError = %d\n",        status.unrecoverableError);
    printf("\tstatus.cutterError = %d\n",               status.cutterError);
    printf("\tstatus.mechError = %d\n",                 status.mechError);
    printf("\tstatus.pageModeCmdError = %d\n",          status.pageModeCmdError);
    printf("\tstatus.paperSizeError = %d\n",            status.paperSizeError);
    printf("\tstatus.presenterPaperJamError = %d\n",    status.presenterPaperJamError);
    printf("\tstatus.headUpError = %d\n",               status.headUpError);
    printf("\tstatus.blackMarkDetectStatus = %d\n",     status.blackMarkDetectStatus);
    printf("\tstatus.paperEmpty = %d\n",                status.paperEmpty);
    printf("\tstatus.paperNearEmptyInner = %d\n",       status.paperNearEmptyInner);
    printf("\tstatus.paperNearEmptyOuter = %d\n",       status.paperNearEmptyOuter);
    printf("\tstatus.stackerFull = %d\n",               status.stackerFull);
    printf("\tstatus.etbAvailable = %d\n",              status.etbAvailable);
    printf("\tstatus.etbCounter = %d\n",                status.etbCounter);
    printf("\tstatus.presenterState = %d\n",            status.presenterState);
}

int main(int argc, char ** argv)
{
    long res = STARIO_ERROR_SUCCESS;
    enum action {writeport, checkedblock, getstatus, reset, printcard};
    enum action reqAction = writeport;
    struct timeval sleepTime;
    StarPrinterStatus status;
    VisualCardCmd cmd;
    int i = 0;
    
    if (argc < 4)
    {
        printf("usage: libstario-test \"portName\" \"portSettings\" \"action\"\n");
        printf("\tportName is one of \"/dev/ttyS0\", \"/dev/parport0\", \"usb:TSP700\"\n");
        printf("\tportSettings is one of \"38400,none,8,1,hdwr\", \"\", \"\"\n");
        printf("\taction is one of \"writeport\", \"checkedblock\", \"getstatus\", \"reset\", \"printcard\"\n");
        
        return 1;
    }
    
    if (strcmp(argv[3], "writeport") == 0)
    {
        reqAction = writeport;
    }
    else if (strcmp(argv[3], "checkedblock") == 0)
    {
        reqAction = checkedblock;
    }
    else if (strcmp(argv[3], "getstatus") == 0)
    {
        reqAction = getstatus;
    }
    else if (strcmp(argv[3], "reset") == 0)
    {
        reqAction = reset;
    }
    else if (strcmp(argv[3], "printcard") == 0)
    {
        reqAction = printcard;
    }
    else
    {
        printf("misuse: action is one of \"writeport\", \"checkedblock\", \"getstatus\", \"reset\", \"printcard\"\n");
        
        return 1;
    }
    
    do
    {
        res = openPort(argv[1], argv[2]);
        dispRes("openPort", res);
        if (res != STARIO_ERROR_SUCCESS) break;
        
        if ((strncmp(argv[1], "/dev/ttyS", 9) == 0) && (reqAction != printcard))
        {
            sleepTime.tv_sec = 2;
            sleepTime.tv_usec = 0;
    
            select(0, NULL, NULL, NULL, &sleepTime);
        }
        
        if (reqAction == writeport)
        {
            for (i = 0; i < 10; i++)
            {
                res = writePort(argv[1], "libstario - writeport\n", sizeof("libstario - writeport\n") - 1);
                dispRes("writePort", res);
                if (res != sizeof("libstario - writeport\n") - 1) break;
            }
            if (res < STARIO_ERROR_SUCCESS) break;
            
            res = writePort(argv[1], "\x1b""d3", sizeof("\x1b""d3") - 1);
            dispRes("writePort", res);
            if (res != sizeof("\x1b""d3") - 1) break;
        }
        else if (reqAction == checkedblock)
        {
            res = beginCheckedBlock(argv[1]);
            dispRes("beginCheckedBlock", res);
            if (res != STARIO_ERROR_SUCCESS) break;
            
            for (i = 0; i < 10; i++)
            {
                res = writePort(argv[1], "libstario - writeport\n", sizeof("libstario - writeport\n") - 1);
                dispRes("writePort", res);
                if (res != sizeof("libstario - writeport\n") - 1) break;
            }
            if (res < STARIO_ERROR_SUCCESS) break;
            
            res = writePort(argv[1], "\x1b""d3", sizeof("\x1b""d3") - 1);
            dispRes("writePort", res);
            if (res != sizeof("\x1b""d3") - 1) break;
            
            res = endCheckedBlock(argv[1], &status);
            dispRes("endCheckedBlock", res);
            if (res != STARIO_ERROR_SUCCESS) break;
            
            if (status.offline == 0)
            {
                printf("Checked block printed successfully.\n");
            }
            else
            {
                printf("Checked block printing failed.\n");
                dispStatus(status);
            }
        }
        else if (reqAction == getstatus)
        {
            res = getStarPrinterStatus(argv[1], &status);
            dispRes("getStarPrinterStatus", res);
            if (res != STARIO_ERROR_SUCCESS) break;
            
            dispStatus(status);
        }
        else if (reqAction == reset)
        {
            res = hdwrResetDevice(argv[1]);
            dispRes("hdwrResetDevice", res);
            if (res != STARIO_ERROR_SUCCESS) break;
        }
        else if (reqAction == printcard)
        {
            memset(&cmd, 0x00, sizeof(VisualCardCmd));
            cmd.command = 0x41;
            cmd.txData = "libstario & Visual Card";
            cmd.txDataLength = sizeof("libstario & Visual Card") - 1;
            
            res = doVisualCardCmd(argv[1], &cmd, 10 * 1000);
            dispRes("doVisualCardCmd", res);
            if (res != STARIO_ERROR_SUCCESS) break;
            
            memset(&cmd, 0x00, sizeof(VisualCardCmd));
            cmd.command = 0x46;
            cmd.txData = "1";
            cmd.txDataLength = sizeof("1") - 1;
            
            res = doVisualCardCmd(argv[1], &cmd, 10 * 1000);
            dispRes("doVisualCardCmd", res);
            if (res != STARIO_ERROR_SUCCESS) break;
        }
        
        if ((strncmp(argv[1], "/dev/ttyS", 9) == 0) && (reqAction != printcard))
        {
            sleepTime.tv_sec = 2;
            sleepTime.tv_usec = 0;
    
            select(0, NULL, NULL, NULL, &sleepTime);
        }
        
    } while (0);
    
    res = closePort(argv[1]);
    dispRes("closePort", res);
    
    printf("Finished\n");

    return 0;
}
