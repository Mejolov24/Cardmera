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

// --- NEW BUFFERING VARIABLES ---
bool is_receiving = false;
uint32_t frame_bytes_read = 0;
const int IMG_WIDTH = 160;
const int IMG_HEIGHT = 120;
const int IMG_SIZE = IMG_WIDTH * IMG_HEIGHT * 2; // 38,400 bytes
uint8_t image_buffer[IMG_SIZE];

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
  Serial.begin(9600);
  Serial2.setRxBufferSize(40000);
  Serial2.begin(2000000,SERIAL_8N1,13,15);
  keyHandler.SetupKeyboardCallback(OnKey);
  modeSetSend(REQUEST_FRAME);
  is_receiving = true;
  frame_bytes_read = 0;
}

void loop() {
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();

  if (is_receiving) {
    int available_bytes = Serial2.available();
    if (available_bytes > 0) {
      // Read chunks of data as they arrive
      int bytes_to_read = min(available_bytes, (int)(IMG_SIZE - frame_bytes_read));
      Serial2.readBytes(&image_buffer[frame_bytes_read], bytes_to_read);
      frame_bytes_read += bytes_to_read;

      // Once we have a full 160x120 frame
      if (frame_bytes_read >= IMG_SIZE) {
        is_receiving = false;
        Serial.println("Frame complete. Rendering...");

        // Center the 160x120 image on the 240x135 screen
        int x = (M5.Display.width() - IMG_WIDTH) / 2;
        int y = (M5.Display.height() - IMG_HEIGHT) / 2;

        // The ESP32-CAM outputs RGB565 with a swapped byte order relative to M5GFX 
        M5.Display.setSwapBytes(false); 
        M5.Display.pushImage(x, y, IMG_WIDTH, IMG_HEIGHT, (uint16_t*)image_buffer);
        modeSetSend(REQUEST_FRAME);
        is_receiving = true;
        frame_bytes_read = 0;
      }
    }
  } else {
    // Flush out any garbage bytes when not explicitly waiting for a frame
    while (Serial2.available()) Serial2.read();

  }

}