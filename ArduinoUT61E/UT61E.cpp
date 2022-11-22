#include "UT61E.h"

/****************************************************************************/
// Constructor
/****************************************************************************/
UT61E::UT61E(HardwareSerial* serialObj, int dtrPin) {
  _Serial = serialObj;
  _dtrPin = dtrPin;
  _Serial->begin(19200, SERIAL_7O1);          // seven bit word length, odd parity, one stop bit.
  pinMode(_dtrPin, OUTPUT);
  digitalWrite(_dtrPin, HIGH);                // DTR needs to be high as it powers the adapter
                                              // RTS needs to be grounded as well to power it
                                              // RX Line needs inversion (e.g. 74HC14N) before Arduino

  _resistance = 0.0;
}

/****************************************************************************/
// readPacket()
/****************************************************************************/
bool UT61E::readPacket(void) {
  // read the packet
  _Serial->flush();                                                 // flush the input buffer

  byte size = _Serial->readBytesUntil(10, (char *)&_packet, 14);    // read until LF, i.e. end of packet
  _Serial->read();                                                  // flush the LF
  if (size == 13) {                                                 // packet size is 14 including the LF
    return true;
  } else {
    // retry now that the incomplete partial packet has been read to end of packet
    size = _Serial->readBytesUntil(10, (char *)&_packet, 14);       // read until LF, i.e. end of packet
    _Serial->read();                                                // flush the LF
    if (size == 13) {                                               // packet size is 14 including the LF
      return true;
    } else {
      return false;
    }
  }
}

/****************************************************************************/
// massagePacket()
/****************************************************************************/
void UT61E::massagePacket(void) {
    // strip out the junk
    _packet.range  = _packet.range  & B00000111;          // bits 7 through 3 are always 00110
    _packet.digit1 = _packet.digit1 & B00001111;          // bits 7 through 4 are always  0011
    _packet.digit2 = _packet.digit2 & B00001111;          // bits 7 through 4 are always  0011
    _packet.digit3 = _packet.digit3 & B00001111;          // bits 7 through 4 are always  0011
    _packet.digit4 = _packet.digit4 & B00001111;          // bits 7 through 4 are always  0011
    _packet.digit5 = _packet.digit5 & B00001111;          // bits 7 through 4 are always  0011
    _packet.mode   = _packet.mode   & B00001111;          // bits 7 through 4 are always  0011
}

/****************************************************************************/
// measureResistance()
/****************************************************************************/
int UT61E::measureResistance(void) {
  unsigned long start = millis();
  for (;;) {
    if (_Serial->available() == 0) {
      if (millis() - start > 1500) {
        return UT61E_ERROR_TIMEOUT;
      }
    } else {
      // read the packet
      if (!this->readPacket()) {
        return UT61E_ERROR_READING_PACKET;
      }

      // strip out the junk
      this->massagePacket();

      // check meter mode
      if (_packet.mode !=  3) {                     // Mode 3 = Resistance Measurement
        return UT61E_ERROR_INVALID_MODE;
      }

      // do the maths
      _resistance = (100.0 * _packet.digit1) + (10.0 * _packet.digit2) + (1.0 * _packet.digit3)
                + (0.1 * _packet.digit4) + (0.01 * _packet.digit5);
      if (_packet.range > 0) {
        _resistance = _resistance * (pow(10.0, _packet.range));
      }
      return UT61E_SUCCESS;
    }
  }
}

/****************************************************************************/
// getResistance()
/****************************************************************************/
float UT61E::getResistance(void) {
  return _resistance;
}