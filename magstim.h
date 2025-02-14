#ifndef MAGSTIM_H
#define MAGSTIM_H
#include <stdio.h>
#include <iostream>
#include "rs232.h"
#include <math.h>
#include <iomanip>
#include <sstream>


#include <QString>
#include <queue>
#include <tuple>
#include <list>
#include <map>
#include <QByteArray>
#include "connectionrobot.h"
#include "serialportcontroller.h"
#include <QObject>

class MagStim : public QObject
{
    Q_OBJECT

public:
    MagStim(QString serialConnection);
    //FW: TODO Destruktor!?

    virtual void connect(int &error);
    virtual void disconnect(int &error);
    virtual std::map<QString, std::map<QString, int>> parseMagstimResponse(std::list<int> responseString, QString responseType);
    virtual std::tuple<int, int, int> parseMagstimResponse_version(std::list<int> responseString);
    virtual void remoteControl(bool enable, std::map<QString, std::map<QString, int> > &message, int &error);
    virtual std::map<QString, std::map<QString, int> > getParameters(int &error);
    virtual void setPower(int newPower, bool delay, int &error, QString commandByte);
    virtual std::map<QString, std::map<QString, int> > getTemperature(int &error);
    virtual void poke();
    virtual void arm(bool delay, std::map<QString, std::map<QString, int> > &message, int &error);
    virtual void disarm(std::map<QString, std::map<QString, int> > &message, int &error);
    virtual bool isArmed();
    virtual bool isUnderControl();
    virtual bool isReadyToFire();
    virtual void fire(std::map<QString, std::map<QString, int> > &message, int &error);
    virtual void resetQuickFire();
    virtual void quickFire();

    // ErrorCodes
    const int INVALID_COMMAND_ERR       = 3; // INVALID_COMMAND_ERR: Invalid command sent.
    const int INVALID_DATA_ERR          = 4; // INVALID_DATA_ERR: Invalid data provided.
    const int COMMAND_CONFLICT_ERR      = 5; // COMMAND_CONFLICT_ERR: Command conflicts with current system configuration.
    const int INVALID_CONFIRMATION_ERR  = 6; // INVALID_CONFIRMATION_ERR: Unexpected command confirmation received.
    const int CRC_MISMATCH_ERR          = 7; // CRC_MISMATCH_ERR: Message contents and CRC value do not match.
    const int NO_REMOTE_CONTROL_ERR     = 8; // NO_REMOTE_CONTROL_ERR: You have not established control of the Magstim unit.
    const int PARAMETER_ACQUISTION_ERR  = 9; // PARAMETER_ACQUISTION_ERR: Could not obtain prior parameter settings.
    const int PARAMETER_UPDATE_ERR      = 10;// PARAMETER_UPDATE_ERR: Could not update secondary parameter to accommodate primary parameter change.
    const int PARAMETER_FLOAT_ERR       = 11;// PARAMETER_FLOAT_ERR: A float value is not allowed for this parameter.
    const int PARAMETER_PRECISION_ERR   = 12;// PARAMETER_PRECISION_ERR: Only one decimal placed allowed for this parameter.
    const int PARAMETER_RANGE_ERR       = 13;// PARAMETER_RANGE_ERR: Parameter value is outside the allowed range.
    const int GET_SYSTEM_STATUS_ERR     = 14;// GET_SYSTEM_STATUS_ERR: Cannot call getSystemStatus() until software version has been established.
    const int SYSTEM_STATUS_VERSION_ERR = 15;// SYSTEM_STATUS_VERSION_ERR: Method getSystemStatus() is not compatible with your software version.
    const int SEQUENCE_VALIDATION_ERR   = 16;// SEQUENCE_VALIDATION_ERR: You must call validateSequence() before you can run a rTMS train.
    const int MIN_WAIT_TIME_ERR         = 17;// MIN_WAIT_TIME_ERR: Minimum wait time between trains violated. Call isReadyToFire() to check.
    const int MAX_ON_TIME_ERR           = 18;// MAX_ON_TIME_ERR: Maximum on time exceeded for current train.


protected:
    virtual void setupSerialPort(QString serialConnection);
    virtual int processCommand(QString commandString, QString receiptType, int readBytes, std::map<QString, std::map<QString, int> > &message);
    virtual int processCommand(QString commandString, QString receiptType, int readBytes, std::tuple<int, int, int> &version);
    virtual int processCommand(QString commandString, QString receiptType, int readBytes, std::tuple<int, int, int> &version, std::map<QString, std::map<QString, int> > &message);
    virtual char calcCRC(QByteArray command);

    static int er;
    static std::tuple<int, int, int> ver;
    static std::map<QString, std::map<QString, int>> mes;

    std::queue<std::tuple<QByteArray, QString, int>> sendQueue;
    std::queue<std::tuple<int, QByteArray>> receiveQueue;
    std::queue<float> robotQueue;
    connectionRobot *robot;
    serialPortController *connection;
    // connection.daemon = true; //FW: TODO
    // connectionRobot robot;
    // robot.daemon = true; //FW: TODO
    bool connected;
    std::tuple<QByteArray, QString, int> connectionCommand;
    // auto queryCommand; //FW: TODO
    int parameterReturnByte;

public slots:
    void updateReceiveQueue(std::tuple<int, QByteArray> info);

signals:
    void updateSendQueue(const std::tuple<QByteArray, QString, int> &info);
    void updateRobotQueue(const float &info);
};

#endif // MAGSTIM_H
