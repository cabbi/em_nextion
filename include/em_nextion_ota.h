#ifndef _EM_NEXTION_OTA_H__
#define _EM_NEXTION_OTA_H__

#include "em_nextion.h"
#include "em_ota_updater.h"

// Nextion OTA updater class providing over-the-air updates for Nextion displays.
//
// This uses the 'whmi-wri' command to upload a new firmware to the display as described 
// in the official Nextion Instruction Set page.
//
// Note that the "Nextion Editor" update files (.tft) are NOT using the "normal" firmware update, but
// it is querying the display to grab differences and updating only edited/updated parts. This can ONLY
// be done with the Nextion Editor (i.e. this updater will send the entire firmware file!).
class EmNextionOtaUpdater : public EmOtaUpdater {
public:
    EmNextionOtaUpdater(EmNextion& disp)
     : m_disp(disp) {}

    virtual bool update(Stream& client, size_t contentLength) override;
        
protected:
    void tx_(const char* buf);
    void tx_(const char* buf, size_t size);
    bool rx_(char* buf, size_t maxSize);
    bool uploadPacket_(Stream& client, size_t size, bool skip);

    // Member vars
    EmNextion& m_disp;
};

#endif //_EM_NEXTION_OTA_H__