#ifndef __EM_NEXTION__
#define __EM_NEXTION__

#include <stdint.h>

#include "em_defs.h"
#include "em_log.h"
#include "em_optional.h"
#include "em_com_device.h"
#include "em_sync_value.h"
#include "em_tag.h"

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
inline uint16_t toColor565(uint8_t red, uint8_t green, uint8_t blue) {
    return ((red>>3) << 11) | ((green>>2) << 5) | (blue >> 3);
}

inline void fromColor565(uint16_t color565, uint8_t& red, uint8_t& green, uint8_t& blue) {
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

    bool begin() const;

    bool isInit() const { 
        return m_isInit;
    }

    bool isCurPage(uint8_t pageId) const;
    bool getCurPage(uint8_t& pageId) const;
    bool setCurPage(uint8_t pageId) const;
    bool setCurPage(const char* pageName) const;

    EmGetValueResult getNumElementValue(const char* pageName, 
                                        const char* elementName, 
                                        int32_t& val) const;
    template<size_t len>
    EmGetValueResult getTextElementValue(const char* pageName, 
                                         const char* elementName, 
                                         char* txt) const;

    bool setNumElementValue(const char* pageName, 
                            const char* elementName, 
                            int32_t val) const;
    bool setTextElementValue(const char* pageName, 
                             const char* elementName, 
                             const char* txt) const;

    // Set element visibility.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. visibility attribute is reset if page is changed 
    //     or display recovers from screen saver
    bool setVisible(const char* elementName, 
                    bool visible) const;

    bool setVisible(uint8_t pageId, 
                    const char* elementName, 
                    bool visible) const;

    // Set element picture (only for picture objects).
    bool setPicture(const char* pageName, 
                    const char* elementName, 
                    uint8_t picId) const;

    // Get element picture (only for picture objects).
    bool getPicture(const char* pageName, 
                    const char* elementName, 
                    uint8_t& picId) const;

    // Set background color.
    bool setBkColor(const char* pageName, 
                    const char* elementName, 
                    uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return setBkColor(pageName, 
                          elementName, 
                          toColor565(red, green, blue));
    }

    bool setBkColor(const char* pageName, 
                    const char* elementName, 
                    uint16_t color565) const {
        return setColor_(pageName, elementName, "bco", color565);
    }

    // Get background color.
    bool getBkColor(const char* pageName, 
                    const char* elementName, 
                    uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        uint16_t c565;
        if (!getBkColor(pageName, elementName, c565)) {
            return false;
        }
        fromColor565(c565, red, green, blue);
        return true;
    }

    bool getBkColor(const char* pageName, 
                    const char* elementName, 
                    uint16_t& color565) const {
        return getColor_(pageName, elementName, "bco", color565);
    }

    // Set font color.
    bool setFontColor(const char* pageName, 
                      const char* elementName, 
                      uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return setFontColor(pageName, 
                            elementName, 
                            toColor565(red, green, blue));
    }

    bool setFontColor(const char* pageName, 
                      const char* elementName, 
                      uint16_t color565) const {
        return setColor_(pageName, elementName, "pco", color565);
    }

    // Get font color.
    bool getFontColor(const char* pageName, 
                      const char* elementName, 
                      uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        uint16_t c565;
        if (!getFontColor(pageName, elementName, c565)) {
            return false;
        }
        fromColor565(c565, red, green, blue);
        return true;
    }

    bool getFontColor(const char* pageName, 
                      const char* elementName, 
                      uint16_t& color565) const {
        return getColor_(pageName, elementName, "pco", color565);
    }

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool click(const char* elementName, 
               bool pressed = true) const;

    bool click(uint8_t pageId, 
               const char* elementName, 
               bool pressed = true) const;               

protected:
    bool sendGetCmd_(const char* pageName, 
                     const char* elementName, 
                     const char* property) const;
    bool sendSetCmd_(const char* pageName, 
                     const char* elementName, 
                     const char* property, 
                     int32_t value) const;
    bool sendSetCmd_(const char* pageName, 
                     const char* elementName, 
                     const char* property, 
                     const char* value) const;
    EmGetValueResult getNumber_(int32_t& val) const;
    EmGetValueResult getString_(char* txt, 
                                uint8_t bufLen, 
                                const char* elementName) const;


    bool sendCmd_(const char* firstCmd, ...) const;
    bool sendCmdParam_(const char* cmdParam) const;
    bool sendCmdEnd_() const;
    bool ack_(uint8_t ackCode) const;
    EmGetValueResult recv_(uint8_t ackCode, 
                           char* buf, 
                           uint8_t len, 
                           bool isText=false) const;
    EmGetValueResult result_(bool result, bool valueChanged) const;
    bool bResult_(bool result) const;

    bool setColor_(const char* pageName, 
                   const char* elementName, 
                   const char* colorCode, 
                   uint16_t color565) const;

    bool getColor_(const char* pageName, 
                   const char* elementName, 
                   const char* colorCode, 
                   uint16_t& color565) const;
private:
    EmComSerial& m_serial;       
    const uint32_t m_timeoutMs;
    mutable bool m_isInit;
};

class EmNexObject: public EmLog {
public:
    EmNexObject(const char* name,
                EmLogLevel logLevel=EmLogLevel::none)
     : EmLog("NexObj", logLevel),
       m_name(name) {}

    const char* name() const { return m_name; }

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
    
    EmNextion& nex() const {
        return m_nex;
    }

    uint8_t id() const {
        return m_id;
    }

    bool isCurrent() const {
        return nex().isCurPage(m_id);
    }

    bool setAsCurrent() const {
        return nex().setCurPage(m_id);
    }

protected:
    EmNextion& m_nex;
    const uint8_t m_id;
};

template<EmNexPage& tPage>
class EmNexPageElement: public EmNexObject
{
public:
    EmNexPageElement(const char* name,
                     EmLogLevel logLevel=EmLogLevel::none)
     : EmNexObject(name, logLevel) {}

    EmNextion& nex() const {
        return tPage.nex();
    }

    EmNexPage& page() const {
        return tPage;
    }

    const char* pageName() const {
        return tPage.name();
    }

    // Set element visibility.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. visibility attribute is reset if page is changed 
    //     or display recovers from screen saver
    bool setVisible(bool visible) const {
        return nex().setVisible(tPage.id(), m_name, visible);
    }

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool click(bool pressed = true) const {
        return nex().click(tPage.id(), m_name, pressed);
    }
};

template<EmNexPage& tPage>
class EmNexPicture: public EmNexPageElement<tPage>
{
public:
    EmNexPicture(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement<tPage>(name, logLevel){}

    // Set element picture (only for picture objects).
    bool setPicture(uint8_t picId) const {
        return this->nex().SetPicture(tPage.name(), this->m_name, picId);
    }

    // Get element picture (only for picture objects).
    bool getPicture(uint8_t& picId) const {
        return this->nex().GetPicture(tPage.name(), this->m_name, picId);
    }
};

template<EmNexPage& tPage>
class EmNexColoredElement: public EmNexPageElement<tPage>
{
public:
    EmNexColoredElement(const char* name,
                        EmLogLevel logLevel=EmLogLevel::none)
     : EmNexPageElement<tPage>(name, logLevel){}


    // Set background color.
    bool setBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->m_name, 
                                      toColor565(red, green, blue));
    }

    bool setBkColor(uint16_t color565) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->m_name, 
                                      color565);
    }

    // Get background color.
    bool getBkColor(uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        return this->nex().getBkColor(tPage.name(), 
                                      this->m_name, 
                                      red, green, blue);
    }

    bool getBkColor(uint16_t& color565) const {
        return this->nex().getBkColor(tPage.name(), 
                                      this->m_name, 
                                      color565);
    }

    // Set font color.
    bool setFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->m_name, 
                                        red, green, blue);
    }

    bool setFontColor(uint16_t color565) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->m_name, 
                                        color565);
    }

    // Get font color.
    bool getFontColor(uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        return this->nex().getFontColor(tPage.name(), 
                                        this->m_name, 
                                        red, green, blue);
    }

    bool getFontColor(uint16_t& color565) const {
        return this->nex().getFontColor(tPage.name(), 
                                        this->m_name, 
                                        color565);
    }

};

template<EmNexPage& tPage>
class EmNexText: public EmNexColoredElement<tPage>
{
public:
    EmNexText(const char* name,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<tPage>(name, logLevel) {}

    template<size_t len>
    EmGetValueResult getValue(char* value) const {
        return this->nex().GetTextElementValue<len>(this->pageName(), this->m_name, value);
    }

    bool setValue(const char* value) const {
        return this->nex().setTextElementValue(this->pageName(), this->m_name, value);
    }

    template <uint16_t max_len>
    bool setValue(const char* format, ...) const {
        char text[max_len+1];
        va_list args;
        va_start(args, format);     
        vsnprintf(text, max_len+1, format, args);
        va_end(args);
        return this->nex().setTextElementValue(this->pageName(), this->m_name, text);
    }
};

// Use 'EmNexTextEx' class if you need an 'EmValue' object 
template<EmNexPage& tPage>
class EmNexTextEx: public EmNexText<tPage>,
                   public EmValue<char*>
{
public:
     EmNexTextEx(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexText<tPage>(name, logLevel), 
       EmValue<char*>() {}

    virtual EmGetValueResult getValue(char* value) const override {
        // NOTE:
        //  Since 'getValue' overrides a virtual method it can not 
        //  be template based. 100 should be a good compromise.
        //  To use exact len please use the templated non virtual 'getValue' method. 
        return EmNexText<tPage>::getValue<100>(value);
    }

    virtual bool setValue(const char* value) override {
        return EmNexText<tPage>::setValue(value);
    }
};

// Use 'EmNexTextTag' class if you need an 'EmTagValue' object 
template<EmNexPage& tPage>
class EmNexTextTag: public EmTagBase,
                    public EmNexText<tPage> {   
public:
     EmNexTextTag(const char* name,
                  EmSyncFlags flags,
                  EmLogLevel logLevel=EmLogLevel::none)
     : EmTagBase(flags),
       EmNexText<tPage>(name, logLevel) {} 

    virtual const char* getId() const override {
        return this->m_name;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        // NOTE:
        //  Since 'getValue' overrides a virtual method it can not 
        //  be template based. 100 should be a good compromise.
        //  To use exact len please use the templated non virtual 'getValue' method. 
        const char* val[101];
        EmGetValueResult res = EmNexText<tPage>::getValue<100>(val);
        if (EmGetValueResult::failed != res) {
            if (!value.setValue(val, true)) {
                return EmGetValueResult::failed;
            }
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        if (value.getType() != EmTagValueType::vt_string) {
            return false;
        }
        EmTagValueStruct out;
        value.toStruct(out);
        return EmNexText<tPage>::setValue(out.m_value.as_string->c_str());
    }};

template<EmNexPage& tPage>
class EmNexInteger: public EmNexColoredElement<tPage>
{
public:
    EmNexInteger(const char* name,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<tPage>(name, logLevel) {}

    // Templated methods (not virtual)
    template <class int_type>
    EmGetValueResult getValue(int_type& value) const {
        int32_t val = static_cast<int32_t>(value);
        EmGetValueResult res = getValue(val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<int_type>(val);
        }
        return res;
    }

    EmGetValueResult getValue(int32_t& value) const {
        return this->nex().getNumElementValue(this->pageName(), 
                                              this->m_name, 
                                              value);
    }

    bool setValue(const int32_t value) const {
        return this->nex().setNumElementValue(this->pageName(), 
                                              this->m_name, 
                                              value);
    }
};

// Use 'EmNexIntegerEx' class if you need an 'EmValue' object 
template<EmNexPage& tPage>
class EmNexIntegerEx: public EmNexInteger<tPage>,
                      public EmValue<int32_t>
{
public:
    EmNexIntegerEx(const char* name,
                   EmLogLevel logLevel=EmLogLevel::none)
     : EmNexInteger<tPage>(name, logLevel),
       EmValue<int32_t>() {}

    virtual EmGetValueResult getValue(int32_t& value) const override {
        return EmNexInteger<tPage>::getValue(value);
    }

    virtual bool setValue(const int32_t value) override {
        return EmNexInteger<tPage>::setValue(value);
    }
};

// Use 'EmNexIntegerTag' class if you need an 'EmTagValue' object 
template<EmNexPage& tPage>
class EmNexIntegerTag: public EmTagBase,
                       public EmNexInteger<tPage> {
public:
    EmNexIntegerTag(const char* name,
                    EmSyncFlags flags,
                    EmLogLevel logLevel=EmLogLevel::none)
     : EmTagBase(flags),
       EmNexInteger<tPage>(name, logLevel) {}
    
    virtual const char* getId() const override {
        return this->m_name;
    }

    virtual EmTagValue getValue() const {
        EmTagValue val(EmTagValueType::vt_integer);
        this->getValue(val);    
        return val;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        int32_t val;
        EmGetValueResult res = EmNexInteger<tPage>::getValue(val);
        if (EmGetValueResult::failed != res) {
            if (!value.setValue(val, true)) {
                return EmGetValueResult::failed;
            }
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        if (value.getType() != EmTagValueType::vt_integer) {
            return false;
        }
        EmTagValueStruct out;
        value.toStruct(out);
        return EmNexInteger<tPage>::setValue(out.m_value.as_integer);
    }
};

template<EmNexPage& tPage>
class EmNexReal: public EmNexColoredElement<tPage>
{
public:
    EmNexReal(const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<tPage>(name, logLevel),
       m_decPlaces(decPlaces) {}

    // Templated methods (not virtual)
    template <class real_type>
    EmGetValueResult getValue(real_type& value) const {
        int32_t val = iMolt<real_type>(value, iPow10(m_decPlaces));
        EmGetValueResult res = this->nex().getNumElementValue(this->pageName(), 
                                                              this->m_name, 
                                                              val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<real_type>(val)/pow(10, m_decPlaces);
        }
        return res;
    }
 
    template <class real_type>
    bool setValue(real_type const value) {
        return this->nex().setNumElementValue(this->pageName(), 
                                             this->m_name, 
                                             iRound<real_type>(value*iPow10(m_decPlaces)));
    }

    EmGetValueResult getValue(double& value) const {
        return getValue<double>(value);
    }

    bool setValue(double value) const {
        return setValue<double>(value);
    }

protected:
    const uint8_t m_decPlaces;
};

// Use 'EmNexRealEx' class if you need an 'EmValue' object 
template<EmNexPage& tPage>
class EmNexRealEx: public EmNexReal<tPage>,
                   public EmValue<double>
{
public:
    EmNexRealEx(const char* name,
                uint8_t decPlaces,
                EmLogLevel logLevel=EmLogLevel::none)
     : EmNexReal<tPage>(name, logLevel),
       EmValue<double>() {}

    virtual EmGetValueResult getValue(double& value) const override {
        return EmNexReal<tPage>::getValue(value);
    }

    virtual bool setValue(const double& value) override {
        return EmNexReal<tPage>::setValue(value);
    }
};

// Use 'EmNexRealTag' class if you need an 'EmTagValue' object 
template<EmNexPage& tPage>
class EmNexRealTag: public EmTagBase,
                    public EmNexReal<tPage>{
public:
    EmNexRealTag(const char* name,
                 uint8_t decPlaces,
                 EmSyncFlags flags,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmTagBase(flags),
       EmNexReal<tPage>(name, decPlaces, logLevel) {}
    
    virtual const char* getId() const override {
        return this->m_name;
    }

    virtual EmTagValue getValue() const {
        EmTagValue val(EmTagValueType::vt_real);
        this->getValue(val);    
        return val;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        double val;
        EmGetValueResult res = EmNexReal<tPage>::getValue(val);
        if (EmGetValueResult::failed != res) {
            if (!value.setValue(val, true)) {
                return EmGetValueResult::failed;
            }
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        if (value.getType() != EmTagValueType::vt_real) {
            return false;
        }
        EmTagValueStruct out;
        value.toStruct(out);
        return EmNexReal<tPage>::setValue(out.m_value.as_real);
    }
};

// A two labels number
template<EmNexPage& tPage>
class EmNexDecimal: public EmNexColoredElement<tPage>
{
public:
    EmNexDecimal(const char* intElementName,
                 const char* decElementName,
                 uint8_t decPlaces,
                 EmLogLevel logLevel=EmLogLevel::none)
     : EmNexColoredElement<tPage>(intElementName, logLevel),
       m_decElementName(decElementName),
       m_decPlaces(decPlaces) {}

    bool setValue(double value) {
        int32_t exp = iPow10(this->m_decPlaces);
        int32_t dispValue = iRound(value*static_cast<double>(exp));
        return this->nex().setNumElementValue(this->pageName(), 
                                              this->m_name, 
                                              iDiv(dispValue, exp)) &&
               this->nex().setNumElementValue(this->pageName(), 
                                              this->m_decElementName, 
                                              dispValue % exp);        
    }

    EmGetValueResult getValue(float& value) const {
        double val;
        EmGetValueResult res = this->getValue(val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<float>(val);
        }
        return res;
    }

    EmGetValueResult getValue(double& value) const { 
        double prevValue = value;
        EmGetValueResult res;
        int32_t intVal;
        res = this->nex().getNumElementValue(this->pageName(), 
                                             this->m_name, 
                                             intVal);
        if (res == EmGetValueResult::failed) {
            return EmGetValueResult::failed;
        }
        int32_t decVal;
        res = this->nex().getNumElementValue(this->pageName(), 
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
    bool setBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this-> m_name, 
                                      toColor565(red, green, blue)) &&
               this->nex().setBkColor(tPage.name(), 
                                      m_decElementName, 
                                      toColor565(red, green, blue));
    }

    bool setBkColor(uint16_t color565) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->m_name, 
                                      color565) &&
               this->nex().setBkColor(tPage.name(), 
                                      m_decElementName, 
                                      color565);
    }

    // Set font color.
    bool setFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->m_name, 
                                        red, green, blue) &&
               this->nex().setFontColor(tPage.name(), 
                                        m_decElementName, 
                                        red, green, blue);
    }

    bool setFontColor(uint16_t color565) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->m_name, 
                                        color565) &&
               this->nex().setFontColor(tPage.name(), 
                                        m_decElementName, 
                                        color565);
    }

protected:
    const char* m_decElementName;
    const uint8_t m_decPlaces;
};

// Use 'EmNexDecimalEx' class if you need an 'EmValue' object 
template<EmNexPage& tPage>
class EmNexDecimalEx: public EmNexDecimal<tPage>,
                      public EmValue<double>
{
public:
    EmNexDecimalEx(const char* intElementName,
                   const char* decElementName,
                   uint8_t decPlaces,
                   EmLogLevel logLevel=EmLogLevel::none)
     : EmNexDecimal<tPage>(intElementName, logLevel),
       EmValue<double>() {}

    virtual bool setValue(const double& value) override {
        return EmNexDecimal<tPage>::setValue(value);
    }

    virtual EmGetValueResult getValue(double& value) const override { 
        return EmNexDecimal<tPage>::getValue(value);
    }
};

// Use 'EmNexDecimalTag' class if you need an 'EmTagValue' object 
template<EmNexPage& tPage>
class EmNexDecimalTag: public EmTagBase,
                       public EmNexDecimal<tPage> {
public:
    EmNexDecimalTag(const char* intElementName,
                   const char* decElementName,
                   uint8_t decPlaces,
                   EmSyncFlags flags,
                   EmLogLevel logLevel=EmLogLevel::none)
     : EmTagBase(flags),
       EmNexDecimal<tPage>(intElementName, decElementName, decPlaces, logLevel) {}
    
    virtual const char* getId() const override {
        return this->m_name;
    }

    virtual EmTagValue getValue() const {
        EmTagValue val(EmTagValueType::vt_real);
        this->getValue(val);    
        return val;
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        double val;
        EmGetValueResult res = EmNexDecimal<tPage>::getValue(val);
        if (EmGetValueResult::failed != res) {
            value.setValue(val, true);
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) override {
        if (value.getType() != EmTagValueType::vt_real) {
            return false;
        }
        EmTagValueStruct out;
        value.toStruct(out);
        return EmNexDecimal<tPage>::setValue(out.m_value.as_real);
    }
};


// Configuration element for integer values
//
// This class is used to handle configuration elements on the Nextion display.
// A configuration element should be initialized first (i.e. value sent to the display element)
// before reading it.
//
// TODO: handle uninitialized elements on startup (i.e. alternatives to 'dispInitialValue' parameter)
//       This should be done in case the display powers off an on while controlle is still running.
//       If this happens configuration values need to be re-initialized to the display.
template<typename T, EmNexPage& tPage>
class EmNexCfgValue: public EmNexInteger<tPage> {
public: 
    EmNexCfgValue(const char* name)
     : EmNexInteger<tPage>(name),
       m_isInitialized(false) {}

    
    // Reads the value if already initialized, or set it.
    //
    // 'minValue' is the optional initial minimum value 
    //  (i.e. if display value is less than minValue, minValue is assigned to 'value').
    // 'maxValue' is the optional initial maximum value 
    //  (i.e. if display value is greater than maxValue, maxValue is assigned to 'value').
    // 'dispInitialValue' is the optional initial value of the display element on power on
    //  (i.e. if the display value is equal to 'dispInitialValue' then this element is considered uninitialized).
    virtual bool updateValue(EmValue<T>& value, 
                             EmOptional<T> minValue = emUndefined,
                             EmOptional<T> maxValue = emUndefined,
                             EmOptional<T> dispInitialValue = emUndefined) {
        // Set or get value ONLY if not on that page!
        if (tPage.isCurrent()) {
            return false;
        }
        // Value already set the first time?
        if (m_isInitialized) {
            // Get value from display
            T dispValue;
            if (getValue_(dispValue)) {
                if (dispInitialValue.hasValue() && dispInitialValue.value() == dispValue) {
                    // Somehow variable got reset (display power off?)
                    m_isInitialized = false;
                } else {
                    if (minValue.hasValue()) {
                        dispValue = MAX(dispValue, minValue.value());
                    }
                    if (maxValue.hasValue()) {
                        dispValue = MIN(dispValue, maxValue.value());
                    }
                    return value.setValue(dispValue);
                }
            }
        }
        // Need to set the value?
        if (!isInitialized()) {
            T dispValue = T();
            if (EmGetValueResult::failed != value.getValue(dispValue)) {
                m_isInitialized = setValue_(dispValue);
            }
            return m_isInitialized;
        } 
        return false;
    }

    // True if the element is initialized (i.e. display value has been set on startup)
    virtual bool isInitialized() const {
        return m_isInitialized;
    }

    // Resets the element to its uninitialized state
    virtual void reset() {
        m_isInitialized = false;
    }

protected:
    virtual bool getValue_(int16_t& value) {
        return EmNexInteger<tPage>::getValue(value) != EmGetValueResult::failed;
    }

    virtual bool getValue_(int32_t& value) {
        return EmNexInteger<tPage>::getValue(value) != EmGetValueResult::failed;
    }

    virtual bool getValue_(EmTagValue& value) {
        EmTagValueStruct out;
        value.toStruct(out);
        if (out.m_type != EmTagValueType::vt_integer) {
            return false;
        }
        return EmNexInteger<tPage>::getValue(out.m_value.as_integer) != EmGetValueResult::failed;
    }

    virtual bool setValue_(int32_t value) {
        return EmNexInteger<tPage>::setValue(value);
    }

    virtual bool setValue_(EmTagValue value) {
        EmTagValueStruct out;
        value.toStruct(out);
        if (out.m_type != EmTagValueType::vt_integer) {
            return false;
        }
        return EmNexInteger<tPage>::setValue(out.m_value.as_integer);
    }

    bool m_isInitialized;   
};

template<typename intT, EmNexPage& tPage>
class EmNexCfgInteger: public EmNexCfgValue<intT, tPage> {
public:
    EmNexCfgInteger(const char* name)
     : EmNexCfgValue<intT, tPage>(name) {}
};

// NOTE: EmTagValue must be of type EmTagValueType::vt_integer
template<EmNexPage& tPage>
class EmNexCfgTag: public EmNexCfgValue<EmTagValue, tPage> {
public:
    EmNexCfgTag(const char* name)
     : EmNexCfgValue<EmTagValue, tPage>(name) {}
};

template<size_t len>
inline EmGetValueResult EmNextion::getTextElementValue(
    const char* pageName, 
    const char* elementName, 
    char* txt) const
{
    // Create a copy in case communication fails
    // (i.e. some bytes might be modified by _recv method!)
    char dispTxt[len+1];
    strncpy(dispTxt, txt, len);
    EmGetValueResult res = EmGetValueResult::failed;
    if (sendGetCmd_(pageName, elementName, "txt")) {
        res = getString_(dispTxt, sizeof(dispTxt), elementName);    
    }
    // Copy the received text int user value
    if (EmGetValueResult::failed != res) {
        strcpy(txt, dispTxt);
    }
    return res;
}

#endif // __EM_NEXTION__
