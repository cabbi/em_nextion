#ifndef __NEXTION
#define __NEXTION

#include <Arduino.h>

#include "em_log.h"
#include "em_com_device.h"
#include "em_sync_value.h"

enum EmNextionRet
{
  ACK_CMD_SUCCEED = 0x01,
  ACK_CURRENT_PAGE_ID = 0x66,
  ACK_STRING = 0x70,
  ACK_NUMBER = 0x71,
  INVALID_CMD = 0x00,
  INVALID_COMPONENT_ID = 0x02,
  INVALID_PAGE_ID = 0x03,
  INVALID_PICTURE_ID = 0x04,
  INVALID_FONT_ID = 0x05,
  INVALID_BAUD = 0x11,
  INVALID_VARIABLE = 0x1A,
  INVALID_OPERATION = 0x1B
};

class EmNexPage;
class EmNexPageElement;
class EmNexText;
class EmNexInteger;
class EmNexFloat;

class EmNextion: public EmLog
{
    friend class EmNexPage;
    friend class EmNexPageElement;
    friend class EmNexText;
    friend class EmNexInteger;
    friend class EmNexFloat;

public:
  EmNextion(EmComSerial& serial, 
            uint32_t timeoutMs=500, 
            bool logEnabled=false);

  bool Init();

  bool IsInit()
    { return m_IsInit;}

  bool GetCurPage(uint8_t& page_id);
  bool SetCurPage(const EmNexPage& page);

  bool GetNumElementValue(const char* page_name, 
                          const char* element_name, 
                          int32_t& val);
  bool GetTextElementValue(const char* page_name, 
                          const char* element_name, 
                          char* txt, 
                          size_t len);

  bool SetNumElementValue(const char* page_name, 
                          const char* element_name, 
                          int32_t val);
  bool SetTextElementValue(const char* page_name, 
                           const char* element_name, 
                           const char* txt);

protected:
  bool SendGetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property);
  bool SendSetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property, 
                  int32_t value);
  bool SendSetCmd(const char* page_name, 
                  const char* element_name, 
                  const char* property, 
                  const char* value);
  bool GetNumber(int32_t& val);
  bool GetString(char* txt, size_t len);

  bool Send(const char* cmd);
  bool Ack(uint8_t ack_code);
  
  void ReleaseSerial() 
  { 
      m_Serial.ReleaseDevice(); 
  };

protected:
  bool _Send(const char* cmd);
  bool _Recv(uint8_t ack_code, char* buf, uint8_t len, bool isText=false);
  bool _Result(bool result)
  { 
      if (!result) m_IsInit = false;
      return result;
  }

private:
  EmComSerial& m_Serial;       
  uint32_t m_TimeoutMs;
  bool m_IsInit;
};

class EmNexObject: public EmLog {
  public:
    EmNexObject(EmNextion& nex,
                const char* name,
                bool logEnabled=false)
      : EmLog(logEnabled),
        m_name(name),
        m_nex(nex)
    {
    }

    virtual const char* Name() const { return m_name; }

  protected:
    const char* m_name;
    EmNextion& m_nex;
};

class EmNexPage: public EmNexObject
{
  public:
    EmNexPage(EmNextion& nex, 
              const int id, 
              const char* name,
              bool logEnabled=false)
      : EmNexObject(nex, name, logEnabled),
        m_id(id)
    {}
    
    const int m_id;

    bool IsCurrent() {
        uint8_t id;
        return m_nex.GetCurPage(id) && id == m_id;
    }
};

class EmNexPageElement: public EmNexObject
{
  public:
    EmNexPageElement(EmNextion& nex,
                     EmNexPage& page, 
                     const char* name,
                     bool logEnabled=false)
      : EmNexObject(nex, name, logEnabled),
        m_page(page)
    {}

  protected:
    const EmNexPage& m_page;
};

class EmNexText: public EmNexPageElement,
                 public EmValueSource<char*>
{
  public:
    EmNexText(EmNextion& nex, 
              EmNexPage& page, 
              const char* name,
              const uint8_t len,
              bool logEnabled=false)
      : EmNexPageElement(nex, page, name, logEnabled),
        EmValueSource<char*>(len)
    {}

    virtual bool GetSourceValue(char* value);
    virtual bool SetSourceValue(const char* value);
};

class EmNexInteger: public EmNexPageElement,
                    public EmValueSource<int32_t>
{
    public:
      EmNexInteger(EmNextion& nex, 
                   EmNexPage& page, 
                   const char* name,
                   bool logEnabled=false)
        : EmNexPageElement(nex, page, name, logEnabled)
      {}

    virtual bool GetSourceValue(int32_t& value);
    virtual bool SetSourceValue(const int32_t value);
};

class EmNexFloat: public EmNexPageElement,
                  public EmValueSource<float>
{
    public:
      EmNexFloat(EmNextion& nex, 
                 EmNexPage& page, 
                 const char* name,
                 uint8_t dec_places,
                 bool logEnabled=false)
        : EmNexPageElement(nex, page, name, logEnabled),
          m_dec_places(dec_places)
      {}

    virtual bool GetSourceValue(float& value);
    virtual bool SetSourceValue(const float value);

    const uint8_t m_dec_places;
};

// A two labels number
class EmNexDecimal: public EmValueSource<float>
{
    public:
      EmNexDecimal(EmNextion& nex, 
                   EmNexPage& page, 
                   const char* int_name,
                   const char* dec_name,
                   uint8_t dec_places,
                   bool logEnabled=false)
        : m_int_element(nex, page, int_name, logEnabled),
          m_dec_element(nex, page, dec_name, logEnabled),
          m_dec_places(dec_places)
      {}

    virtual bool GetSourceValue(float& value) { return false; }
    virtual bool SetSourceValue(const float value);

    EmNexInteger m_int_element;
    EmNexInteger m_dec_element;
    const uint8_t m_dec_places;
};

#endif
