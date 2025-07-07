#ifndef SIMMODULE_HPP
#define SIMMODULE_HPP

#include <Arduino.h>
// #include "globals.h"

typedef void (*CallCallback)(const String &callerNumber);
typedef void (*SMSCallback)(const String &sender, const String &message);

class SIMModule {
public:
    SIMModule(HardwareSerial &serial, uint32_t baud = 115200, int8_t rx = -1, int8_t tx = -1);

    void begin();
    void sendAT(const String &cmd, uint32_t timeout = 1000);
    String sendATWithResponse(const String &cmd, uint32_t timeout = 1000);

    // Call / SMS
    void makeCall(const String &number);
    void answerCall();
    void hangUp();
    void sendSMS(const String &number, const String &message);
    void enableCallerID();
    String getSignalStrength();
    String getOperator();
    String getSMSC();
    void setSMSC(const String &smsc);

    // LTE data
    void setupAPN(const String &apn);
    bool startData();
    void stopData();
    String getIP();
    String httpGET(const String &url);
    int getHTTPCode();
    String httpPOST(const String &url, const String &data, const String &contentType = "application/x-www-form-urlencoded");

    // USSD / MMI
    bool isValidMMI(const String &code);
    String sendUSSD(const String &code);
    String sendUSSDChecked(const String &code);

    // Utility
    void setCallCallback(CallCallback cb);
    void setSMSCallback(SMSCallback cb);

    void loop();
    void end();

private:
    HardwareSerial &_serial;
    uint32_t _baud;
    int8_t _rx, _tx;
    bool _setupAPN = false;
    int _httpCode;

    CallCallback _callCb = nullptr;
    SMSCallback _smsCb = nullptr;
    String _serialBuffer = "";
};

#endif
