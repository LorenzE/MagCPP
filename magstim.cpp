#include "magstim.h"

#include <functional>
#include <QRegExp>
#include <QStringList>

MagStim::MagStim(QString serialConnection) :
    robot(this->sendQueue, this->robotQueue)
{
    this->connect(serialConnection.toStdString());
    // connection.daemon = true; //FW: TODO
    // robot.daemon = true; //FW: TODO
    this->connected = false;
    // connectionCommand = (b'Q@n', None, 3) //FW: TODO
    // auto queryCommand = std::bind(this->remoteControl, true, true);//FW: TODO
}

std::map<QString, std::map<QString, int> > MagStim::parseMagstimResponse(std::list<int> responseString, QString responseType)
// Interprets response sent from the Magstim unit.
{
    std::map<QString, std::map<QString, int>> magstimResponse;
    if (responseType == "version") {
        // FW: Convert to char except first and last element
        char foo[4]; //FIXME: responseString.size()-2
        std::list<int>::iterator it = responseString.begin();
        it++;
        int i=0;
        do {
            foo[i] = *it;
            it++; i++;
        } while (it != responseString.end());
        // FW: read as number
        QString s = foo;
        QStringList versionList = s.split('.');
        for (int i = 0; i<versionList.length(); i++) {
            bool ok = true;
            int hex = versionList.at(i).toInt(&ok, 10);
            if (!ok)
                versionList[i] = "0";
            std::cout << versionList.at(i).toStdString() << " ";
        }
        // FW: create tuple;
        std::tuple<int, int, int> magstimResponse;
        if (versionList.length() == 3)
            magstimResponse = std::make_tuple(versionList.at(0).toInt(), versionList.at(1).toInt(), versionList.at(2).toInt());
        if (versionList.length() == 2)
            magstimResponse = std::make_tuple(versionList.at(0).toInt(), versionList.at(1).toInt(), 0);
        if (versionList.length() == 1)
            magstimResponse = std::make_tuple(versionList.at(0).toInt(), 0, 0);
        // FW: TODO this magstimResponse is not the correct type for returning!
    } else {
        // Get ASCII code of first data character
        int temp = responseString.front();
        responseString.pop_front();
        std::map<QString, int> instr;
        instr["standby"]        =  temp      & 1;
        instr["armed"]          = (temp >>1) & 1;
        instr["ready"]          = (temp >>2) & 1;
        instr["coilPresent"]    = (temp >>3) & 1;
        instr["replaceCoil"]    = (temp >>4) & 1;
        instr["errorPresent"]   = (temp >>5) & 1;
        instr["errorType"]      = (temp >>6) & 1;
        instr["remoteStatus"]   = (temp >>7) & 1;
        magstimResponse["instr"]= instr;
    }
    // If a Rapid system and response includes rTMS status
    if (responseType == "instrRapid" || responseType == "rapidParam" || responseType=="systemRapid") {
        // Get ASCII code of second data character
        int temp = responseString.front();
        responseString.pop_front();
        // interpret bits
        std::map<QString, int> rapid;
        rapid["enhancedPowerMode"]      =  temp      & 1;
        rapid["train"]                  = (temp >>1) & 1;
        rapid["wait"]                   = (temp >>2) & 1;
        rapid["singlePulseMode"]        = (temp >>3) & 1;
        rapid["hvpsuConnected"]         = (temp >>4) & 1;
        rapid["coilReady"]              = (temp >>5) & 1;
        rapid["thetaPSUDetected"]       = (temp >>6) & 1;
        rapid["modifiedCoilAlgorithm"]  = (temp >>7) & 1;
        magstimResponse["rapid"]= rapid;
    }
    std::string str(responseString.begin(), responseString.end());
    QString rs =  QString::fromUtf8(str.data(), str.size());
    // if requesting parameter settings or coil temperature
    if (responseType == "bistimParam") {
        std::map<QString, int> bistimParam;
        bistimParam["powerA"]   = rs.mid(0,2).toInt();
        bistimParam["powerB"]   = rs.mid(3,5).toInt();
        bistimParam["ppOffset"] = rs.mid(6,8).toInt();
        magstimResponse["bistimParam"]= bistimParam;
    } else if (responseType == "magstimParam") {
        std::map<QString, int> magstimParam;
        magstimParam["power"]   = rs.mid(0,3).toInt();
        magstimResponse["magstimParam"]= magstimParam;
    } else if (responseType == "rapidParam") {
        // This is a bit of a hack to determine which software version we're dealing with
        if (responseString.size() == 20) {
            std::map<QString, int> rapidParam;
            rapidParam["power"]     = rs.mid(0,2).toInt();
            rapidParam["frequency"] = rs.mid(3,6).toInt(); //FW: TODO
            rapidParam["nPulses"]   = rs.mid(7,11).toInt();
            rapidParam["duration"]  = rs.mid(12,15).toInt(); //FW: TODO
            rapidParam["wait"]      = rs.mid(16,19).toInt(); //FW: TODO
            magstimResponse["rapidParam"]= rapidParam;
        } else {
            std::map<QString, int> rapidParam;      // responseString 38 50 57 48 = als chr --> 029
            //FW: TODO      // Power = 29     WICHTIG = 0:3 heißt 0:2
            rapidParam["power"]     = rs.mid(0,2).toInt();
            rapidParam["frequency"] = rs.mid(3,6).toInt(); //FW: TODO
            rapidParam["nPulses"]   = rs.mid(7,10).toInt();
            rapidParam["duration"]  = rs.mid(11,13).toInt(); //FW: TODO
            rapidParam["wait"]      = rs.mid(14,rs.length()-1).toInt(); //FW: TODO
            magstimResponse["rapidParam"]= rapidParam;
        }
    } else if (responseType == "magstimTemp") {
        std::map<QString, int> magstimTemp;
        magstimTemp["coil1Temp"]   = rs.mid(0,2).toInt(); //FW: TODO
        magstimTemp["coil2Temp"]   = rs.mid(2,5).toInt(); //FW: TODO
        magstimResponse["magstimTemp"]= magstimTemp;
    } else if (responseType == "systemRapid") {
        int temp = responseString.front();
        std::string str(responseString.begin(), responseString.end());
        QString rs =  QString::fromUtf8(str.data(), str.size());
        responseString.pop_front();
        std::map<QString, int> extInstr;
        extInstr["plus1ModuleDetected"]         = temp & 1;
        extInstr["specialTriggerModeActive"]    = (temp >> 1) & 1;
        extInstr["chargeDelaySet"]              = (temp >> 2) & 1;
        magstimResponse["extInstr"]= extInstr;
    } else if (responseType == "error") {
        std::map<QString, int> currentErrorCode;
        currentErrorCode["currentErrorCode"]  = rs.mid(0,rs.length()-2).toInt(); //FW: TODO
        magstimResponse["currentErrorCode"]= currentErrorCode;
    } else if (responseType == "instrCharge") {
        std::map<QString, int> chargeDelay;
        chargeDelay["currentErrorCode"]  = rs.toInt();
        magstimResponse["chargeDelay"]= chargeDelay;
    }
    return magstimResponse;
}

bool MagStim::connect(std::string port)
{

    //FIXME: get portnumber from portlist
    bool connected=false;
    cp_num=0;
    connected=RS232_OpenComport(cp_num,bdrate,mode);
    //Check if connection was established
    if(connected){
        return true;
    }
    else {
        return false;
    }
}

void MagStim::disconnect()
{
    RS232_CloseComport(cp_num);
}

bool MagStim::encode_command(uint8_t *destination, uint8_t *data)
{
    //Muss noch angepasst werden
    destination[4]=*data;
        destination[5]=*++data;
        destination[6]=*++data;

        std::stringstream test;
        std::string hexstring;
        test<<std::setbase(16);
        test<<std::uppercase;
        test<<(0xff&(destination[0]+destination[1]+
                destination[2]+destination[3]+
                destination[4]+destination[5]+
                destination[6]+destination[7]+
                destination[8]));
        //std::cout<<test.str();
        hexstring+=test.str();
        destination[9]= hexstring[0];
        destination[10]= hexstring[1];
        return true;
}

bool MagStim::get_status()
{
    RS232_SendBuf(cp_num,stat_command,10);
    return true; //FW FIXME
}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::remoteControl(bool enable, bool receipt)
{
    QString str;
    if (receipt) {
        str = "instr";
    }
    if (enable) {
        return this->processCommand("Q@", str, 3);
    } else {
        return this->processCommand("R@", str, 3);
    }
}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::getParameters()
{
    return this->processCommand("J@", "magstimParam", 12);
}

void MagStim::setPower()
{

}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::getTemperature()
{
    return this->processCommand("F@", "magstimTemp", 9);
}

void MagStim::poke()
{

}

void MagStim::arm()
{

}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::disarm(bool receipt)
{
    QString str;
    if (receipt) {
        str = "instr";
    }
    return this->processCommand("EA", str, 3);
}

void MagStim::isArmed()
{

}

void MagStim::isUnderControl()
{

}

void MagStim::isReadyToFire()
{

}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::fire(bool receipt)
{
    QString str;
    if (receipt) {
        str = "instr";
    }
    return this->processCommand("EH", str, 3);
}

void MagStim::resetQuickFire()
{

}

void MagStim::quickFire()
{

}

void MagStim::setupSerialPort(QString serialConnection)
{

}

std::tuple<int, std::map<QString, std::map<QString, int> > > MagStim::processCommand(QString commandString, QString receiptType, int readBytes)
{
    // TODO: Return Error oder Tuple oder ... Referenz verwenden!?
    // commandString "/@"
    // EB --> 69 66
    QByteArray comString = commandString.toLocal8Bit();
    //TODO check?
    if (this->connected || comString.at(0) == (char)0x81 || comString.at(0) == (char)0x82 || comString.at(0) == (char)0x74 || comString.at(0) == (char)0x70 || comString.contains("EA") || ( comString.at(0) == (char)0x92 && this->parameterReturnByte != 0 )  ) {
        std::tuple<QByteArray,QString, int> test;
        // this->sendQueue.push(test);
        if (receiptType != 0) {
            // error, reply = self.receiveQueue.get()
            int error = 0;
            QByteArray reply = "Test";
            if (error) {
//                return; // TODO
            } else {
                if (reply.at(0) == (char)0x63) {
//                    return; // TODO
                } else if (reply.at(1) == (char)0x63)  {
//                    return; // TODO
                } else if (reply.at(1) == (char)0x83) {
//                    return; // TODO
                } else if (reply.at(0) != comString.at(0)) {
//                    return; // TODO
                } else if (false) { //TODO ord(calcCRC(reply[:-1])) != reply[-1]:
//                    return; // TODO
                }

            }
        }
        if (this->connected) {
            if (comString.at(0) == (char)0x82) {
                this->robotQueue.push(-1);
            } else if (comString.left(2).contains("EA") ) {
                this->robotQueue.push(1);
            } else if (comString.left(2).contains("EB") ) {
                this->robotQueue.push(2);
            }  else {
                this->robotQueue.push(0);
            }
        }
//        return; // TODO
    } else {
//        return; // TODO
    }
    std::map<QString, std::map<QString, int>> first;
    std::tuple<int, std::map<QString, std::map<QString, int> > > vorruebergehend = std::make_tuple(1, first);
    return vorruebergehend;
}

char MagStim::calcCRC(QByteArray command)
//  Return the CRC checksum for the command string.
{
    // Convert command string to sum of ASCII/byte values
    int commandSum = 0;
    for (int i = 1 ; i< command.length() ; i++) {
        commandSum += command.at(i);
    }
    // Convert command sum to binary, then invert and return 8-bit character value
    return (char) (~commandSum & 0xff);
}
