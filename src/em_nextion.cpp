#include <math.h> 
#include "em_nextion.h"
#include "em_timeout.h"


// NOTE: program MUST set "bauds" at first page initialization)
EmNextion::EmNextion(EmComSerial& serial, 
                     uint32_t timeoutMs, 
                     bool logEnabled)
 : EmLog(logEnabled),
   m_Serial(serial),
   m_TimeoutMs(timeoutMs),
   m_IsInit(false)
{
}

bool EmNextion::Init()
{
    // Have command feedback on both success/fail  
    _Send("bkcmd=3");
    m_IsInit = Ack(ACK_CMD_SUCCEED);
    return m_IsInit;
}

bool EmNextion::Send(const char* cmd)
{
    // Before sending let's see if display is active/connected
    if (!m_IsInit)
    {
        if (!Init())
        {
            return false;
        } 
    }
    return _Send(cmd);
}

bool EmNextion::_Send(const char* cmd)
{
    m_Serial.flush();
    m_Serial.write(cmd);
    LogInfo("TX: ", false);
    LogInfo(cmd, true);
    m_Serial.write(0xFF);
    m_Serial.write(0xFF);
    return m_Serial.write(0xFF) == 1;
}

bool EmNextion::_Recv(uint8_t ack_code, char* buf, uint8_t len, bool isText)
{
    bool got_ack_code=false;
    bool got_buffer=(len==0);
    uint8_t term_count=0;
    uint8_t buf_pos=0;
    EmTimeout rxTimeout(m_TimeoutMs);
    LogInfo("RX: ", false);
    while (!rxTimeout.IsElapsed(false))
    {
        while (m_Serial.available())
        {
            int c = m_Serial.read();
            LogInfo((uint8_t)c, false);
            LogInfo("-", false);
            // Still waiting for ack code?
            if (!got_ack_code)
            {
                got_ack_code = (c==ack_code);
            }
            // Still waiting for data?
            else if (!got_buffer)
            {
                if (isText && c == 0xFF)
                {
                    buf[buf_pos++] = 0;
                    got_buffer = true;    
                }
                else
                {
                    buf[buf_pos++] = c;
                    got_buffer = (buf_pos==len);
                }
            }
            // Still waiting for terminators!
            else
            {
                if (c != 0xFF)
                {
                    return _Result(false);
                }
                term_count++;
                if (term_count>=3)
                {
                    // We got evertying
                    return _Result(true);
                }
            }
        }
    }
    return _Result(false);
}

bool EmNextion::Ack(uint8_t ack_code) 
{
    LogInfo("Waiting ACK", false);
    return _Recv(ack_code, NULL, 0);
}

bool EmNextion::GetCurPage(uint8_t& page_id) 
{
    if (!Send("sendme"))
    {
        return false;
    }
    if (_Recv(ACK_CURRENT_PAGE_ID, (char*)&page_id, 1))
    {
        return true; //Ack(ACK_CMD_SUCCEED);
    }
    return false;
}

bool EmNextion::SetCurPage(const EmNexPage& page) 
{
    const uint8_t max_size = 8;
    char cmd[max_size+1];
    snprintf(cmd, max_size, "page %d", page.Id());
    cmd[max_size]=0;
    if (!Send(cmd))
    {
        return false;
    }
    return Ack(ACK_CMD_SUCCEED);
}

bool EmNextion::GetNumElementValue(const char* page_name, 
                                 const char* element_name, 
                                 int32_t& val) 
{
    bool res = false;
    LogInfo("get: ", false);LogInfo(element_name, false);
    if (SendGetCmd(page_name, element_name, "val"))
    {
        res = GetNumber(val);
        LogInfo(" -> ", false);LogInfo(val, false);
    }
    LogInfo(res ? " [SUCCESS]" : " [FAIL]", true);
    return res;
}

bool EmNextion::GetTextElementValue(const char* page_name, 
                                    const char* element_name, 
                                    char* txt, 
                                    size_t len) 
{
    bool res = false;
    LogInfo("get: ", false);LogInfo(element_name, false);
    if (SendGetCmd(page_name, element_name, "txt"))
    {
        res = GetString(txt, len);    
        LogInfo(" -> ", false);LogInfo(txt, false);
    }
    LogInfo(res ? " [SUCCESS]" : " [FAIL]", true);
    return res;
}

bool EmNextion::SetNumElementValue(const char* page_name, 
                                 const char* element_name, 
                                 int32_t val) {
    bool res = false;
    LogInfo("set: ", false);LogInfo(element_name, false);LogInfo(" -> ", false);LogInfo(val, false);
    if (SendSetCmd(page_name, element_name, "val", val))
    {
        res = Ack(ACK_CMD_SUCCEED);
    }
    LogInfo(res ? " [SUCCESS]" : " [FAIL]", true);
    return res;
}

bool EmNextion::SetTextElementValue(const char* page_name, 
                                  const char* element_name, 
                                  const char* txt) {
    bool res = false;
    LogInfo("set: ", false);LogInfo(element_name, false);LogInfo(" -> ", false);LogInfo(txt, false);
    if (SendSetCmd(page_name, element_name, "txt", element_name))
    {
        res = Ack(ACK_CMD_SUCCEED);
    }
    LogInfo(res ? " [SUCCESS]" : " [FAIL]", true);
    return res;
}

bool EmNextion::SendGetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property)
{
    const uint8_t max_size = 80;
    char cmd[max_size+1];
    snprintf(cmd, max_size, "get %s.%s.%s", page_name, element_name, property);
    cmd[max_size]=0;
    return Send(cmd);
}

bool EmNextion::SendSetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property, int32_t value)
{
    const uint8_t max_size = 100;
    char cmd[max_size+1];
    snprintf(cmd, max_size, "%s.%s.%s=%ld", page_name, element_name, property, value);
    cmd[max_size]=0;
    return Send(cmd);
}

bool EmNextion::SendSetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property, const char* value)
{
    const uint8_t max_size = 200;
    char cmd[max_size+1];
    snprintf(cmd, max_size, "%s.%s.%s=\"%s\"", page_name, element_name, property, value);
    cmd[max_size]=0;
    return Send(cmd);
}


bool EmNextion::GetNumber(int32_t& val) 
{
    uint8_t buf[4] = {0};

    if (_Recv(ACK_NUMBER, (char*)buf, sizeof(buf)))
    {
        val = ((int32_t)buf[3]<<24) | 
              ((int32_t)buf[2]<<16) | 
              ((int32_t)buf[1]<<8) | 
              ((int32_t)buf[0]);        
        return true;
    }
    return false;
}

bool EmNextion::GetString(char* txt, size_t len)  
{
    if (_Recv(ACK_STRING, txt, len, true))
    {
        txt[len-1]=0;
        return true;
    }
    txt[0]=0;
    return false;
}

bool EmNexText::GetSourceValue(char* value)
{
    return m_nex.GetTextElementValue(m_page.Name(), m_name, value, m_MaxLen);
}

bool EmNexText::SetSourceValue(const char* value)
{
    return m_nex.SetTextElementValue(m_page.Name(), m_name, value);
}

bool EmNexInteger::GetSourceValue(int32_t& value)
{
    return m_nex.GetNumElementValue(m_page.Name(), m_name, value);
}

bool EmNexInteger::SetSourceValue(const int32_t value)
{
    return m_nex.SetNumElementValue(m_page.Name(), m_name, value);
}

bool EmNexFloat::GetSourceValue(float& value)
{
    bool res = false;
    int32_t val;
    if (m_nex.GetNumElementValue(m_page.Name(), m_name, val))
    {
        value = (float)val/pow(10, m_dec_places);
        LogInfo(" -> ", false);LogInfo(value, false);
        res = true;
    }
    return res;
}

bool EmNexFloat::SetSourceValue(const float value)
{
    return m_nex.SetNumElementValue(m_page.Name(), 
                                    m_name, 
                                    (int32_t)round(value*pow(10, m_dec_places)));
}

bool EmNexDecimal::SetSourceValue(const float value)
{
    bool res = false;
    int32_t exp = pow(10, m_dec_places);
    int32_t dispValue = (int32_t)round(value*exp);
    res = m_int_element.SetSourceValue(dispValue/exp);
    res = m_dec_element.SetSourceValue(dispValue%exp);

    return res;
}
