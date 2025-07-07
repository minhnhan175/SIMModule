# SIMModule
A convenient library for SIM modules.

SIM modules require a lot of knowledge of AT commands, such as `AT+CMGS`, `AT+NETOPEN`, `AT+NETCLOSE`, etc.
That's why I made this library for convenience.
# Features
- Access to LTE
- Fetch data from the LTE Internet
- Make and receive calls
- Send and receive messages
- And more!
# How to use
First, define a `HardwareSerial` for the module.
For example:
```cpp
#include <Arduino.h>
HardwareSerial SIMSerial(1);
```
Then, include the library and define a variable for the library with RX and TX pins
Finally, you can use functions from the library to serve your purposes!
