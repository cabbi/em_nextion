#ifndef __NEXTION
#define __NEXTION

#include <stdint.h>

#include "em_defs.h"
#include "em_log.h"
#include "em_com_device.h"
#include "em_sync_value.h"

// Nextion defined result codes
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

template<EmNexPage& page>
class EmNexPageElement: public EmNexObject
{
public:
    EmNexPageElement(const char* name,
                     EmLogLevel logLevel=EmLogLevel::none)
     : EmNexObject(name, logLevel) {}

    EmNextion& Nex() const {
        return page.Nex();
    }

    EmNexPage& Page() const {
        return page;
    }

    const char* PageName() const {
        return page.Name();
    }

    // Set element visibility.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. visibility attribute is reset if page is changed 
    //     or display recovers from screen saver
    bool SetVisible(bool visible) const {
        return Nex().SetVisible(page.Id(), m_name, visible);
    }

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool Click(bool pressed = true) const {
        return Nex().Click(page.Id(), m_name, pressed);
    }
};

template<EmNexPage& page>
class EmNexPicture: public EmNexPageElement<page>
{
public:
    EmNexPicture(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement<page>(name, logLevel){}

    // Set element picture (only for picture objects).
    bool SetPicture(uint8_t picId) const {
        return this->Nex().SetPicture(page.Name(), this->m_name, picId);
    }

    // Get element picture (only for picture objects).
    bool GetPicture(uint8_t& picId) const {
        return this->Nex().GetPicture(page.Name(), this->m_name, picId);
    }
};

template<EmNexPage& page>
class EmNexColoredElement: public EmNexPageElement<page>
{
public:
    EmNexColoredElement(const char* name,
                        EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement<page>(name, logLevel){}


    // Set background color.
    bool SetBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return this->Nex().SetBkColor(page.Name(), 
                                      this->m_name, 
                                      ToColor565(red, green, blue));
    }

    bool SetBkColor(uint16_t color565) const {
        return this->Nex().SetBkColor(page.Name(), 
                                      this->m_name, 
                                      color565);
    }

    // Get background color.
    bool GetBkColor(uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        return this->Nex().GetBkColor(page.Name(), 
                                      this->m_name, 
                                      red, green, blue);
    }

    bool GetBkColor(uint16_t& color565) const {
        return this->Nex().GetBkColor(page.Name(), 
                                      this->m_name, 
                                      color565);
    }

    // Set font color.
    bool SetFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return this->Nex().SetFontColor(page.Name(), 
                                        this->m_name, 
                                        red, green, blue);
    }

    bool SetFontColor(uint16_t color565) const {
        return this->Nex().SetFontColor(page.Name(), 
                                        this->m_name, 
                                        color565);
    }

    // Get font color.
    bool GetFontColor(uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        return this->Nex().GetFontColor(page.Name(), 
                                        this->m_name, 
                                        red, green, blue);
    }

    bool GetFontColor(uint16_t& color565) const {
        return this->Nex().GetFontColor(page.Name(), 
                                        this->m_name, 
                                        color565);
    }

};

template<EmNexPage& page>
class EmNexText: public EmNexColoredElement<page>
{
public:
    EmNexText(const char* name,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<page>(name, logLevel) {}

    template<size_t len>
    EmGetValueResult GetValue(char* value) const {
        return this->Nex().GetTextElementValue<len>(this->PageName(), this->m_name, value);
    }

    bool SetValue(const char* value) const {
        return this->Nex().SetTextElementValue(this->PageName(), this->m_name, value);
    }

    template <uint16_t max_len>
    bool SetValue(const char* format, ...) const {
        char text[max_len+1];
        va_list args;
        va_start(args, format);     
        vsnprintf(text, max_len+1, format, args);
        va_end(args);
        return this->Nex().SetTextElementValue(this->PageName(), this->m_name, text);
    }
};

// Use 'EmNexTextEx' class if you need an 'EmValue' object 
template<EmNexPage& page>
class EmNexTextEx: public EmNexText<page>,
                   public EmValue<char*>
{
public:
     EmNexTextEx(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexText<page>(name, logLevel), 
       EmValue<char*>() {}

    virtual EmGetValueResult GetValue(char* value) const override {
        // NOTE:
        //  Since 'GetValue' overrides a virtual method it can not 
        //  be template based. 100 should be a good compromise.
        //  To use exact len please use the templated 'getValue' method. 
        return EmNexText<page>::GetValue<100>(value);
    }

    virtual bool SetValue(const char* value) override {
        return EmNexText<page>::SetValue(value);
    }
};

template<EmNexPage& page>
class EmNexInteger: public EmNexColoredElement<page>
{
public:
    EmNexInteger(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<page>(name, logLevel) {}

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

    EmGetValueResult GetValue(int32_t& value) const {
        return this->Nex().GetNumElementValue(this->PageName(), 
                                              this->m_name, 
                                              value);
    }

    bool SetValue(int32_t const value) const {
        return this->Nex().SetNumElementValue(this->PageName(), 
                                              this->m_name, 
                                              value);
    }
};

// Use 'EmNexIntegerEx' class if you need an 'EmValue' object 
template<EmNexPage& page>
class EmNexIntegerEx: public EmNexInteger<page>,
                      public EmValue<int32_t>
{
public:
    EmNexIntegerEx(const char* name,
                   EmLogLevel logLevel=EmLogLevel::none)
     : EmNexInteger<page>(name, logLevel),
       EmValue<int32_t>() {}

    virtual EmGetValueResult GetValue(int32_t& value) const override {
        return EmNexInteger<page>::GetValue(value);
    }

    virtual bool SetValue(int32_t const value) override {
        return EmNexInteger<page>::SetValue(value);
    }
};

template<EmNexPage& page>
class EmNexReal: public EmNexColoredElement<page>
{
public:
    EmNexReal(const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<page>(name, logLevel),
       m_decPlaces(decPlaces) {}

    // Templated methods (not virtual)
    template <class real_type>
    EmGetValueResult GetValue(real_type& value) const {
        int32_t val = iMolt<real_type>(value, iPow10(m_decPlaces));
        EmGetValueResult res = this->Nex().GetNumElementValue(this->Page().Name(), 
                                                              this->m_name, 
                                                              val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<real_type>(val)/pow(10, m_decPlaces);
        }
        return res;
    }
 
    template <class real_type>
    bool SetValue(real_type const value) {
        return this->Nex().SetNumElementValue(this->PageName(), 
                                             this->m_name, 
                                             iRound<real_type>(value*iPow10(m_decPlaces)));
    }

    EmGetValueResult GetValue(double& value) const {
        return GetValue<double>(value);
    }

    bool SetValue(double const value) const {
        return SetValue<double>(value);
    }

protected:
    const uint8_t m_decPlaces;
};

// Use 'EmNexRealEx' class if you need an 'EmValue' object 
template<EmNexPage& page>
class EmNexRealEx: public EmNexReal<page>,
                   public EmValue<double>
{
public:
    EmNexRealEx(const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexReal<page>(name, logLevel),
       EmValue<double>() {}

    virtual EmGetValueResult GetValue(double& value) const override {
        return EmNexReal<page>::GetValue(value);
    }

    virtual bool SetValue(double const value) override {
        return EmNexReal<page>::SetValue(value);
    }
};

// A two labels number
template<EmNexPage& page>
class EmNexDecimal: public EmNexColoredElement<page>
{
public:
    EmNexDecimal(const char* intElementName,
                 const char* decElementName,
                 uint8_t decPlaces,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<page>(intElementName, logLevel),
       m_decElementName(decElementName),
       m_decPlaces(decPlaces) {}

    bool SetValue(double const value) {
        int32_t exp = iPow10(this->m_decPlaces);
        int32_t dispValue = iRound(value*static_cast<double>(exp));
        return this->Nex().SetNumElementValue(this->Page().Name(), 
                                              this->m_name, 
                                              iDiv(dispValue, exp)) &&
               this->Nex().SetNumElementValue(this->Page().Name(), 
                                              this->m_decElementName, 
                                              dispValue % exp);        
    }

    EmGetValueResult GetValue(float& value) const {
        double val;
        EmGetValueResult res = this->GetValue(val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<float>(val);
        }
        return res;
    }

    EmGetValueResult GetValue(double& value) const { 
        double prevValue = value;
        EmGetValueResult res;
        int32_t intVal;
        res = this->Nex().GetNumElementValue(this->Page().Name(), 
                                             this->m_name, 
                                             intVal);
        if (res == EmGetValueResult::failed) {
            return EmGetValueResult::failed;
        }
        int32_t decVal;
        res = this->Nex().GetNumElementValue(this->Page().Name(), 
                                             m_decElementName, 
                                             decVal);
        if (res == EmGetValueResult::failed) {
            return EmGetValueResult::failed;
        }
        value = intVal+(static_cast<double>(decVal)/pow(10, m_decPlaces));

        return prevValue == value ? 
            EmGetValueResult::succeedEqualValue :
            EmGetValueResult::succeedNotEqualValue;
    }


    // Set background color.
    bool SetBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return this->Nex().SetBkColor(page.Name(), 
                                      this-> m_name, 
                                      ToColor565(red, green, blue)) &&
               this->Nex().SetBkColor(page.Name(), 
                                      m_decElementName, 
                                      ToColor565(red, green, blue));
    }

    bool SetBkColor(uint16_t color565) const {
        return this->Nex().SetBkColor(page.Name(), 
                                      this->m_name, 
                                      color565) &&
               this->Nex().SetBkColor(page.Name(), 
                                      m_decElementName, 
                                      color565);
    }

    // Set font color.
    bool SetFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return this->Nex().SetFontColor(page.Name(), 
                                        this->m_name, 
                                        red, green, blue) &&
               this->Nex().SetFontColor(page.Name(), 
                                        m_decElementName, 
                                        red, green, blue);
    }

    bool SetFontColor(uint16_t color565) const {
        return this->Nex().SetFontColor(page.Name(), 
                                        this->m_name, 
                                        color565) &&
               this->Nex().SetFontColor(page.Name(), 
                                        m_decElementName, 
                                        color565);
    }

protected:
    const char* m_decElementName;
    const uint8_t m_decPlaces;
};

// Use 'EmNexDecimalEx' class if you need an 'EmValue' object 
template<EmNexPage& page>
class EmNexDecimalEx: public EmNexDecimal<page>,
                      public EmValue<double>
{
public:
    EmNexDecimalEx(const char* intElementName,
                   const char* decElementName,
                   uint8_t decPlaces,
                   EmLogLevel logLevel=EmLogLevel::none)
     : EmNexDecimal<page>(intElementName, logLevel),
       EmValue<double>() {}

    virtual bool SetValue(double const value) override {
        return EmNexDecimal<page>::SetValue(value);
    }

    virtual EmGetValueResult GetValue(double& value) const override { 
        return EmNexDecimal<page>::GetValue(value);
    }
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
