#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5CADVKeyCB.h>
M5CADVKeyCB keyHandler;


enum PixFormat{
    PIXFORMAT_RGB565,    // 2BPP/RGB565
    PIXFORMAT_YUV422,    // 2BPP/YUV422
    PIXFORMAT_YUV420,    // 1.5BPP/YUV420
    PIXFORMAT_GRAYSCALE, // 1BPP/GRAYSCALE
    PIXFORMAT_JPEG,      // JPEG/COMPRESSED
    PIXFORMAT_RGB888,    // 3BPP/RGB888
    PIXFORMAT_RAW,       // RAW
    PIXFORMAT_RGB444,    // 3BP2P/RGB444
    PIXFORMAT_RGB555,    // 3BP2P/RGB555
};

enum FrameSize{
    FRAMESIZE_96X96,    // 96x96
    FRAMESIZE_QQVGA,    // 160x120
    FRAMESIZE_QCIF,     // 176x144
    FRAMESIZE_HQVGA,    // 240x176
    FRAMESIZE_240X240,  // 240x240
    FRAMESIZE_QVGA,     // 320x240
    FRAMESIZE_CIF,      // 400x296
    FRAMESIZE_HVGA,     // 480x320
    FRAMESIZE_VGA,      // 640x480
    FRAMESIZE_SVGA,     // 800x600
    FRAMESIZE_XGA,      // 1024x768
    FRAMESIZE_HD,       // 1280x720
    FRAMESIZE_SXGA,     // 1280x1024
    FRAMESIZE_UXGA,     // 1600x1200
    // 3MP Sensors
    FRAMESIZE_FHD,      // 1920x1080
    FRAMESIZE_P_HD,     //  720x1280
    FRAMESIZE_P_3MP,    //  864x1536
    FRAMESIZE_QXGA,     // 2048x1536
    // 5MP Sensors
    FRAMESIZE_QHD,      // 2560x1440
    FRAMESIZE_WQXGA,    // 2560x1600
    FRAMESIZE_P_FHD,    // 1080x1920
    FRAMESIZE_QSXGA,    // 2560x1920
    FRAMESIZE_INVALID
};

enum modetype{
  FLASH,
  PX_FORMAT,
  FR_SIZE,
  FB_SIZE,
  APPLY_CAM,
  REQUEST_FRAME
};

void modeSetSend(modetype mode,uint8_t value = 0){
Serial2.write(0xFF);
Serial2.write(mode);
Serial2.write(value);
}

void OnKey(uint8_t key, bool pressed){
    Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
    if(!pressed) return;
    switch (key)
        {
        case 54: // left
        modeSetSend(FLASH,false);
        break;

        case 56:// right
        modeSetSend(FLASH,true);
        break;

        default:
            break;
}
}

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  M5.Display.fillScreen(BLACK);
  Serial2.begin(2000000,SERIAL_8N1,13,15);
  keyHandler.SetupKeyboardCallback(OnKey);

}

void loop() {
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}