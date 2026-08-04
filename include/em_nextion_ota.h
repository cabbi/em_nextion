#ifndef _EM_NEXTION_OTA_H__
#define _EM_NEXTION_OTA_H__

#include "em_duration.h"

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
//
// NOTE: the Nextion firmware update procedure gets stuck forever if not finalized.
//       That said we try to get it to the end even with failure just to have the
//       display in a state where it accepts another firmware update command.

class EmNextionOtaUpdater : public EmOtaUpdater {
public:
    EmNextionOtaUpdater(EmNextion& disp,
                        EmDuration clientReadTimeout = EmDuration(3000))
     : m_disp(disp),
       m_clientReadTimeout(clientReadTimeout) {}

    virtual bool update(EmStreamRx& client, size_t contentLength) override;
        
protected:
    void tx_(const char* cmd);
    bool rx_(char* buf, size_t maxSize);
    bool rx_(char rxChar);
    bool uploadPacket_(EmStreamRx& client, 
                       size_t size, 
                       bool& fillupMode,
                       bool skip);

    // Member vars
    EmNextion& m_disp;
    EmDuration m_clientReadTimeout;
};

#endif //_EM_NEXTION_OTA_H__