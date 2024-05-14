/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

#include <stdio.h>
#include <stario/stario.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qbitmap.h>
#include <qimage.h>
#include <qcolor.h>

void ProcessErrorResult(StarIOTest * me, QString fn, long error)
{
    QString msg;

    switch (error)
    {
    case STARIO_ERROR_NOT_OPEN:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_NOT_OPEN";
        me->pushButton_OpenPort->setText("Open Port");
        break;
    case STARIO_ERROR_IO_FAIL:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_IO_FAIL";
        break;
    case STARIO_ERROR_NOT_AVAILABLE:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_NOT_AVAILABLE";
        break;
    case STARIO_ERROR_DLE:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_DLE";
        break;
    case STARIO_ERROR_NAK:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_NAK";
        break;
    case STARIO_ERROR_NO_RESPONSE:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_NO_RESPONSE";
        break;
    case STARIO_ERROR_RUNTIME:
        msg = "Failure during call to " + fn + ": STARIO_ERROR_RUNTIME";
        break;
    }

    QMessageBox::warning(me, "StarIOTest", msg, QMessageBox::Ok, QMessageBox::NoButton);
}

int ShowMsg(StarIOTest * me, QString msg, int button0 = QMessageBox::Ok, int button1 = QMessageBox::NoButton)
{
    return QMessageBox::warning(me, "StarIOTest", msg, button0, button1);
}

void DisplayStatus(StarIOTest * me, StarPrinterStatus status)
{
    me->checkBox_Status_CoverOpen->setChecked(status.coverOpen);
    me->checkBox_Status_Offline->setChecked(status.offline);
    me->checkBox_Status_CompulsionSwitch->setChecked(status.compulsionSwitch);
    me->checkBox_Status_OverTemp->setChecked(status.overTemp);
    me->checkBox_Status_UnrecoverableError->setChecked(status.unrecoverableError);
    me->checkBox_Status_CutterError->setChecked(status.cutterError);
    me->checkBox_Status_MechError->setChecked(status.mechError);
    me->checkBox_Status_PageModeCmdError->setChecked(status.pageModeCmdError);
    me->checkBox_Status_PaperSizeError->setChecked(status.paperSizeError);
    me->checkBox_Status_PresenterPaperJamError->setChecked(status.presenterPaperJamError);
    me->checkBox_Status_HeadUpError->setChecked(status.headUpError);
    me->checkBox_Status_BlackMarkDetectStatus->setChecked(status.blackMarkDetectStatus);
    me->checkBox_Status_PaperEmpty->setChecked(status.paperEmpty);
    me->checkBox_Status_PaperNearEmptyInner->setChecked(status.paperNearEmptyInner);
    me->checkBox_Status_PaperNearEmptyOuter->setChecked(status.paperNearEmptyOuter);
    me->checkBox_Status_StackerFull->setChecked(status.stackerFull);
    me->checkBox_Status_ETBAvailable->setChecked(status.etbAvailable);
    me->textLabel_Status_ETBCounter->setText(QString("%1").arg(status.etbCounter));
    me->textLabel_Status_PresenterState->setText(QString("%1").arg(status.presenterState));
}

void StarIOTest::pushButton_WritePort_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        ShowMsg(this, "Misuse: Please open the port first.");
        return;
    }

    QString writeData = "";

    for (int i = 0; i < 20; i++)
    {
        writeData += "libstario - writePort test output\n";
    }
    writeData += "\x1b""d3";

    long res = STARIO_ERROR_SUCCESS;

    if (checkBox_DoBlockChecking->isChecked())
    {
        res = beginCheckedBlock(textEdit_PortName->text().ascii());

        if (res != STARIO_ERROR_SUCCESS)
        {
            ProcessErrorResult(this, "beginCheckedBlock", res);
            return;
        }
    }

    res = writePort(textEdit_PortName->text().ascii(), writeData.ascii(), (long) writeData.length());

    if (res < STARIO_ERROR_SUCCESS)
    {
        ProcessErrorResult(this, "writePort", res);
    }

    if (res < (long) writeData.length())
    {
        if (ShowMsg(this, "Could not send all data.\nExecute hardware reset?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            pushButton_HdwrReset_clicked();
        }

        return;
    }

    if (checkBox_DoBlockChecking->isChecked())
    {
        StarPrinterStatus status;

        res = endCheckedBlock(textEdit_PortName->text().ascii(), &status);

        if (res != STARIO_ERROR_SUCCESS)
        {
            ProcessErrorResult(this, "endCheckedBlock", res);

            return;
        }

        DisplayStatus(this, status);

        if (status.offline)
        {
            ShowMsg(this, "Checked block failed - see device status for reason.");
        }
        else
        {
            ShowMsg(this, "Checked block printed successfully.");
        }

        return;
    }


    ShowMsg(this, "WritePort completed successfully.");
}


void StarIOTest::pushButton_GetStatus_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        ShowMsg(this, "Misuse: Please open the port first.");
        return;
    }

    StarPrinterStatus status;

    long res = getStarPrinterStatus(textEdit_PortName->text().ascii(), &status);

    if (res != STARIO_ERROR_SUCCESS)
    {
        ProcessErrorResult(this, "getStarPrinterStatus", res);
        return;
    }

    DisplayStatus(this, status);

    ShowMsg(this, "Got Star Printer Status");
}


void StarIOTest::pushButton_HdwrReset_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        ShowMsg(this, "Misuse: Please open the port first.");
        return;
    }

    long res = hdwrResetDevice(textEdit_PortName->text().ascii());

    if (res != STARIO_ERROR_SUCCESS)
    {
        ProcessErrorResult(this, "hdwrResetDevice", res);
        return;
    }

    ShowMsg(this, "Hardware reset succeeded.");
}

void StarIOTest::pushButton_VCRequest_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        ShowMsg(this, "Misuse: Please open the port first.");
        return;
    }

    bool hexCommand = true;

    VisualCardCmd command;

    command.command = (char) textEdit_VCRequest_Command->text().toLong(&hexCommand, 16);
    if (hexCommand == false)
    {
        ShowMsg(this, "Misuse: Command must be 2 digit hex value.");
        return;
    }

    QString txData = textEdit_VCRequest_Data->text();
    command.txData = txData.ascii();
    command.txDataLength = textEdit_VCRequest_Data->length();

    long res = doVisualCardCmd(textEdit_PortName->text().ascii(), &command, 10 * 1000);

    if (res != STARIO_ERROR_SUCCESS)
    {
        textLabel_VCResponse_Command->setText("Dynamic");
        textLabel_VCResponse_Status->setText("Dynamic");
        textLabel_VCResponse_Data->setText("Dynamic");

        ProcessErrorResult(this, "doVisualCardCmd", res);
        return;
    }

    QString responseCmd;
    responseCmd.sprintf("%x", ((int) command.command) & 0xff);
    textLabel_VCResponse_Command->setText(responseCmd);

    char const * statusMsg = "Normal";

    switch (command.status)
    {
        case 0x22: statusMsg = "22h - No target card";                break;
        case 0x23: statusMsg = "23h - No magnetic stripe";            break;
        case 0x31: statusMsg = "31h - Parity Error";                  break;
        case 0x32: statusMsg = "32h - No start code / end code";      break;
        case 0x33: statusMsg = "33h - LRC error";                     break;
        case 0x34: statusMsg = "34h - Erroneous character";           break;
        case 0x37: statusMsg = "37h - Magnetic stripe writing error"; break;
        case 0x38: statusMsg = "38h - Card jam";                      break;
        case 0x40: statusMsg = "40h - Cover open";                    break;
        case 0x41: statusMsg = "41h - Invalid command";               break;
        case 0x42: statusMsg = "42h - Can motor error";               break;
        case 0x43: statusMsg = "43h - Erase head temperature error";  break;
        case 0x45: statusMsg = "45h - EEPROM error";                  break;
        case 0x4C: statusMsg = "4Ch - Non-compatible BMP file data";  break;
        case 0x51: statusMsg = "51h - Expand buffer overflow";        break;
    }

    textLabel_VCResponse_Status->setText(statusMsg);

    char data[129];
    memcpy(data, command.rxData, command.rxDataLength);
    data[command.rxDataLength] = 0x00;

    textLabel_VCResponse_Data->setText(data);

    ShowMsg(this, "doVisualCardCmd succeeded.");
}


void StarIOTest::pushButton_OpenPort_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        long res = openPort(textEdit_PortName->text().ascii(), textEdit_PortSettings->text().ascii());

        if (res != STARIO_ERROR_SUCCESS)
        {
            ProcessErrorResult(this, "openPort", res);
            return;
        }

        pushButton_OpenPort->setText("Close Port");
    }
    else
    {
        long res = closePort(textEdit_PortName->text().ascii());

        if (res != STARIO_ERROR_SUCCESS)
        {
            ProcessErrorResult(this, "closePort", res);
        }

        pushButton_OpenPort->setText("Open Port");
    }
}


void StarIOTest::pushButton_PrintImage_clicked()
{
    if (pushButton_OpenPort->text() == "Open Port")
    {
        ShowMsg(this, "Misuse: Please open the port first.");
        return;
    }

    QBitmap bitmap;

    if (bitmap.load(textEdit_ImageFile->text()) == false)
    {
        ProcessErrorResult(this, "QBitmap", STARIO_ERROR_RUNTIME);

        return;
    }

    QImage image = bitmap.convertToImage();

    int height = image.height();
    int width = image.width();

    if ((height <= 0) || (width <= 0))
    {
        ProcessErrorResult(this, "QImage", STARIO_ERROR_RUNTIME);

        return;
    }

    int cmdLength = 0;
    if (width % 8)
    {
        cmdLength = (4 + 4 + 6 + 6 + (3 + width / 8 + 1) * height + 4) + 3;
    }
    else
    {
        cmdLength = (4 + 4 + 6 + 6 + (3 + width / 8) * height + 4) + 3;
    }

    unsigned char * cmd = new unsigned char [cmdLength];
    if (cmd == NULL)
    {
        ProcessErrorResult(this, "new []", STARIO_ERROR_RUNTIME);

        return;
    }

    memset(cmd, 0x00, cmdLength);

    int cmdIdx = 0;

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = '*';
    cmd [ cmdIdx++ ] = 'r';
    cmd [ cmdIdx++ ] = 'R';

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = '*';
    cmd [ cmdIdx++ ] = 'r';
    cmd [ cmdIdx++ ] = 'A';

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = '*';
    cmd [ cmdIdx++ ] = 'r';
    cmd [ cmdIdx++ ] = 'E';
    cmd [ cmdIdx++ ] = '1';
    cmd [ cmdIdx++ ] = 0x00;

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = '*';
    cmd [ cmdIdx++ ] = 'r';
    cmd [ cmdIdx++ ] = 'P';
    cmd [ cmdIdx++ ] = '0';
    cmd [ cmdIdx++ ] = 0x00;

    for	(int y = 0; y < height; y++)
    {
        if (width % 8)
        {
            cmd [ cmdIdx++ ] = 'b';
            cmd [ cmdIdx++ ] = (unsigned char) ((width / 8 + 1) % 0x0100);
            cmd [ cmdIdx++ ] = (unsigned char) ((width / 8 + 1) / 0x0100);
        }
        else
        {
            cmd [ cmdIdx++ ] = 'b';
            cmd [ cmdIdx++ ] = (unsigned char) ((width / 8) % 0x0100);
            cmd [ cmdIdx++ ] = (unsigned char) ((width / 8) / 0x0100);
        }

        for	(int x = 0; x < width; x += 8, cmdIdx++)
        {
            switch (width -	x)
            {
                default:    if (qGray(image.pixel(x + 7, y)) == 0) cmd [ cmdIdx ] |= 0x01;
                case 7:     if (qGray(image.pixel(x + 6, y)) == 0) cmd [ cmdIdx ] |= 0x02;
                case 6:     if (qGray(image.pixel(x + 5, y)) == 0) cmd [ cmdIdx ] |= 0x04;
                case 5:     if (qGray(image.pixel(x + 4, y)) == 0) cmd [ cmdIdx ] |= 0x08;
                case 4:     if (qGray(image.pixel(x + 3, y)) == 0) cmd [ cmdIdx ] |= 0x10;
                case 3:     if (qGray(image.pixel(x + 2, y)) == 0) cmd [ cmdIdx ] |= 0x20;
                case 2:     if (qGray(image.pixel(x + 1, y)) == 0) cmd [ cmdIdx ] |= 0x40;
                case 1:     if (qGray(image.pixel(x + 0, y)) == 0) cmd [ cmdIdx ] |= 0x80;
            }
        }
    }

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = '*';
    cmd [ cmdIdx++ ] = 'r';
    cmd [ cmdIdx++ ] = 'B';

    cmd [ cmdIdx++ ] = 0x1b;
    cmd [ cmdIdx++ ] = 'd';
    cmd [ cmdIdx++ ] = '3';

    long res = STARIO_ERROR_SUCCESS;

    if (checkBox_DoBlockChecking->isChecked())
    {
        res = beginCheckedBlock(textEdit_PortName->text().ascii());

        if (res != STARIO_ERROR_SUCCESS)
        {
            delete [] cmd;

            ProcessErrorResult(this, "beginCheckedBlock", res);
            return;
        }
    }

    res = writePort(textEdit_PortName->text().ascii(), (char const *) cmd, (long) cmdLength);

    delete [] cmd;

    if (res < STARIO_ERROR_SUCCESS)
    {
        ProcessErrorResult(this, "writePort", res);
    }

    if (res < (long) cmdLength)
    {
        if (ShowMsg(this, "Could not send all data.\nExecute hardware reset?", QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
        {
            pushButton_HdwrReset_clicked();
        }

        return;
    }

    if (checkBox_DoBlockChecking->isChecked())
    {
        StarPrinterStatus status;

        res = endCheckedBlock(textEdit_PortName->text().ascii(), &status);

        if (res != STARIO_ERROR_SUCCESS)
        {
            ProcessErrorResult(this, "endCheckedBlock", res);

            return;
        }

        DisplayStatus(this, status);

        if (status.offline)
        {
            ShowMsg(this, "Checked block failed - see device status for reason.");
        }
        else
        {
            ShowMsg(this, "Checked block printed successfully.");
        }

        return;
    }

    ShowMsg(this, "PrintImage completed successfully.");

}


void StarIOTest::pushButton_BrowseImageFile_clicked()
{
    QString s = QFileDialog::getOpenFileName(
                    "/home",
                    "Images (*.png *.xpm *.jpg *.bmp)",
                    this,
                    "open file dialog"
                    "Choose a file" );

    textEdit_ImageFile->setText(s);
}


void StarIOTest::pushButton_ReadROMVer_clicked()
{
    long res = STARIO_ERROR_SUCCESS;

    do
    {
        char firmVerReq[] = {0x1b, '#', '*', 0x0a, 0x00};
        if ((res = writePort(textEdit_PortName->text().ascii(), firmVerReq, sizeof(firmVerReq))) < (long) sizeof(firmVerReq)) break;

        char inBuf[256];
        memset(inBuf, 0x00, sizeof(inBuf));

        do
        {
            if ((res = readPort(textEdit_PortName->text().ascii(), inBuf, sizeof(inBuf))) < STARIO_ERROR_SUCCESS) break;
        } while (memchr(inBuf, 0x1b, sizeof(inBuf)) == NULL);

        if (res < STARIO_ERROR_SUCCESS) break;

        int i = (char *) memchr(inBuf, 0x1b, sizeof(inBuf)) - inBuf;
        int state = 0; // 0 - init, 1 - rom name str, 2 - terminate 1, 3 - terminate 2
        QString firmName;

        do
        {
            if (i >= res)
            {
                if ((res = readPort(textEdit_PortName->text().ascii(), inBuf, sizeof(inBuf))) < STARIO_ERROR_SUCCESS) break;

                if (res == 0) break;

                i = 0;
            }

            if (state == 0)
            {
                if      (inBuf[i] == 0x1b) state = 0;
                else if (inBuf[i] ==  '#') state = 0;
                else if (inBuf[i] ==  '*') state = 0;
                else if (inBuf[i] ==  ',') state = 0;
                else                       {state = 1; continue;}
            }
            else if (state == 1)
            {
                if (inBuf[i] == 0x0a) state = 2;
                else firmName += inBuf[i];
            }
            else if (state == 2)
            {
                if (inBuf[i] == 0x00) state = 3;
                else break;
            }

            i++;

        } while (state != 3);

        if (state == 3)
        {
            QString msg;

            msg += "ROM Version = ";
            msg += firmName;

            ShowMsg(this, msg);

            return;
        }

    } while (0);

    if (res < STARIO_ERROR_SUCCESS)
    {
        ProcessErrorResult(this, "read rom version", res);

        return;
    }

    ShowMsg(this, "Could not read rom version");
}


