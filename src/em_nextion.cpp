#include <stdarg.h>

#include "em_defs.h"
#include "em_string.h"
#include "em_timeout.h"
#include "em_nextion.h"

#ifdef EM_HW_SERIAL_AVR
bool EmNextion::scanBaudrate(uint32_t& baud) const {
#else
bool EmNextion::scanBaudrate(uint32_t& baud, int8_t rxPin, int8_t txPin) const {
#endif        
    // This procedure follows the official Nextion recommendations at page:
    // https://nextion.tech/2017/12/08/nextion-hmi-upload-protocol-v1-1/
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    baud = 0;
    uint32_t bauds[] = {921600, 512000, 500000, 460800, 256000, 250000, 230400, 192000, 
                        128000, 115200, 74880, 57600, 38400, 31250, 19200, 9600, 4800, 2400};
    const char msg[] = "DRAKJHSUYDGBNCJHGJKSHBDN" 
                       "\xFF\xFF\xFF" 
                       "connect" 
                       "\xFF\xFF\xFF\xFF\xFF" 
                       "connect" 
                       "\xFF\xFF\xFF";
    for (size_t i=0; i<sizeof(bauds)/sizeof(bauds[0]); i++) {        
        uint32_t testBaud = bauds[i];
    #ifdef EM_HW_SERIAL_AVR
        m_serial.begin(testBaud, SERIAL_8N1);
    #else
        m_serial.begin(testBaud, SERIAL_8N1, rxPin, txPin);
    #endif        
        m_serial.write(msg, sizeof(msg)-1);
        EmString<100> res;
        if (recv_('c', res.buffer(), res.capacity(), true, 100) != EmGetValueResult::failed &&
            res.startsWith("omok")) {
            baud = testBaud;
            return true;
        }
        // Drain any remaining bytes during the suggested timeout between two attempts
        EmTimeout timeout((1000000UL/testBaud) + 30);
        while (!timeout.isExpired()) {
            m_serial.read();
        }
    }
    return false;
}

bool EmNextion::begin(uint32_t baud, int8_t rxPin, int8_t txPin) const
{
#ifdef EM_HW_SERIAL_AVR
    m_serial.begin(static_cast<unsigned long>(baud), SERIAL_8N1);
#else
    m_serial.begin(static_cast<unsigned long>(baud), SERIAL_8N1, rxPin, txPin);
#endif        
    return begin_();
}

bool EmNextion::begin_() const
{
    // Have command feedback on both success/fail  
    sendCmdParam_("bkcmd=3", true);
    sendCmdEnd_();
    m_isInit = ack_(ACK_CMD_SUCCEED);
    return m_isInit;
}

bool EmNextion::wakeup() {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    // Try multiple times to ensure wakeup
    for (uint8_t i=0; i<3; i++) {
        // NOTE: cannot use 'sendCmd_' since 'begin_' will fail if display is sleeping!
        sendCmdParam_("sleep=0", true);
        sendCmdEnd_();
        delay(10);
        sendCmdParam_("bkcmd=3", true); // Get 'ack' char back from commands!
        sendCmdEnd_();
        if (ack_(ACK_CMD_SUCCEED, 500)) {
            return true;
        }
    }
    return false;
}

bool EmNextion::sendCmd_(const char* firstCmd, ...) const
{
    // Before sending let's see if display is active/connected
    if (!m_isInit && !begin_()) {
        return false;
    }
    sendCmdParam_(firstCmd, true);
    va_list args;
    va_start(args, firstCmd);     
    const char* cmdParam = va_arg(args, const char*);
    while (cmdParam) {
        sendCmdParam_(cmdParam, false);
        cmdParam = va_arg(args, const char*);
    }
    va_end(args);

    return sendCmdEnd_();
}

bool EmNextion::sendCmdParam_(const char* cmdParam, bool flushTxRxBuffers) const
{
    if (flushTxRxBuffers) {
        m_serial.flush(true);
    }
    return bResult_(m_serial.write(cmdParam) > 0);
}

bool EmNextion::sendCmdEnd_() const
{
    m_serial.write(0xFF);
    m_serial.write(0xFF);
    return bResult_(m_serial.write(0xFF) == 1);
}

EmGetValueResult EmNextion::recv_(uint8_t ackCode, 
                                  char* buf, 
                                  uint8_t len, 
                                  bool isText,
                                  uint32_t timeoutMs) const
{
    bool value_changed = false;
    bool got_ackCode=false;
    bool got_buffer=(len==0);
    uint8_t term_count=0;
    uint8_t buf_pos=0;
    EmTimeout rxTimeout(timeoutMs ? timeoutMs : m_timeoutMs);
    while (!rxTimeout.isExpired(false)) {
        while (m_serial.available()) {
            uint8_t c = static_cast<uint8_t>(m_serial.read());
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
                    return result_(false, value_changed);
                }
                term_count++;
                if (term_count>=3) {
                    // Got everything
                    return result_(true, value_changed);
                }
            }
        }
    }
    logDebug<60>("Received %d of %d bytes [Timeout elapsed!]", buf_pos, len);
    return result_(false, value_changed);
}

EmGetValueResult EmNextion::result_(bool result, bool valueChanged) const
{ 
    if (!result) { 
        m_isInit = false;
        return EmGetValueResult::failed;
    }
    return valueChanged ? 
           EmGetValueResult::succeedNotEqualValue : 
           EmGetValueResult::succeedEqualValue;
}

bool EmNextion::bResult_(bool result) const
{ 
    if (!result) { 
        m_isInit = false;
    }
    return result;
}

bool EmNextion::ack_(uint8_t ackCode, uint32_t timeoutMs) const 
{
    logDebug(F("Waiting ACK"));
    return EmGetValueResult::failed != recv_(ackCode, NULL, 0, false, timeoutMs);
}

bool EmNextion::isCurPage(uint8_t pageId) const {
    uint8_t id;
    return getCurPage(id) && id == pageId;
}

bool EmNextion::getCurPage(uint8_t& pageId) const 
{
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    if (!sendCmd_("sendme", NULL)) {
        return false;
    }
    if (EmGetValueResult::failed != recv_(ACK_CURRENT_PAGE_ID, 
                                         (char*)&pageId, 1)) {
        return true;
    }
    return false;
}

bool EmNextion::setCurPage(uint8_t pageId) const 
{
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    char buf[3];
    if (!sendCmd_("page ", to_str(buf, 3, pageId), NULL)) {
        return false;
    }
    return ack_(ACK_CMD_SUCCEED);
}

bool EmNextion::setCurPage(const char* pageName) const 
{
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    if (!sendCmd_("page ", pageName, NULL)) {
        return false;
    }
    return ack_(ACK_CMD_SUCCEED);
}

EmGetValueResult EmNextion::getNumElementValue(const char* pageName, 
                                               const char* elementName, 
                                               int32_t& val) const 
{
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    EmGetValueResult res = EmGetValueResult::failed;
    if (sendGetCmd_(pageName, elementName, "val")) {
        res = getNumber_(val);
    }
    logDebug<50>("get: %s -> %d [%s]", 
                 elementName,
                 val,
                 (EmGetValueResult::failed != res ? 
                  " [SUCCESS]" : 
                  " [FAIL]"));
    return res;
}

bool EmNextion::setNumElementValue(const char* pageName, 
                                   const char* elementName, 
                                   int32_t val) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendSetCmd_(pageName, elementName, "val", val)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("set: %s -> %d [%s]", 
                 elementName,
                 val,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::setTextElementValue(const char* pageName, 
                                    const char* elementName, 
                                    const char* txt) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendSetCmd_(pageName, elementName, "txt", txt)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("set: %s -> %s [%s]", 
                 elementName,
                 txt,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::setVisible(const char* elementName, 
                           bool visible) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendCmd_("vis ", elementName, visible ? ",1" : ",0", NULL)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("visible: %s -> %s [%s]", 
                 elementName,
                 visible,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::setVisible(uint8_t pageId, 
                           const char* elementName, 
                           bool visible) const {
    return isCurPage(pageId) && setVisible(elementName, visible);
}

bool EmNextion::setPicture(const char* pageName, 
                           const char* elementName, 
                           uint8_t picId) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendSetCmd_(pageName, elementName, "pic", picId)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("pic: %s -> %s [%s]", 
                 elementName,
                 picId,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::getPicture(const char* pageName, 
                           const char* elementName, 
                           uint8_t& picId) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendGetCmd_(pageName, elementName, "val")) {
        int32_t val;
        res = getNumber_(val) != EmGetValueResult::failed;
        if (res) {
            picId = static_cast<uint8_t>(val);
        }
    }
    logDebug<50>("pic: %s -> %d [%s]", 
                 elementName,
                 picId,
                 res ? " [SUCCESS]" : " [FAIL]");
    return res;
}

bool EmNextion::click(const char* elementName, 
                      bool pressed) const {
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_serialLock);
#endif
    bool res = false;
    if (sendCmd_("click ", elementName, pressed ? ",1" : ",0", NULL)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("click: %s -> %s [%s]", 
                 elementName,
                 pressed,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::click(uint8_t pageId, 
                      const char* elementName, 
                      bool pressed) const {
    return isCurPage(pageId) && click(elementName, pressed);
}


bool EmNextion::setColor_(const char* pageName, 
                          const char* elementName, 
                          const char* colorCode, 
                          uint16_t color565) const{
    bool res = false;
    if (sendSetCmd_(pageName, elementName, colorCode, color565)) {
        res = ack_(ACK_CMD_SUCCEED);
    }
    logDebug<50>("%s: %s -> %s [%s]", 
                 colorCode,
                 elementName,
                 color565,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::getColor_(const char* pageName, 
                          const char* elementName, 
                          const char* colorCode, 
                          uint16_t& color565) const {
    bool res = false;
    if (sendGetCmd_(pageName, elementName, colorCode)) {
        int32_t val;
        res = getNumber_(val) != EmGetValueResult::failed;
        if (res) {
            color565 = static_cast<uint16_t>(val);
        }
    }
    logDebug<50>("%s: %s -> %s [%s]", 
                 colorCode,
                 elementName,
                 color565,
                 (res ? " [SUCCESS]" : " [FAIL]"));
    return res;
}

bool EmNextion::sendGetCmd_(const char* pageName, 
                  const char* elementName, 
                  const char* property) const
{
    return sendCmd_("get ", 
                    pageName, ".", 
                    elementName, ".",
                    property, NULL);
}

bool EmNextion::sendSetCmd_(const char* pageName, 
                            const char* elementName, 
                            const char* property, 
                            int32_t value) const
{
    char buf[11];
    return sendCmd_(pageName, ".", 
                    elementName, ".",
                    property, "=",
                    to_str(buf, 11, value), NULL);
}

bool EmNextion::sendSetCmd_(const char* pageName, 
                            const char* elementName, 
                            const char* property, 
                            const char* value) const
{
    return sendCmd_(pageName, ".", 
                    elementName, ".",
                    property, "=",
                    "\"", value, "\"", NULL);
}


EmGetValueResult EmNextion::getNumber_(int32_t& val) const 
{
    // Create a copy in case communication fails 
    // (i.e. some bytes might be modified by _recv method!)
    int32_t buf = val;
    EmGetValueResult res = recv_(ACK_NUMBER, (char*)&buf, sizeof(buf));

    if (EmGetValueResult::failed != res) {
        // TODO: Nextion is little endian, should we check about big endian CPU?
        val = buf;        
    }
    return res;
}

EmGetValueResult EmNextion::getString_(char* txt, 
                                       uint8_t bufLen, 
                                       const char* elementName) const  
{
    EmGetValueResult res = recv_(ACK_STRING, txt, bufLen, true);
    if (EmGetValueResult::failed == res) {
        txt[0]=0;
    } else { 
        txt[bufLen-1]=0;
    } 
    logDebug<50>("get: %s -> %s [%s]", 
                 elementName,
                 txt,
                 (EmGetValueResult::failed != res ? 
                  " [SUCCESS]" : 
                  " [FAIL]"));
    return res;
}
