#include "em_log.h"
#include "em_string.h"
#include "em_timeout.h"
#include "em_nextion_ota.h"

uint32_t bytesToInt(const char* p) {
    return ((uint32_t)p[0])       |
           ((uint32_t)p[1] << 8)  |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

bool EmNextionOtaUpdater::update(EmStreamRx& client, size_t contentLength) {
    m_disp.logInfo(F("Initiating Nextion OTA update..."));
    tx_("connect");
    char buf[100];
    rx_(buf, 100);
    // Wakeup display before sending firmware    
    if (!m_disp.wakeup()) {
        m_disp.logError(F("OTA: Failed to initiate OTA update"));
        return false;
    }
#ifdef EM_MULTITHREAD
    EmMutexLock lock(m_disp.m_serialLock);
#endif
    #define R_ACK 0x05
    #define R_SKIP 0x08
    #define PACKET_SIZE 4096
    // Send 'whmi-wri' command
    char rxBuf[5];
    EmStringM cmd;
    cmd.format("whmi-wri %u,%u,1", contentLength, m_disp.m_serial.baudRate());
    tx_(cmd.c_str());
    
    if (!rx_(R_ACK)) {
        m_disp.logError(F("OTA: No response for 'whmi-wri' command!"));        
        //return false; // NOTE: we continue even if no response, just to try
                        //       to get the display to the end of the procedure
    }
    // Start transmitting firmware packets
    bool skip = false;
    bool fillupMode = false;
    size_t sentBytes = 0;
    size_t toSend = MIN(PACKET_SIZE, contentLength);
    while (sentBytes < contentLength) {
        // Send next packet
        if (!uploadPacket_(client, toSend, fillupMode, skip)) {
            return false;
        }
        sentBytes += toSend;
        // Read response
        if (!rx_(rxBuf, 1)) {
            m_disp.logError(F("OTA: No response for sent packet!"));
            return false;
        }
        skip = rxBuf[0] == R_SKIP;
        if (skip) {
            if (!rx_(rxBuf, 4)) {
                m_disp.logError(F("OTA: No skip size received!"));
                return false;
            }
            toSend = MIN(bytesToInt(rxBuf), contentLength - sentBytes);
        } else 
        if (rxBuf[0] == R_ACK) {
            toSend = MIN(PACKET_SIZE, contentLength - sentBytes);
        } else {
            m_disp.logError(F("OTA: Unexpected response!"));
            return false;
        }
    }
    return !fillupMode;
}

void EmNextionOtaUpdater::tx_(const char* cmd) {
    m_disp.m_serial.flush(true);
    m_disp.m_serial.write(cmd);
    m_disp.m_serial.write("\xFF\xFF\xFF");
}    

bool EmNextionOtaUpdater::rx_(char rxChar) {
    EmTimeout rxTimeout(500);
    while (!rxTimeout.isExpired(false)) {
        while (m_disp.m_serial.available()) {
            char c = static_cast<char>(m_disp.m_serial.read());
            if (c == rxChar) {
                return true;
            }
        }
    }
    return false;
}

bool EmNextionOtaUpdater::rx_(char* buf, size_t size) {
    uint8_t term_count=0;
    uint8_t buf_pos=0;
    EmTimeout rxTimeout(500);
    while (!rxTimeout.isExpired(false) && buf_pos < size) {
        while (m_disp.m_serial.available()) {
            uint8_t c = static_cast<uint8_t>(m_disp.m_serial.read());
            buf[buf_pos++] = static_cast<char>(c);
            if (c == 0xFF) {
                term_count++;
                if (term_count>=3) {
                    return true;
                }
            } else {
                term_count = 0;
            }
        }
    }
    return buf_pos == size;
}

bool EmNextionOtaUpdater::uploadPacket_(EmStreamRx& client, 
                                        size_t size,
                                        bool& fillupMode, 
                                        bool skip) {
    // The uart chunk used to optimize serial transmission.
    const size_t chunkSize = 512;
    char chunk[chunkSize];
    // A big timeout in case of slow streams (e.g. HTTP responses on weak wifi connection)
    EmTimeout streamTimeout(m_clientReadTimeout); 
    size_t sentBytes = 0;
    while (sentBytes < size) {
        if (fillupMode) {
            // Filling up with zeros just to end up procedure and not get display stuck
            // into the firmware update procedure where it does not react to any command.
            size_t toSend = MIN(chunkSize, size - sentBytes);
            memset(chunk, 0, toSend);
            sentBytes += m_disp.m_serial.write(chunk, toSend);
        } else { 
            // Wait for stream data
            streamTimeout.restart();
            while (!client.available() && !streamTimeout.isExpired(true)) {
                tDelay(1, true);
            }
            // Got data from stream?
            size_t toSend = MIN(chunkSize, MIN(size - sentBytes, client.available()));
            if (!toSend) {
                m_disp.logError(F("OTA: stream data timeout, filling with zeros to end the procedure!"));
                fillupMode = true;
            }
            // Read the stream byte
            toSend = client.read((uint8_t*)chunk, toSend);
            if (skip) {
                sentBytes += toSend; // Skip the bytes, but count them as sent
            } else {
                sentBytes += m_disp.m_serial.write(chunk, toSend);
            }  
        } 
        m_disp.logInfo<100>("Sent %u bytes of %u", sentBytes, size);
    }
    return true;
}
