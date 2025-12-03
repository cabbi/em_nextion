#ifndef __EM_NEXTION_EX__
#define __EM_NEXTION_EX__

#include "em_tag.h"
#include "em_nextion.h"


// The base EmTag nextion object used as base for other nextion tag classes.
template<class nexElement, class T>
class EmNexTagBase: public nexElement
{
public:
    using nexElement::nexElement;

    virtual ~EmNexTagBase() = default;

    virtual const char* getId() const {
        return EmNexTagBase::name();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const {
        T val;
        EmGetValueResult res = nexElement::getValue(val);
        if (EmGetValueResult::failed != res) {
            value.setValue(val, true);
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) {
        return nexElement::setValue(value.as<T>());
    }
};


// A text tag displayed on an 'Text' nextion object.
template<EmNexPage& tPage, size_t max_str_len>
class EmNexTextTag: public EmNexText<tPage>,
                    public EmTagBase {
public:
    EmNexTextTag(const char* name,
                 EmSyncFlags syncFlags,
                 EmLogLevel logLevel=EmLogLevel::global)
     : EmNexText<tPage>(name, logLevel),
       EmTagBase(syncFlags) {}

     EmNexTextTag(const char* name,
                  EmSyncFlags syncFlags,
                  EmTags& tags,
                  EmLogLevel logLevel=EmLogLevel::global)
     : EmNexTextTag(name, syncFlags, logLevel) {
        tags.add(*this);
    }

    virtual const char* getId() const override {
        return EmNexText<tPage>::name();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const {
        char val[max_str_len+1];
        EmGetValueResult res = EmNexText<tPage>::getValue<max_str_len>(val);
        if (EmGetValueResult::failed != res) {
            value.setValue(val, true);
        }
        return res;
    }

    virtual bool setValue(const EmTagValue& value) {
        return EmNexText<tPage>::setValue(value.asString());
    }
};


// An integer tag displayed on an 'Number' nextion object.
template<EmNexPage& tPage>
class EmNexIntegerTag: public EmNexTagBase<EmNexInteger<tPage>, int32_t>,
                       public EmTagBase {
public:
    EmNexIntegerTag(const char* name,
                    EmSyncFlags syncFlags,
                    EmLogLevel logLevel=EmLogLevel::global)
     : EmNexTagBase<EmNexInteger<tPage>, int32_t>(name, logLevel),
       EmTagBase(syncFlags) {}

     EmNexIntegerTag(const char* name,
                     EmSyncFlags syncFlags,
                     EmTags& tags,
                    EmLogLevel logLevel=EmLogLevel::global)
     : EmNexIntegerTag(name, syncFlags, logLevel) {
        tags.add(*this);
    }

    virtual const char* getId() const override {
        return EmNexTagBase<EmNexInteger<tPage>, int32_t>::getId();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmNexTagBase<EmNexInteger<tPage>, int32_t>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmNexTagBase<EmNexInteger<tPage>, int32_t>::setValue(value);
    }
};

// An real tag displayed on an 'Xfloat' nextion object.
template<EmNexPage& tPage>
class EmNexRealTag: public EmNexTagBase<EmNexReal<tPage>, double>,
                    public EmTagBase  {
public:
    EmNexRealTag(const char* name,
                 uint8_t decPlaces,
                 EmSyncFlags syncFlags,
                 EmLogLevel logLevel=EmLogLevel::global)
     : EmNexTagBase<EmNexReal<tPage>, double>(name, decPlaces, logLevel),
       EmTagBase(syncFlags) {}

     EmNexRealTag(const char* name,
                  uint8_t decPlaces,
                  EmSyncFlags syncFlags,
                  EmTags& tags,
                  EmLogLevel logLevel=EmLogLevel::global)
     : EmNexRealTag(name, decPlaces, syncFlags, logLevel) {
        tags.add(*this);
    }

    virtual const char* getId() const override {
        return EmNexTagBase<EmNexReal<tPage>, double>::getId();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmNexTagBase<EmNexReal<tPage>, double>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmNexTagBase<EmNexReal<tPage>, double>::setValue(value);
    }
};


// A two labels number. 
//
// The float value is displayed on two different 'Number' nextion objects.
template<EmNexPage& tPage>
class EmNexDecimalTag: public EmNexTagBase<EmNexDecimal<tPage>, double>,
                       public EmTagBase  {
public:
    EmNexDecimalTag(const char* intElementName, // This will be the object name!
                    const char* decElementName,
                    uint8_t decPlaces,
                    EmSyncFlags syncFlags,
                    EmLogLevel logLevel=EmLogLevel::global)
     : EmNexTagBase<EmNexDecimal<tPage>, double>(intElementName, decElementName, decPlaces, logLevel),
       EmTagBase(syncFlags) {}

     EmNexDecimalTag(const char* intElementName, // This will be the object name!
                     const char* decElementName,
                     uint8_t decPlaces,
                     EmSyncFlags syncFlags,
                     EmTags& tags,
                     EmLogLevel logLevel=EmLogLevel::global)
     : EmNexDecimalTag(intElementName, decElementName, decPlaces, syncFlags, logLevel) {
        tags.add(*this);
    }

    virtual const char* getId() const override {
        return EmNexTagBase<EmNexDecimal<tPage>, double>::getId();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmNexTagBase<EmNexDecimal<tPage>, double>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmNexTagBase<EmNexDecimal<tPage>, double>::setValue(value);
    }
};

// Configuration element for integer tag values
template<EmNexPage& tPage>
class EmNexCfgIntegerTag: public EmNexTagBase<EmNexCfgInteger<tPage>, int32_t>,
                          public EmTagBase {
public: 
    EmNexCfgIntegerTag(const char* name,
                       EmSyncFlags syncFlags,
                       EmLogLevel logLevel=EmLogLevel::global)
     : EmNexTagBase<EmNexCfgInteger<tPage>, int32_t>(name, logLevel), 
       EmTagBase(syncFlags) {}

    EmNexCfgIntegerTag(const char* name,
                       EmSyncFlags syncFlags,
                       EmTags& tags,
                       EmLogLevel logLevel=EmLogLevel::global)
     : EmNexCfgIntegerTag<tPage>(name, logLevel) {
        tags.add(*this);
    }

    virtual const char* getId() const override {
        return EmNexTagBase<EmNexCfgInteger<tPage>, int32_t>::getId();
    }

    virtual EmGetValueResult getValue(EmTagValue& value) const override {
        return EmNexTagBase<EmNexCfgInteger<tPage>, int32_t>::getValue(value);
    }

    virtual bool setValue(const EmTagValue& value) override {
        return EmNexTagBase<EmNexCfgInteger<tPage>, int32_t>::setValue(value);
    }
};

#endif // __EM_NEXTION_EX__
