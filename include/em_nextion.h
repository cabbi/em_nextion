#ifndef __NEXTION
#define __NEXTION

#include <stdint.h>

#include "em_log.h"
#include "em_com_device.h"
#include "em_sync_value.h"

enum EmNextionRet: uint8_t {
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

class EmNextion: public EmLog {
public:
    EmNextion(EmComSerial& serial, 
              uint32_t timeoutMs=10, 
              EmLogLevel logLevel=EmLogLevel::none);

    bool Init() const;

    bool IsInit() const { 
        return m_IsInit;
    }

    bool IsCurPage(uint8_t pageId) const;
    bool GetCurPage(uint8_t& pageId) const;
    bool SetCurPage(uint8_t pageId) const;
    bool SetCurPage(const char* pageName) const;

    EmGetValueResult GetNumElementValue(const char* pageName, 
                                        const char* elementName, 
                                        int32_t& val) const;
    template<size_t len>
    EmGetValueResult GetTextElementValue(const char* pageName, 
                                         const char* elementName, 
                                         char* txt) const;

    bool SetNumElementValue(const char* pageName, 
                            const char* elementName, 
                            int32_t val) const;
    bool SetTextElementValue(const char* pageName, 
                             const char* elementName, 
                             const char* txt) const;

    // Set element visibility.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. visibility attribute is reset if page is changed 
    //     or display recovers from screen saver
    bool SetVisible(const char* elementName, 
                    bool visible) const;

    bool SetVisible(uint8_t pageId, 
                    const char* elementName, 
                    bool visible) const;

protected:
    bool _sendGetCmd(const char* pageName, 
                     const char* elementName, 
                     const char* property) const;
    bool _sendSetCmd(const char* pageName, 
                     const char* elementName, 
                     const char* property, 
                     int32_t value) const;
    bool _sendSetCmd(const char* pageName, 
                     const char* elementName, 
                     const char* property, 
                     const char* value) const;
    EmGetValueResult _getNumber(int32_t& val) const;
    EmGetValueResult _getString(char* txt, 
                                uint8_t bufLen, 
                                const char* elementName) const;

    bool _sendCmd(const char* firstCmd, ...) const;
    bool _sendCmdParam(const char* cmdParam) const;
    bool _sendCmdEnd() const;
    bool _ack(uint8_t ackCode) const;
    EmGetValueResult _recv(uint8_t ackCode, 
                           char* buf, 
                           uint8_t len, 
                           bool isText=false) const;
    EmGetValueResult _result(bool result, bool valueChanged) const;

private:
    EmComSerial& m_Serial;       
    const uint32_t m_TimeoutMs;
    mutable bool m_IsInit;
};

class EmNexObject: public EmLog {
public:
    EmNexObject(const char* name,
                EmLogLevel logLevel=EmLogLevel::none)
     : EmLog("NexObj", logLevel),
       m_name(name) {}

    const char* Name() const { return m_name; }

protected:
    const char* m_name;
};

class EmNexPage: public EmNexObject
{
public:
    EmNexPage(EmNextion& nex,
              const uint8_t id, 
              const char* name,
              EmLogLevel logLevel=EmLogLevel::none)
      : EmNexObject(name, logLevel),
        m_nex(nex),
        m_id(id)
    {}
    
    EmNextion& Nex() const {
        return m_nex;
    }

    uint8_t Id() const {
        return m_id;
    }

    bool IsCurrent() const {
        return Nex().IsCurPage(m_id);
    }

protected:
    EmNextion& m_nex;
    const uint8_t m_id;
};

class EmNexPageElement: public EmNexObject
{
public:
    EmNexPageElement(EmNexPage& page,
                     const char* name,
                     EmLogLevel logLevel=EmLogLevel::none)
     : EmNexObject(name, logLevel),
       m_page(page) {}

    EmNextion& Nex() const {
        return m_page.Nex();
    }

    EmNexPage& Page() const {
        return m_page;
    }

    const char* PageName() const {
        return m_page.Name();
    }

    // Set element visibility.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. visibility attribute is reset if page is changed 
    //     or display recovers from screen saver
    bool SetVisible(bool visible) const {
        return Nex().SetVisible(m_page.Id(), m_name, visible);
    }

protected:
    EmNexPage& m_page;
};

class EmNexText: public EmNexPageElement,
                 public EmValue<char*>
{
public:
    EmNexText(EmNexPage& page,
              const char* name,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, name, logLevel),
       EmValue<char*>() {}

    template<size_t len>
    EmGetValueResult GetValue(char* value) const {
        return Nex().GetTextElementValue<len>(PageName(), m_name, value);
    }

    virtual EmGetValueResult GetValue(char* value) const override {
        // NOTE:
        //  Since 'GetValue' overrides a virtual method it can not 
        //  be template based. 100 should be a good compromise.
        //  To use exact len please use the templated 'getValue' method. 
        return GetValue<100>(value);
    }

    virtual bool SetValue(const char* value) override {
        return Nex().SetTextElementValue(PageName(), m_name, value);
    }
};

class EmNexInteger: public EmNexPageElement,
                    public EmValue<int32_t>
{
public:
    EmNexInteger(EmNexPage& page,
                 const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, name, logLevel) {}

    // Templated methods (not virtual)
    template <class int_type>
    EmGetValueResult GetValue(int_type& value) const {
        int32_t val = static_cast<int32_t>(value);
        EmGetValueResult res = GetValue(val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<int_type>(val);
        }
        return res;
    }

    // virtual 'EmValue' overrides (cannot be templated)
    virtual EmGetValueResult GetValue(int32_t& value) const override {
        return Nex().GetNumElementValue(PageName(), m_name, value);
    }

    virtual bool SetValue(int32_t const value) override {
        return Nex().SetNumElementValue(PageName(), m_name, value);
    }
};

class EmNexReal: public EmNexPageElement,
                 public EmValue<double>
{
public:
    EmNexReal(EmNexPage& page,
              const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, name, logLevel),
       m_decPlaces(decPlaces) {}

    // Templated methods (not virtual)
    template <class real_type>
    EmGetValueResult GetValue(real_type& value) const {
        int32_t val = iMolt<real_type>(value, iPow10(m_decPlaces));
        EmGetValueResult res = Nex().GetNumElementValue(Page().Name(), m_name, val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<real_type>(val)/pow(10, m_decPlaces);
        }
        return res;
    }
 
    template <class real_type>
    bool SetValue(real_type const value) {
        return Nex().SetNumElementValue(PageName(), 
                                        m_name, 
                                        iRound<real_type>(value*iPow10(m_decPlaces)));
    }

    // virtual 'EmValue' overrides (cannot be templated)
    virtual EmGetValueResult GetValue(double& value) const override {
        return GetValue<double>(value);
    }

    virtual bool SetValue(double const value) override {
        return SetValue<double>(value);
    }

protected:
    const uint8_t m_decPlaces;
};

// A two labels number
class EmNexDecimal:public EmNexPageElement,
                   public EmValue<double>
{
public:
    EmNexDecimal(EmNexPage& page,
                 const char* intElementName,
                 const char* decElementName,
                 uint8_t decPlaces,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, intElementName, logLevel),
       m_decElementName(decElementName),
       m_decPlaces(decPlaces) {}

    virtual EmGetValueResult GetValue(float& value) const;
    virtual EmGetValueResult GetValue(double& value) const override;
    virtual bool SetValue(double const value) override;

protected:
    const char* m_decElementName;
    const uint8_t m_decPlaces;
};

template<size_t len>
inline EmGetValueResult EmNextion::GetTextElementValue(
    const char* pageName, 
    const char* elementName, 
    char* txt) const
{
    // Create a copy in case communication fails
    // (i.e. some bytes might be modified by _recv method!)
    char dispTxt[len+1];
    strncpy(dispTxt, txt, len);
    EmGetValueResult res = EmGetValueResult::failed;
    if (_sendGetCmd(pageName, elementName, "txt")) {
        res = _getString(dispTxt, sizeof(dispTxt), elementName);    
    }
    // Copy the received text int user value
    if (EmGetValueResult::failed != res) {
        strcpy(txt, dispTxt);
    }
    return res;
}

#endif
