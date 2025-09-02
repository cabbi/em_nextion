#include <Arduino.h>
#include <SoftwareSerial.h>

#include "em_nextion.h"


// NOTE:
//   EmNextion uses 'EmComSerial' as a wrapper to enable both Hardware an Software serials
SoftwareSerial dispSerial(A0, A1); // RX, TX
EmComSerial comSerial(dispSerial);
EmNextion display(comSerial, 30);

// Some objects
EmNexPage mainPage(display, 0, "p_main");

EmNexText caption(mainPage, "t_caption");
EmNexDecimal decimalNumber(mainPage, "tr_1", "tr_2", 1);

EmNexPage alertPage(display, 2, "p_alert");
EmNexText alertLabel(alertPage, "c_label");

EmNexPicture cfgBtn(mainPage, "btn_cfg");

void setup() {
    dispSerial.begin(9600);
    display.Init();
}

void loop() {
    bool bRes;
    bRes = display.setVisible(0, "p_temp", false);
    delay(1000);
    bRes = display.setVisible("p_temp", true);

    uint16_t bkCol = 0, fontCol = 0;
    bRes = decimalNumber.getBkColor(bkCol);
    bRes = decimalNumber.getFontColor(fontCol);
    delay(1000);
    bRes = decimalNumber.setBkColor(EmNexColor::BROWN);
    bRes = decimalNumber.setFontColor(EmNexColor::BLUE);
    delay(1000);
    bRes = decimalNumber.setBkColor(bkCol);
    bRes = decimalNumber.setFontColor(fontCol);

    bRes = cfgBtn.click();
    delay(5000);
    bRes = mainPage.setAsCurrent();

    EmGetValueResult res;
    EmString<20> captionTxt;
    res = caption.getValue<10>(captionTxt.Buffer());

    int32_t num=10000;
    res = display.getNumElementValue("p_main", "tr_1", num);

    uint8_t pageId;
    if (!display.getCurPage(pageId)) {
        printf("GetCurPage failed!");
    }

    if (!decimalNumber.setValue(12.5)) {
        printf("Set number failed!");
    }

    double dblVal = 1234;
    res = decimalNumber.getValue(dblVal);
    res = decimalNumber.getValue(dblVal);

    if (!caption.setValue("This is a test!")) {
        printf("Set caption text failed!");
    }

    delay(1000);

    if (!alertLabel.setValue("This test is very long\\rmessage!!!")) {
        printf("Set alert text failed!");
    }

    if (!alertPage.setAsCurrent()) {
        printf("SetCurPage failed!");
    }

    delay(10000);
    mainPage.setAsCurrent();
}
