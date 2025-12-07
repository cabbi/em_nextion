#ifndef __EM_NEXTION__
#define __EM_NEXTION__

#include <stdint.h>

#include "em_defs.h"
#include "em_log.h"
#include "em_serial.h"
#include "em_optional.h"
#include "em_value_sync.h"
#include "em_tag.h"

// NOTE:
// The following classes have no virtual methods. 
// This is to reduce the RAM footprint of each object instance.

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
    red   = (color565 & 0xF800) >> 8; // rrrrr... ........ -> rrrrr000
    green = (color565 & 0x07E0) >> 3; // .....ggg ggg..... -> gggggg00
    blue  = (color565 & 0x001F) << 3; // ............bbbbb -> bbbbb000
}

// The main nextion display handling class.
// 
// The 'serial' object must be an 'EmSerialStream' implementation like 'EmHardwareSerial'.
// The instance 'begin' method will call the 'serial' object begin as well.
class EmNextion: public EmLog {
public:
    EmNextion(EmSerialStream& serial, 
              uint32_t timeoutMs, 
              const char* logContext="Nex", 
              EmLogLevel logLevel=EmLogLevel::global) : 
        EmLog(logContext, logLevel),
        m_serial(serial),
        m_timeoutMs(timeoutMs),
        m_isInit(false) {}

    bool begin(unsigned long baud) const;

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

    template<size_t max_str_len>
    EmGetValueResult getTextElementValue(const char* pageName, 
                                         const char* elementName, 
                                         char* txt) const {
        // Create a copy in case communication fails
        // (i.e. some bytes might be modified by _recv method!)
        char dispTxt[max_str_len+1];
        strncpy(dispTxt, txt, max_str_len);
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
    bool begin_() const;
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
    EmSerialStream& m_serial;       
    const uint32_t m_timeoutMs;
    mutable bool m_isInit;
};

// The base nextion object class
class EmNexObject: public EmLog {
public:
    EmNexObject(const char* name,
                EmLogLevel logLevel=EmLogLevel::global)
     : EmLog(name, logLevel)
    #ifdef EM_NO_LOG
     , m_name(name)
    #endif
    {}

    const char* name() const {
        #ifdef EM_NO_LOG
        return m_name;
        #else
        return getContext();
        #endif
    };

protected:
#ifdef EM_NO_LOG
    const char* m_name;
#endif
};

// The page object of a nextion page.
class EmNexPage: public EmNexObject
{
public:
    EmNexPage(EmNextion& nex,
              const uint8_t id, 
              const char* name,
              EmLogLevel logLevel=EmLogLevel::global)
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

// A general page UI element.
//
// This is the base class for many following objects, each belonging to a nextion page.
// Page templete is to reduce RAM footprint.
template<EmNexPage& tPage>
class EmNexPageElement: public EmNexObject
{
public:
    EmNexPageElement(const char* name,
                     EmLogLevel logLevel=EmLogLevel::global)
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
        return nex().setVisible(tPage.id(), name(), visible);
    }

    // Simulate a 'Click' event.
    //
    // NOTES:
    //  1. element should be in current page
    //  2. if pressed = False a release event is sent
    bool click(bool pressed = true) const {
        return nex().click(tPage.id(), name(), pressed);
    }
};

// An picture displayed on an 'Picture' nextion object.
//
// Use the 'setPicture' and 'getPicture' methods to set and get the nextion picture id.
template<EmNexPage& tPage>
class EmNexPicture: public EmNexPageElement<tPage>
{
public:
    EmNexPicture(const char* name,
                 EmLogLevel logLevel=EmLogLevel::global)
     : EmNexPageElement<tPage>(name, logLevel){}

    // Set element picture (only for picture objects).
    bool setPicture(uint8_t picId) const {
        return this->nex().setPicture(tPage.name(), this->name(), picId);
    }

    // Get element picture (only for picture objects).
    bool getPicture(uint8_t& picId) const {
        return this->nex().getPicture(tPage.name(), this->name(), picId);
    }
};

// A general colored item. 
//
// This is the base class for many other nextion objects having font and background color properties.
template<EmNexPage& tPage>
class EmNexColoredElement: public EmNexPageElement<tPage>
{
public:
    EmNexColoredElement(const char* name,
                        EmLogLevel logLevel=EmLogLevel::global)
     : EmNexPageElement<tPage>(name, logLevel){}


    // Set background color.
    bool setBkColor(uint8_t red,
                    uint8_t green,
                    uint8_t blue) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->name(), 
                                      toColor565(red, green, blue));
    }

    bool setBkColor(uint16_t color565) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->name(), 
                                      color565);
    }

    // Get background color.
    bool getBkColor(uint8_t& red,
                    uint8_t& green,
                    uint8_t& blue) const {
        return this->nex().getBkColor(tPage.name(), 
                                      this->name(), 
                                      red, green, blue);
    }

    bool getBkColor(uint16_t& color565) const {
        return this->nex().getBkColor(tPage.name(), 
                                      this->name(), 
                                      color565);
    }

    // Set font color.
    bool setFontColor(uint8_t red,
                      uint8_t green,
                      uint8_t blue) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->name(), 
                                        red, green, blue);
    }

    bool setFontColor(uint16_t color565) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->name(), 
                                        color565);
    }

    // Get font color.
    bool getFontColor(uint8_t& red,
                      uint8_t& green,
                      uint8_t& blue) const {
        return this->nex().getFontColor(tPage.name(), 
                                        this->name(), 
                                        red, green, blue);
    }

    bool getFontColor(uint16_t& color565) const {
        return this->nex().getFontColor(tPage.name(), 
                                        this->name(), 
                                        color565);
    }

};

// A text displayed on an 'Text' nextion object.
template<EmNexPage& tPage>
class EmNexText: public EmNexColoredElement<tPage>
{
public:
    EmNexText(const char* name,
              EmLogLevel logLevel=EmLogLevel::global)
     : EmNexColoredElement<tPage>(name, logLevel) {}

    template<size_t max_str_len>
    EmGetValueResult getValue(char* value) const {
        return this->nex().getTextElementValue<max_str_len>(this->pageName(), this->name(), value);
    }

    bool setValue(const char* value) const {
        return this->nex().setTextElementValue(this->pageName(), this->name(), value);
    }

    template <uint16_t max_str_len>
    bool setValue(const char* format, ...) const {
        char text[max_str_len+1];
        va_list args;
        va_start(args, format);     
        vsnprintf(text, max_str_len+1, format, args);
        va_end(args);
        return this->nex().setTextElementValue(this->pageName(), this->name(), text);
    }
};


// An integer number displayed on an 'Number' nextion object.
template<EmNexPage& tPage>
class EmNexInteger: public EmNexColoredElement<tPage>
{
public:
    EmNexInteger(const char* name,
                 EmLogLevel logLevel=EmLogLevel::global)
     : EmNexColoredElement<tPage>(name, logLevel) {}

    // Templated methods
    template <class T>
    EmGetValueResult getValue(T& value) const {
        int32_t val = static_cast<int32_t>(value);
        EmGetValueResult res = getValue(val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<T>(val);
        }
        return res;
    }

    EmGetValueResult getValue(int32_t& value) const {
        return this->nex().getNumElementValue(this->pageName(), 
                                              this->name(), 
                                              value);
    }

    bool setValue(const int32_t& value) const {
        return this->nex().setNumElementValue(this->pageName(), 
                                              this->name(), 
                                              value);
    }
};


// A floating point number displayed on a 'Xfloat' nextion object.
template<EmNexPage& tPage>
class EmNexReal: public EmNexColoredElement<tPage>
{
public:
    EmNexReal(const char* name,
              uint8_t decPlaces,
              EmLogLevel logLevel=EmLogLevel::global)
     : EmNexColoredElement<tPage>(name, logLevel),
       m_decPlaces(decPlaces) {}

    // Templated methods
    template <class real_type>
    EmGetValueResult getValue(real_type& value) const {
        int32_t val = iMolt<real_type>(value, iPow10(m_decPlaces));
        EmGetValueResult res = this->nex().getNumElementValue(this->pageName(), 
                                                              this->name(), 
                                                              val);
        if (EmGetValueResult::failed != res) {
            value = static_cast<real_type>(val)/pow(10, m_decPlaces);
        }
        return res;
    }
 
    template <class real_type>
    bool setValue(real_type const value) {
        return this->nex().setNumElementValue(this->pageName(), 
                                             this->name(), 
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


// A two labels number. 
//
// The float value is displayed on two different 'Number' nextion objects.
template<EmNexPage& tPage>
class EmNexDecimal: public EmNexColoredElement<tPage>
{
public:
    EmNexDecimal(const char* intElementName, // This will be the object name!
                 const char* decElementName,
                 uint8_t decPlaces,
                 EmLogLevel logLevel=EmLogLevel::global)
     : EmNexColoredElement<tPage>(intElementName, logLevel),
       m_decElementName(decElementName),
       m_decPlaces(decPlaces) {}

    bool setValue(double value) {
        int32_t exp = iPow10(this->m_decPlaces);
        int32_t dispValue = iRound(value*static_cast<double>(exp));
        return this->nex().setNumElementValue(this->pageName(), 
                                              this->name(), 
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
                                             this->name(), 
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
                                      this-> name(), 
                                      toColor565(red, green, blue)) &&
               this->nex().setBkColor(tPage.name(), 
                                      m_decElementName, 
                                      toColor565(red, green, blue));
    }

    bool setBkColor(uint16_t color565) const {
        return this->nex().setBkColor(tPage.name(), 
                                      this->name(), 
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
                                        this->name(), 
                                        red, green, blue) &&
               this->nex().setFontColor(tPage.name(), 
                                        m_decElementName, 
                                        red, green, blue);
    }

    bool setFontColor(uint16_t color565) const {
        return this->nex().setFontColor(tPage.name(), 
                                        this->name(), 
                                        color565) &&
               this->nex().setFontColor(tPage.name(), 
                                        m_decElementName, 
                                        color565);
    }

protected:
    const char* m_decElementName;
    const uint8_t m_decPlaces;
};


// Configuration element for integer values.
//
// This class is used to handle configuration elements on the Nextion display.
// A configuration element should be initialized first (i.e. value sent to the display element)
// before reading it.
//
// TODO: handle uninitialized elements on startup (i.e. alternatives to 'dispInitialValue' parameter)
//       This should be done in case the display powers off an on while controller is still running.
//       If this happens configuration values need to be re-initialized to the display.
template<EmNexPage& tPage>
class EmNexCfgInteger: public EmNexInteger<tPage> {
public: 
    EmNexCfgInteger(const char* name,
                    EmLogLevel logLevel=EmLogLevel::global)
     : EmNexInteger<tPage>(name, logLevel),
       m_isInitialized(false) {}

    
    // Reads the value if already initialized, or set it.
    //
    // 'minValue' is the optional initial minimum value 
    //  (i.e. if display value is less than minValue, minValue is assigned to 'value').
    // 'maxValue' is the optional initial maximum value 
    //  (i.e. if display value is greater than maxValue, maxValue is assigned to 'value').
    // 'dispInitialValue' is the optional initial value of the display element on power on
    //  (i.e. if the display value is equal to 'dispInitialValue' then this element is considered uninitialized).
    template<typename T, typename V>
    bool updateValue(EmValue<T>& value, 
                     EmOptional<V> minValue = emUndefined,
                     EmOptional<V> maxValue = emUndefined,
                     EmOptional<V> dispInitialValue = emUndefined) {
        // Set or get value ONLY if not on that page!
        if (tPage.isCurrent()) {
            return false;
        }
        // Value already set the first time?
        if (m_isInitialized) {
            // Get value from display
            T dispValue;
            if (getValue_(dispValue)) {
                if (dispInitialValue.hasValue() && dispValue == dispInitialValue.value()) {
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
    bool isInitialized() const {
        return m_isInitialized;
    }

    // Resets the element to its uninitialized state
    void reset() {
        m_isInitialized = false;
    }

protected:
    bool getValue_(int16_t& value) {
        return EmNexInteger<tPage>::getValue(value) != EmGetValueResult::failed;
    }

    bool getValue_(int32_t& value) {
        return EmNexInteger<tPage>::getValue(value) != EmGetValueResult::failed;
    }

    bool getValue_(EmTagValue& value) {
        int32_t dispValue = value.isType(EmTagValueType::vt_integer) ? value.asInteger() : 0;
        EmGetValueResult res = EmNexInteger<tPage>::getValue(dispValue);
        if (EmGetValueResult::succeedNotEqualValue == res) {
            return value.setValue(dispValue, false);
        }
        return EmGetValueResult::failed != res;
    }

    bool setValue_(int32_t value) {
        return EmNexInteger<tPage>::setValue(value);
    }

    bool setValue_(EmTagValue value) {
        if (value.isNotType(EmTagValueType::vt_integer)) {
            return false;
        }
        return EmNexInteger<tPage>::setValue(value.asInteger());
    }

    // Member vars
    bool m_isInitialized;   
};

#endif // __EM_NEXTION__
