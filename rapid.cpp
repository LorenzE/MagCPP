#include "rapid.h"
#include "rapid.h"

#include "yaml/include/yaml-cpp/yaml.h"
#include <QDir>
#include <QFile>

Rapid::Rapid(QString serialConnection, int superRapid, QString unlockCode, int voltage, std::tuple<int, int, int> version) :
    MagStim(serialConnection)
{
    this->setDefault();
    this->super = superRapid;
    this->unlockCode = unlockCode;
    this->voltage = voltage;
    if (serialConnection.toLower() == "virtual") {
        this->version = version; //FW: Is it right?
    } else {
        this->version = std::make_tuple(0,0,0); // FW: Is it right to set to (0,0,0)?
    }
    // TODO UnlockCode
    this->parameterReturnBytes = NAN;
    this->sequenceValidated = false;
    this->repetitiveMode = false;
}

Rapid::Rapid(QString serialConnection) :
    MagStim(serialConnection)
{
    this->setDefault();
    Rapid(serialConnection,this->DEFAULT_RAPID_TYPE,this->DEFAULT_UNLOCK_CODE,this->DEFAULT_VOLTAGE,this->DEFAULT_VIRTUAL_VERSION);
}

float Rapid::getRapidMinWaitTime(int power, int nPulses, float frequency)
{
    float minWaitTime = 0.5;
    float calcWaitTime = (nPulses * ((frequency * this->JOULES[power]) - float(1050.0)));
    if(minWaitTime < calcWaitTime) {
        return calcWaitTime;
    }
    else {
        return minWaitTime;
    }
}

float Rapid::getRapidMaxOnTime(int power, float frequency)
{
    float PulseNum = 63000.0;
    float FreqPow = frequency * this->JOULES[power];
    return PulseNum / FreqPow;
}

float Rapid::getRapidMaxContinuousOperationsFrequency(int power)
{
    float a = 1050.0;
    float b = this->JOULES[power];
    return a / b;
}

std::tuple<int, std::tuple<int,int,int>> Rapid::getVersion()
       /*
       Get Magstim software version number. This is needed when obtaining parameters from the Magstim.
       Returns:
       :tuple:(error,message):
           error (int): error code (0 = no error; 1+ = error)
           message (tuple): if error is 0 (False) returns a tuple containing the version number (in (Major,Minor,Patch) format), otherwise returns an error string
       */
{
    auto e = this->processCommand("ND", "version", 0); // HO: TODO: 0 or NONE?!
    int error= std::get<0>(e);
    std::tuple message = std::get<1>(e); // HO: TODO: how to get the message here?
    std::tuple<int, std::tuple<int,int,int>> answer;

    // If we didn't receive an error, update the version number and the number of bytes that will be returned by a getParameters() command

    if (error == 0) {
        this->version = message;
        if (std::get<0>(this->version) >= 9) {
            this->parameterReturnBytes = 24;
        }
        else if (std::get<0>(this->version) >= 7) {
            this->parameterReturnBytes = 22;
        }
        else {
            this->parameterReturnBytes = 21;
        }
    }
    return answer;
}

void Rapid::connect(bool receipt)
    /*
    Connect to the Rapid.
    This starts the serial port controller, as well as a process that constantly keeps in contact with the Rapid so as not to lose control.
    It also collects the software version number of the Rapid in order to send the correct command for obtaining parameter settings.

    Args:
    receipt (bool): whether to return occurrence of an error and the automated response from the Rapid unit (defaults to False)

    Returns:
      :tuple:(error,message):
      error (int): error code (0 = no error; 1+ = error)
       message (str): if error is 0 (False) returns a string containing the version number (in (X,X,X) format), otherwise returns an error string
    */

{
    MagStim::connect("COM1");// HO: TODO: right port
    int error = std::get<0>(this->getVersion());
    if (error) {
        this->disconnect();
        throw new std::string("Could not determine software version of Rapid. Disconnecting.");
    }
}

void Rapid::disconnect()
{
    /*
    Disconnect from the Magstim.
        This stops maintaining contact with the Magstim and turns the serial port controller off.
    */
    //Just some housekeeping before we call the base magstim class method disconnect
    this->sequenceValidated = false;
    this->repetitiveMode = false;
    return MagStim::disconnect();
}

void Rapid::rTMSMode(bool enable, bool receipt)
{
    /*
    This is a helper function to enable/disable rTMS mode.

    Args:
    enable (bool): whether to enable (True) or disable (False) control
    receipt (bool): whether to return occurrence of an error and the automated response from the Rapid unit (defaults to False)

        Returns:
        If receipt argument is True:
            :tuple:(error,message):
                error (int): error code (0 = no error; 1+ = error)
                message (dict,str): if error is 0 (False) returns a dict containing Rapid instrument status ['instr'] and rMTS setting ['rapid'] dicts, otherwise returns an error string
        If receipt argument is False:
            None This is a helper function to enable/disable rTMS mode.

        Args:
        enable (bool): whether to enable (True) or disable (False) control
        receipt (bool): whether to return occurrence of an error and the automated response from the Rapid unit (defaults to False)

        Returns:
        If receipt argument is True:
            :tuple:(error,message):
                error (int): error code (0 = no error; 1+ = error)
                message (dict,str): if error is 0 (False) returns a dict containing Rapid instrument status ['instr'] and rMTS setting ['rapid'] dicts, otherwise returns an error string
        If receipt argument is False:
            None
    */
    this->sequenceValidated = false;
    // Durations of 1 or 0 are used to toggle repetitive mode on and off
    if (std::get<0>(this->version) >= 9) {
        if (enable) {
            std::string commandString = "0010";
        }
        else {
            std::string commandString = "0000";
            }
    }
    else {
        if (enable) {
            std::string commandString = "010";
        }
        else {
            std::string commandString = "000";
        }
    }
    //this->processCommand()
}





void Rapid::setDefault()
// FW: CPP exclusive for getting default settings in constructor
{
    QString file = QDir::currentPath() + "/rapid_config.yaml";

    if (QFile::exists(file)) {
        YAML::Node config = YAML::LoadFile(file.toStdString());
        this->DEFAULT_RAPID_TYPE = config["defaultRapidType"].as<int>();
        this->DEFAULT_VOLTAGE = config["defaultVoltage"].as<int>();
        this->DEFAULT_UNLOCK_CODE = config["unlockCode"].as<QString>();
        this->ENFORCE_ENERGY_SAFETY = config["enforceEnergySafety"].as<bool>();
        this->DEFAULT_VIRTUAL_VERSION = config["virtualVersionNumber"].as<std::tuple<int, int, int>>();
    } else {
        this->DEFAULT_RAPID_TYPE = 0;
        this->DEFAULT_VOLTAGE = 240;
        this->DEFAULT_UNLOCK_CODE = "";
        this->ENFORCE_ENERGY_SAFETY = true;
        this->DEFAULT_VIRTUAL_VERSION = std::make_tuple(5,0,0);
    }
    file = QDir::currentPath() + "/rapid_system_info.yaml";
    YAML::Node rapid = YAML::LoadFile(file.toStdString());
    this->MAX_FREQUENCY = rapid["maxFrequency"].as<std::map<int, std::map<int, std::map<int, int>>>>();
    this->JOULES = rapid["joules"].as<std::map<int, float>>();
}
