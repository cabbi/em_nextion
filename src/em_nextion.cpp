#include <stdarg.h>

#include "em_nextion.h"
#include "em_timeout.h"
#include "em_defs.h"


// NOTE: program MUST set "bauds" at first page initialization)
EmNextion::EmNextion(EmComSerial& serial, 
                     uint32_t timeoutMs, 
                     EmLogLevel logLevel)
 : EmLog("Nex", logLevel),
   m_Serial(serial),
   m_TimeoutMs(timeoutMs),
   m_IsInit(false)
{
}

bool EmNextion::Init() const
{
    // Have command feedback on both success/fail  
    _sendCmdParam("bkcmd=3");
    _sendCmdEnd();
    m_IsInit = _ack(ACK_CMD_SUCCEED);
    return m_IsInit;
}

bool EmNextion::_sendCmd(const char* firstCmd, ...) const
{
    // Before sending let's see if display is active/connected
    if (!m_IsInit) {
        if (!Init()) {
            return false;
        } 
    }
    m_Serial.flush();
    _sendCmdParam(firstCmd);
    va_list args;
    va_start(args, firstCmd);     
    const char* cmdParam = va_arg(args, const char*);
    while (cmdParam) {
        _sendCmdParam(cmdParam);
        cmdParam = va_arg(args, const char*);
    }
    va_end(args);

    return _sendCmdEnd();
}

bool EmNextion::_sendCmdParam(const char* cmdParam) const
{
    return m_Serial.write(cmdParam) > 0;
}

bool EmNextion::_sendCmdEnd() const
{
    m_Serial.write(0xFF);
    m_Serial.write(0xFF);
    return m_Serial.write(0xFF) == 1;
}

EmGetValueResult EmNextion::_recv(uint8_t ackCode, 
                                  char* buf, 
                                  uint8_t len, 
                                  bool isText) const
{
    bool value_changed = false;
    bool got_ackCode=false;
    bool got_buffer=(len==0);
    uint8_t term_count=0;
    uint8_t buf_pos=0;
    EmTimeout rxTimeout(m_TimeoutMs);
    while (!rxTimeout.IsElapsed(false)) {
        while (m_Serial.available()) {
            uint8_t c = static_cast<uint8_t>(m_Serial.read());
            // Still waiting for ack code?
            if (!got_ackCode) {
                got_ackCode = (c==ackCode);
            }
            // Still waiting for data?
            else if (!got_buffer) {
                if (isText && c == 0xFF) {
                    buf[buf_pos] = 0;
                    got_buffer = true;    
                    term_count = 1;
                } else {
                    if (buf[buf_pos] != c) {
                        // We might have reached text buffer size 
                        if (!(isText && buf_pos==len-1)) {
                            value_changed = true;
                        }
                    }
                    buf[buf_pos++] = static_cast<char>(c);
                    got_buffer = (buf_pos==len);
                }
            }
            // Still waiting for terminators!
            else {
                if (c != 0xFF) {
                    if (isText) {
                        // We might have reached text buffer 
                        // size but not all display text!
                        buf[buf_pos] = 0;
                        continue;
                    }
                    return _result(false, value_changed);
                }
                term_count++;
                if (term_count>=3) {
                    // Got everything
                    return _result(true, value_changed);
                }
            }
        }
    }
    LogDebug<50>("RX: %s [SUCCESS]", buf);
    return _result(false, value_changed);
}

EmGetValueResult EmNextion::_result(bool result, bool valueChanged) const
{ 
    if (!result) { 
        m_IsInit = false;
        return EmGetValueResult::failed;
    }
    return valueChanged ? 
           EmGetValueResult::succeedNotEqualValue : 
           EmGetValueResult::succeedEqualValue;
}

bool EmNextion::_ack(uint8_t ackCode) const 
{
    LogDebug(F("Waiting ACK"));
    return EmGetValueResult::failed != _recv(ackCode, NULL, 0);
}

bool EmNextion::IsCurPage(uint8_t pageId) const {
    uint8_t id;
    return GetCurPage(id) && id == pageId;
}

bool EmNextion::GetCurPage(uint8_t& pageId) const 
{
    if (!_sendCmd("sendme", NULL)) {
        return false;
    }
    if (EmGetValueResult::failed != _recv(ACK_CURRENT_PAGE_ID, 
                                         (char*)&pageId, 1)) {
        return true;
    }
    return false;
}

bool EmNextion::SetCurPage(uint8_t pageId) const 
{
    char buf[3];
    if (!_sendCmd("page ", to_str(buf, 3, pageId), NULL)) {
        return false;
    }
    return _ack(ACK_CMD_SUCCEED);
}

bool EmNextion::SetCurPage(const char* pageName) const 
{
    if (!_sendCmd("page ", pageName, NULL)) {
        return false;
    }
    return _ack(ACK_CMD_SUCCEED);
}

EmGetValueResult EmNextion::GetNumElementValue(const char* pageName, 
                                               const char* elementName, 
                                               int32_t& val) const 
{
    EmGetValueResult res = EmGetValueResult::failed;
    if (_sendGetCmd(pageName, elementName, "val")) {
        res = _getNumber(val);
    }
    LogDebug<50>("get: %s -> %d [%s]", 
                 elementName,
                 val,
                 (EmGetValueResult::failed != res ? 
                  " [SUCCESS]" : 
                  " [FAIL]"));
    return res;
}

bool EmNextion::SetNumElementValue(const char* pageName, 
                                   const char* elementName, 
                                   int32_t val) const {
    bool res = false;
    if (_sendSetCmd(pageName, elementName, "val", val)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("set: %s -> %d [%s]", 
                 elementName,
                 val,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::SetTextElementValue(const char* pageName, 
                                    const char* elementName, 
                                    const char* txt) const {
    bool res = false;
    if (_sendSetCmd(pageName, elementName, "txt", txt)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("set: %s -> %s [%s]", 
                 elementName,
                 txt,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::SetVisible(const char* elementName, 
                           bool visible) const {
    bool res = false;
    if (_sendCmd("vis ", elementName, visible ? ",1" : ",0", NULL)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("visible: %s -> %s [%s]", 
                 elementName,
                 visible,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::SetVisible(uint8_t pageId, 
                           const char* elementName, 
                           bool visible) const {
    return IsCurPage(pageId) && SetVisible(elementName, visible);
}

bool EmNextion::SetPicture(const char* pageName, 
                           const char* elementName, 
                           uint8_t picId) const {
    bool res = false;
    if (_sendSetCmd(pageName, elementName, "pic", picId)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("pic: %s -> %s [%s]", 
                 elementName,
                 picId,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::GetPicture(const char* pageName, 
                           const char* elementName, 
                           uint8_t& picId) const {
    bool res = false;
    if (_sendGetCmd(pageName, elementName, "val")) {
        int32_t val;
        res = _getNumber(val) != EmGetValueResult::failed;
        if (res) {
            picId = static_cast<uint8_t>(val);
        }
    }
    LogDebug<50>("pic: %s -> %d [%s]", 
                 elementName,
                 picId,
                 res ? " [SUCCESS]" : " [FAIL]");
    return res;
}

bool EmNextion::Click(const char* elementName, 
                      bool pressed) const {
    bool res = false;
    if (_sendCmd("click ", elementName, pressed ? ",1" : ",0", NULL)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("click: %s -> %s [%s]", 
                 elementName,
                 pressed,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::Click(uint8_t pageId, 
                      const char* elementName, 
                      bool pressed) const {
    return IsCurPage(pageId) && Click(elementName, pressed);
}


bool EmNextion::_setColor(const char* pageName, 
                          const char* elementName, 
                          const char* colorCode, 
                          uint16_t color565) const{
    bool res = false;
    if (_sendSetCmd(pageName, elementName, colorCode, color565)) {
        res = _ack(ACK_CMD_SUCCEED);
    }
    LogDebug<50>("%s: %s -> %s [%s]", 
                 colorCode,
                 elementName,
                 color565,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::_getColor(const char* pageName, 
                          const char* elementName, 
                          const char* colorCode, 
                          uint16_t& color565) const {
    bool res = false;
    if (_sendGetCmd(pageName, elementName, colorCode)) {
        int32_t val;
        res = _getNumber(val) != EmGetValueResult::failed;
        if (res) {
            color565 = static_cast<uint16_t>(val);
        }
    }
    LogDebug<50>("%s: %s -> %s [%s]", 
                 colorCode,
                 elementName,
                 color565,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::_sendGetCmd(const char* pageName, 
                  const char* elementName, 
                  const char* property) const
{
    return _sendCmd("get ", 
                    pageName, ".", 
                    elementName, ".",
                    property, NULL);
}

bool EmNextion::_sendSetCmd(const char* pageName, 
                            const char* elementName, 
                            const char* property, 
                            int32_t value) const
{
    char buf[11];
    return _sendCmd(pageName, ".", 
                    elementName, ".",
                    property, "=",
                    to_str(buf, 11, value), NULL);
}

bool EmNextion::_sendSetCmd(const char* pageName, 
                            const char* elementName, 
                            const char* property, 
                            const char* value) const
{
    return _sendCmd(pageName, ".", 
                    elementName, ".",
                    property, "=",
                    "\"", value, "\"", NULL);
}


EmGetValueResult EmNextion::_getNumber(int32_t& val) const 
{
    // Create a copy in case communication fails 
    //(i.e. some bytes might be modified by _recv method!)
    char buf[4];
    // TODO: Nextion is little endian, should we check about big endian CPU?
    memcpy(buf, &val, sizeof(buf));

    EmGetValueResult res = _recv(ACK_NUMBER, buf, sizeof(buf));

    if (EmGetValueResult::failed != res) {
        val = ((int32_t)buf[3]<<24) | 
              ((int32_t)buf[2]<<16) | 
              ((int32_t)buf[1]<<8) | 
              ((int32_t)buf[0]);        
    }
    return res;
}

EmGetValueResult EmNextion::_getString(char* txt, 
                                       uint8_t bufLen, 
                                       const char* elementName) const  
{
    EmGetValueResult res = _recv(ACK_STRING, txt, bufLen, true);
    if (EmGetValueResult::failed == res) {
        txt[0]=0;
    } else { 
        txt[bufLen-1]=0;
    } 
    LogDebug<50>("get: %s -> %s [%s]", 
                 elementName,
                 txt,
                 (EmGetValueResult::failed != res ? 
                  " [SUCCESS]" : 
                  " [FAIL]"));
    return res;
}

bool EmNexDecimal::SetValue(double const value) 
{
    int32_t exp = iPow10(m_decPlaces);
    int32_t dispValue = iRound(value*static_cast<double>(exp));
    return Nex().SetNumElementValue(Page().Name(), m_name, iDiv(dispValue, exp)) &&
           Nex().SetNumElementValue(Page().Name(), m_decElementName, dispValue % exp);        
}

EmGetValueResult EmNexDecimal::GetValue(float& value) const
{
    double val;
    EmGetValueResult res = GetValue(val);
    if (EmGetValueResult::failed != res) {
        value = static_cast<float>(val);
    }
    return res;
}

EmGetValueResult EmNexDecimal::GetValue(double& value) const 
{ 
    double prevValue = value;
    EmGetValueResult res;
    int32_t intVal;
    res = Nex().GetNumElementValue(Page().Name(), m_name, intVal);
    if (res == EmGetValueResult::failed) {
        return EmGetValueResult::failed;
    }
    int32_t decVal;
    res = Nex().GetNumElementValue(Page().Name(), m_decElementName, decVal);
    if (res == EmGetValueResult::failed) {
        return EmGetValueResult::failed;
    }
    value = intVal+(static_cast<double>(decVal)/pow(10, m_decPlaces));

    return prevValue == value ? 
           EmGetValueResult::succeedEqualValue :
           EmGetValueResult::succeedNotEqualValue;
}
