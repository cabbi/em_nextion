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

// Color Code Constants
enum EmNexColor: uint16_t {
    BLACK = 0,
    BLUE = 31,
    BROWN = 48192,
    GREEN = 2016,
    YELLOW = 65504,
    RED = 63488,
    GRAY = 33840,
    WHITE = 65535
};

// Color conversion methods 
// Note:
//	 16-bit 565 Colors are in decimal values from 0 to 65535
//   Example:
//     24-bit RGB 11011000 11011000 11011000
//     16-bit 565 11011 +  110110 + 11011
inline uint16_t ToColor565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red>>3) << 11) | ((green>>2) << 5) | (blue >> 3);
}

inline void FromColor565(uint16_t color565, uint8_t& red, uint8_t& green, uint8_t& blue) {
    red = (color565 & 0xF800) >> 8;    // rrrrr... ........ -> rrrrr000
    green = (color565 & 0x07E0) >> 3;  // .....ggg ggg..... -> gggggg00
    blue = (color565 & 0x1F) << 3;     // ............bbbbb -> bbbbb000
}

// The main nextion display handling class
class EmNextion: public EmLog {
public:
    EmNextion(EmComSerial& serial, 
              uint32_t timeoutMs, 
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

    // Set element picture (only for picture objects).
    bool SetPicture(const char* pageName, 
                    const char* elementName, 
                    uint8_t picId) const;

    // Get element picture (only for picture objects).
    bool GetPicture(const char* pageName, 
                    const char* elementName, 
                    uint8_t& picId) const;

    // Set background color.
    bool SetBkColor(const char* pageName, 
                    const char* elementName, 
                    uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return SetBkColor(pageName, 
                          elementName, 
                          ToColor565(red, green, blue));
    }

    bool SetBkColor(const char* pageName, 
                    const char* elementName, 
                    uint16_t color565) const {
        return _setColor(pageName, elementName, "bco", color565);
    }

    // Get background color.
    bool GetBkColor(const char* pageName, 
                    const char* elementName, 
                    uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        uint16_t c565;
        if (!GetBkColor(pageName, elementName, c565)) {
            return false;
        }
        FromColor565(c565, red, green, blue);
        return true;
    }

    bool GetBkColor(const char* pageName, 
                    const char* elementName, 
                    uint16_t& color565) const {
        return _getColor(pageName, elementName, "bco", color565);
    }

    // Set font color.
    bool SetFontColor(const char* pageName, 
                      const char* elementName, 
                      uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return SetFontColor(pageName, 
                            elementName, 
                            ToColor565(red, green, blue));
    }

    bool SetFontColor(const char* pageName, 
                      const char* elementName, 
                      uint16_t color565) const {
        return _setColor(pageName, elementName, "pco", color565);
    }

    // Get font color.
    bool GetFontColor(const char* pageName, 
                      const char* elementName, 
                      uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        uint16_t c565;
        if (!GetFontColor(pageName, elementName, c565)) {
            return false;
        }
        FromColor565(c565, red, green, blue);
        return true;
    }

    bool GetFontColor(const char* pageName, 
                      const char* elementName, 
                      uint16_t& color565) const {
        return _getColor(pageName, elementName, "pco", color565);
    }

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool Click(const char* elementName, 
               bool pressed = true) const;

    bool Click(uint8_t pageId, 
               const char* elementName, 
               bool pressed = true) const;               

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
    bool _bResult(bool result) const;

    bool _setColor(const char* pageName, 
                   const char* elementName, 
                   const char* colorCode, 
                   uint16_t color565) const;

    bool _getColor(const char* pageName, 
                   const char* elementName, 
                   const char* colorCode, 
                   uint16_t& color565) const;
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

    bool SetAsCurrent() const {
        return Nex().SetCurPage(m_id);
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

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool Click(bool pressed = true) const {
        return Nex().Click(m_page.Id(), m_name, pressed);
    }

protected:
    EmNexPage& m_page;
};

class EmNexPicture: public EmNexPageElement
{
public:
    EmNexPicture(EmNexPage& page,
                 const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, name, logLevel){}

    // Set element picture (only for picture objects).
    bool SetPicture(uint8_t picId) const {
        return Nex().SetPicture(m_page.Name(), m_name, picId);
    }

    // Get element picture (only for picture objects).
    bool GetPicture(uint8_t& picId) const {
        return Nex().GetPicture(m_page.Name(), m_name, picId);
    }
};

class EmNexColoredElement: public EmNexPageElement
{
public:
    EmNexColoredElement(EmNexPage& page,
                        const char* name,
                        EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement(page, name, logLevel){}


    // Set background color.
    bool SetBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return Nex().SetBkColor(m_page.Name(), 
                                m_name, 
                                ToColor565(red, green, blue));
    }

    bool SetBkColor(uint16_t color565) const {
        return Nex().SetBkColor(m_page.Name(), 
                                m_name, 
                                color565);
    }

    // Get background color.
    bool GetBkColor(uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        return Nex().GetBkColor(m_page.Name(), 
                                m_name, 
                                red, green, blue);
    }

    bool GetBkColor(uint16_t& color565) const {
        return Nex().GetBkColor(m_page.Name(), 
                                m_name, 
                                color565);
    }

    // Set font color.
    bool SetFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return Nex().SetFontColor(m_page.Name(), 
                                  m_name, 
                                  red, green, blue);
    }

    bool SetFontColor(uint16_t color565) const {
        return Nex().SetFontColor(m_page.Name(), 
                                  m_name, 
                                  color565);
    }

    // Get font color.
    bool GetFontColor(uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        return Nex().GetFontColor(m_page.Name(), 
                                  m_name, 
                                  red, green, blue);
    }

    bool GetFontColor(uint16_t& color565) const {
        return Nex().GetFontColor(m_page.Name(), 
                                  m_name, 
                                  color565);
    }

};

class EmNexText: public EmNexColoredElement,
                 public EmValue<char*>
{
public:
    EmNexText(EmNexPage& page,
              const char* name,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement(page, name, logLevel),
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

class EmNexInteger: public EmNexColoredElement,
                    public EmValue<int32_t>
{
public:
    EmNexInteger(EmNexPage& page,
                 const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement(page, name, logLevel) {}

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

class EmNexReal: public EmNexColoredElement,
                 public EmValue<double>
{
public:
    EmNexReal(EmNexPage& page,
              const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement(page, name, logLevel),
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
class EmNexDecimal:public EmNexColoredElement,
                   public EmValue<double>
{
public:
    EmNexDecimal(EmNexPage& page,
                 const char* intElementName,
                 const char* decElementName,
                 uint8_t decPlaces,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement(page, intElementName, logLevel),
       m_decElementName(decElementName),
       m_decPlaces(decPlaces) {}

    virtual EmGetValueResult GetValue(float& value) const;
    virtual EmGetValueResult GetValue(double& value) const override;
    virtual bool SetValue(double const value) override;

    // Set background color.
    bool SetBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return Nex().SetBkColor(m_page.Name(), 
                                m_name, 
                                ToColor565(red, green, blue)) &&
               Nex().SetBkColor(m_page.Name(), 
                                m_decElementName, 
                                ToColor565(red, green, blue));
    }

    bool SetBkColor(uint16_t color565) const {
        return Nex().SetBkColor(m_page.Name(), 
                                m_name, 
                                color565) &&
               Nex().SetBkColor(m_page.Name(), 
                                m_decElementName, 
                                color565);
    }

    // Set font color.
    bool SetFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return Nex().SetFontColor(m_page.Name(), 
                                  m_name, 
                                  red, green, blue) &&
               Nex().SetFontColor(m_page.Name(), 
                                  m_decElementName, 
                                  red, green, blue);
    }

    bool SetFontColor(uint16_t color565) const {
        return Nex().SetFontColor(m_page.Name(), 
                                  m_name, 
                                  color565) &&
               Nex().SetFontColor(m_page.Name(), 
                                  m_decElementName, 
                                  color565);
    }

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
