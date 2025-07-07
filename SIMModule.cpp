#include "SIMModule.hpp"

SIMModule::SIMModule(HardwareSerial &serial, uint32_t baud, int8_t rx, int8_t tx)
    : _serial(serial), _baud(baud), _rx(rx), _tx(tx) {}

void SIMModule::begin() {
    if (_rx >= 0 && _tx >= 0) {
        _serial.begin(_baud, SERIAL_8N1, _rx, _tx);
    } else {
        _serial.begin(_baud);
    }
    // delay(3000);
    sendAT("AT");
}

void SIMModule::sendAT(const String &cmd, uint32_t timeout) {
    _serial.println(cmd);
    uint32_t t = millis();
    while (millis() - t < timeout) {
        if (check(EscPress)) break;
        while (_serial.available()) { Serial.write(_serial.read()); }
    }
}

String SIMModule::sendATWithResponse(const String &cmd, uint32_t timeout) {
    _serial.println(cmd);
    String response;
    uint32_t t = millis();
    while (millis() - t < timeout) {
        if (check(EscPress)) break;
        while (_serial.available()) {
            char c = _serial.read();
            response += c;
            Serial.write(c);
        }
    }
    return response;
}
void SIMModule::setCallCallback(CallCallback cb) { _callCb = cb; }
// Call / SMS
void SIMModule::makeCall(const String &number) { sendAT("ATD" + number + ";", 3000); }
void SIMModule::answerCall() { sendAT("ATA", 3000); }
void SIMModule::hangUp() { sendAT("ATH", 3000); }
void SIMModule::sendSMS(const String &number, const String &message) {
    sendAT("AT+CMGF=1");
    _serial.print("AT+CMGS=\"");
    _serial.print(number);
    _serial.println("\"");
    delay(100);
    _serial.print(message);
    _serial.write(26);
    delay(5000);
}
void SIMModule::enableCallerID() { sendAT("AT+CLIP=1"); }
String SIMModule::getSignalStrength() { return sendATWithResponse("AT+CSQ"); }
String SIMModule::getOperator() { return sendATWithResponse("AT+COPS?"); }

// LTE Data
void SIMModule::setupAPN(const String &apn) {
    _setupAPN = true;
    sendAT("AT+CGDCONT=1,\"IP\",\"" + apn + "\"");
}

bool SIMModule::startData() {
    if (!_setupAPN) {
        String apn = "internet"; // Default APN
        // Setup default APN if not set
        sendAT("AT+CGDCONT=1,\"IP\",\"" + apn + "\"");
        _setupAPN = true;
    }
    sendAT("AT+NETOPEN", 3000);
    String resp = sendATWithResponse("", 3000);
    return resp.indexOf("OK") != -1;
}

void SIMModule::stopData() {
    _setupAPN = false;
    sendAT("AT+NETCLOSE", 3000);
}

String SIMModule::getIP() { return sendATWithResponse("AT+IPADDR", 3000); }

String SIMModule::httpGET(const String &url) {
    sendAT("AT+HTTPINIT");
    sendAT("AT+HTTPPARA=\"URL\",\"" + url + "\"");
    sendAT("AT+HTTPACTION=0", 10000);
    String resp = sendATWithResponse("", 10000);
    int code = -1;
    int idx = resp.indexOf("+HTTPACTION:");
    if (idx >= 0) {
        int comma1 = resp.indexOf(',', idx);
        int comma2 = resp.indexOf(',', comma1 + 1);
        if (comma1 >= 0 && comma2 >= 0) { code = resp.substring(comma1 + 1, comma2).toInt(); }
    }
    _httpCode = code;
    return sendATWithResponse("AT+HTTPREAD", 10000);
}
int SIMModule::getHTTPCode() { return _httpCode; }

String SIMModule::httpPOST(const String &url, const String &data, const String &contentType) {
    sendAT("AT+HTTPINIT");
    sendAT("AT+HTTPPARA=\"URL\",\"" + url + "\"");
    sendAT("AT+HTTPPARA=\"CONTENT\",\"" + contentType + "\"");
    sendAT("AT+HTTPDATA=" + String(data.length()) + ",5000");
    delay(100);
    _serial.print(data);
    delay(5000);
    sendAT("AT+HTTPACTION=1", 10000);
    String resp = sendATWithResponse("", 10000);
    int code = -1;
    int idx = resp.indexOf("+HTTPACTION:");
    if (idx >= 0) {
        int comma1 = resp.indexOf(',', idx);
        int comma2 = resp.indexOf(',', comma1 + 1);
        if (comma1 >= 0 && comma2 >= 0) { code = resp.substring(comma1 + 1, comma2).toInt(); }
    }
    _httpCode = code;
    return sendATWithResponse("AT+HTTPREAD", 10000);
}

// USSD / MMI
bool SIMModule::isValidMMI(const String &code) {
    if (code.length() < 3) return false;
    if (code.charAt(0) != '*') return false;
    if (code.charAt(code.length() - 1) != '#') return false;

    for (size_t i = 1; i < code.length() - 1; i++) {
        char c = code.charAt(i);
        if (!isDigit(c) && c != '*' && c != '+') return false;
    }
    return true;
}

String SIMModule::sendUSSD(const String &code) {
    sendAT("AT+CUSD=1,\"" + code + "\",15");
    return sendATWithResponse("", 10000);
}

String SIMModule::sendUSSDChecked(const String &code) {
    if (isValidMMI(code)) {
        Serial.println("Valid MMI code. Sending...");
        return sendUSSD(code);
    } else {
        Serial.println("Invalid MMI code.");
        return "ERROR: Invalid MMI code";
    }
}

void SIMModule::end() {
    _serial.end();
    Serial.println("SIM Module connection closed.");
}

void SIMModule::setSMSCallback(SMSCallback cb) { _smsCb = cb; }

void SIMModule::loop() {
    // if (_serial.available()) {
    while (_serial.available()) {
        char c = _serial.read();
        _serialBuffer += c;
        Serial.write(c); // Echo

        if (c == '\n') {
            _serialBuffer.trim();
            Serial.println("Received: " + _serialBuffer);
            if (_serialBuffer == "RING") {
                // waiting for +CLIP
            } else if (_serialBuffer.startsWith("+CLIP:")) {
                int start = _serialBuffer.indexOf("\"");
                int end = _serialBuffer.indexOf("\"", start + 1);
                String number = _serialBuffer.substring(start + 1, end);
                if (_callCb) _callCb(number);
            } else if (_serialBuffer.startsWith("+CMTI:")) {
                int index = _serialBuffer.substring(_serialBuffer.lastIndexOf(",") + 1).toInt();
                // Read SMS at index
                _serial.println("AT+CMGR=" + String(index));
            } else if (_serialBuffer.startsWith("+CMGR:")) {
                // +CMGR: "REC UNREAD","+1234567890",,"23/06/29,12:34:56+32"
                int start = _serialBuffer.indexOf("\"", 6);
                int end = _serialBuffer.indexOf("\"", start + 1);
                String sender = _serialBuffer.substring(start + 1, end);

                // Get actual message content in next serial line(s)
                String message = "";
                while (_serial.available()) {
                    char mc = _serial.read();
                    if (mc == '\n') break;
                    message += mc;
                }

                if (_smsCb) _smsCb(sender, message);
            }

            _serialBuffer = "";
        }
    }
    // }
}
