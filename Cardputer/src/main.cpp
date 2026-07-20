#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5CADVKeyCB.h>
#include <M5Menu.h>
#include <Menu.h>
#include <OneButton.h>
#include <Ticker.h>
OneButton g0Button(0, true);
M5CADVKeyCB keyHandler;
M5Menu menu;
M5Canvas canvas(&M5.Lcd);

struct Resolution {
    uint16_t w;
    uint16_t h;
};

const Resolution frameSizes[] = {
    {96,96},
    {160,120},
    {176,144},
    {240,176},
    {240,240},
    {320,240},
    {400,296},
    {480,320},
    {640,480},
    {800,600},
    {1024,768},
    {1280,720},
    {1280,1024},
    {1600,1200},
    {1920,1080},
    {720,1280},
    {864,1536},
    {2048,1536},
    {2560,1440},
    {2560,1600},
    {1080,1920},
    {2560,1920},
};

enum ReceiveState {
  WAIT_SYNC_1,
  WAIT_SYNC_2,
  READ_LENGTH,
  READ_PAYLOAD
};

enum modetype{
  FLASH,
  FR_SIZE,
  JPEG_QUALITY,
  BRIGHTNESS,
  CONTRAST,
  SATURATION,
  SHARPNESS,
  WB,
  WB_MODE,
  AWB_GAIN,
  EXP_CTRL,
  AE_LEVEL,
  AEC_VALUE,
  AEC2,
  GAIN_CTRL,
  GAIN_CEILING,
  LENS_CORR,
  MIRROR,
  FLIP,
  SPECIAL,
  REQUEST_FRAME
};

bool force_flash = false;

ReceiveState rx_state = WAIT_SYNC_1;
bool is_receiving = false;
uint32_t jpeg_length = 0;
uint32_t frame_bytes_read = 0;

#define MAX_IMG_SIZE 81920
uint8_t video_buffer[MAX_IMG_SIZE];

unsigned long lastFrameTime = 0;
const unsigned long FRAME_TIMEOUT = 500;
bool video_starved = false;

bool at_menu = false;
bool pending_modeset = false;
bool holding_shutter = false;
localCameraSettings* current_settings = &viewfinder_settings;

#define BEEP_1 1318.51
#define BEEP_2 1046.50
#define CAMERA_PRESS 1567.98
#define CAMERA_RELEASE 2093.00

#define UI_FONT &fonts::FreeSerifBold9pt7b
#define NO_SIGNAL_FONT &fonts::FreeSerifBold12pt7b

void modeSetSend(modetype mode,int8_t value = 0){
Serial2.write(0xFF);
Serial2.write(mode);
Serial2.write(static_cast<uint8_t>(value));
}

uint32_t fast_noise() {
    static uint32_t state = 0xACE1u; // Seed
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void renderStatic() {
  Resolution r = frameSizes[current_settings->FR_SIZE];
  float scale = min((float)canvas.width() / r.w, (float)canvas.height() / r.h);
  int x_offset = (canvas.width() - (r.w * scale)) / 2;
  int y_offset = (canvas.height() - (r.h * scale)) / 2;
  int targetW = (int)(r.w * scale);
  int targetH = (int)(r.h * scale);
  uint16_t* buffer = (uint16_t*)video_buffer;
  for (int y = 0; y < targetH; y += 2){
    for (int x = 0; x < targetW; x += 2){
      uint16_t color = (fast_noise() & 0x1) ? 0xFFFF : 0x0000;
      buffer[y * targetW + x] = color;
      buffer[y * targetW + x + 1] = color;
      buffer[(y + 1) * targetW + x] = color;
      buffer[(y + 1) * targetW + x + 1] = color;
    }
  }
  canvas.pushImage(x_offset, y_offset, targetW, targetH, buffer);
}

void render(){
  canvas.pushSprite(0,0);
  canvas.clear();
}

void apply_modeset(){
// discard old camera data
//while (Serial2.available()) {Serial2.read();}
modeSetSend(FR_SIZE,current_settings->FR_SIZE);
modeSetSend(JPEG_QUALITY,current_settings->JPEG_QUALITY);
modeSetSend(BRIGHTNESS,global_settings.BRIGHTNESS);
modeSetSend(CONTRAST,global_settings.CONTRAST);
modeSetSend(CONTRAST,global_settings.CONTRAST);
modeSetSend(SHARPNESS,global_settings.SHARPNESS);
modeSetSend(WB,global_settings.WB);
modeSetSend(WB_MODE,global_settings.WB_MODE);
modeSetSend(AWB_GAIN,global_settings.AWB_GAIN);
modeSetSend(EXP_CTRL,global_settings.EXP_CTRL);
modeSetSend(AE_LEVEL,global_settings.AE_LEVEL);
modeSetSend(AEC_VALUE,static_cast<int8_t>(aec_map[global_settings.AEC_VALUE]));
modeSetSend(AEC2,global_settings.AEC2);
modeSetSend(GAIN_CTRL,global_settings.GAIN_CTRL);
modeSetSend(GAIN_CEILING,global_settings.GAIN_CEILING);
modeSetSend(LENS_CORR,global_settings.LENS_CORR);
modeSetSend(MIRROR,global_settings.MIRROR);
modeSetSend(FLIP,global_settings.FLIP);
modeSetSend(SPECIAL,global_settings.SPECIAL);
}


void OnUsage(M5Menu::MenuItem* item, M5Menu::Menu* menu){
    if (menu->id == 1 or menu->id == 4){pending_modeset = true;}
    }

void OnKey(uint8_t key, bool pressed){
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  if (status.opt){
    at_menu = !at_menu;
    at_menu ? menu.open() : menu.close();
  }
  if (status.del){menu.process_input(M5Menu::Input::BACK);}
  if (status.enter) menu.process_input(M5Menu::Input::SELECT);
  Serial.print(key);
  if(!pressed) return;
  switch (key){
    case 53: // escape
    force_flash = !force_flash;
    modeSetSend(FLASH,force_flash);
    M5Cardputer.Speaker.tone((force_flash ? BEEP_1 : BEEP_2),100);
    
    case 51:
        menu.process_input(M5Menu::Input::UP);
        break;
    case 55:
        menu.process_input(M5Menu::Input::DOWN);
        break;

    case 54:
    menu.process_input(M5Menu::Input::LEFT);
    break;

    case 56:
    menu.process_input(M5Menu::Input::RIGHT);
    break;

    default:
        break;
  }
}

void RequestFrame(){
modeSetSend(REQUEST_FRAME);
is_receiving = true;
rx_state = WAIT_SYNC_1;
}

void invert_rgb(int x, int y, int w, int h) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();

  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {

      int i = yy * canvas.width() + xx;

      uint16_t p = pixels[i];

      // RGB565 extraction
      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;

      // swap red and blue
      pixels[i] = (b << 11) | (g << 5) | r;
    }
  }
}
void invert_endians(int x, int y, int w, int h) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();

  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {

      int i = yy * canvas.width() + xx;

      uint16_t p = pixels[i];

      // swap the two bytes
      pixels[i] = (p >> 8) | (p << 8);
    }
  }
}
void CameraTick(){
  if (!is_receiving or at_menu) return;
  if (pending_modeset and !at_menu) apply_modeset(); pending_modeset = false;
  while (Serial2.available() > 0) {
    if (rx_state == WAIT_SYNC_1) {if (Serial2.read() == 0xAA) rx_state = WAIT_SYNC_2;}
      else if (rx_state == WAIT_SYNC_2) {
      if (Serial2.read() == 0xBB) rx_state = READ_LENGTH;
      else rx_state = WAIT_SYNC_1;
    }
    else if (rx_state == READ_LENGTH) {
      if (Serial2.available() >= 4) {
        Serial2.readBytes((uint8_t*)&jpeg_length, 4);
        if (jpeg_length > MAX_IMG_SIZE || jpeg_length == 0) rx_state = WAIT_SYNC_1; // Corrupted header, restart
        else {
          rx_state = READ_PAYLOAD;
          frame_bytes_read = 0;
        }
      } else {
        break; // Wait for the rest of the 4 length bytes
      }
      
    }
    else if (rx_state == READ_PAYLOAD) {
      int bytes_available = Serial2.available();
      int bytes_to_read = min(bytes_available, (int)(jpeg_length - frame_bytes_read));
      Serial2.readBytes(&video_buffer[frame_bytes_read], bytes_to_read);
      frame_bytes_read += bytes_to_read;
      lastFrameTime = millis();
      // If we have received the full JPEG
      if (frame_bytes_read >= jpeg_length) {
        Resolution r = frameSizes[current_settings->FR_SIZE];
        float scale = min((float)canvas.width() / r.w, (float)canvas.height() / r.h);
        int x = (canvas.width() - (r.w * scale)) / 2;
        int y = (canvas.height() - (r.h * scale)) / 2;
        canvas.drawJpg(video_buffer, jpeg_length, x, y,0,0,0, scale,scale);
        if (global_settings.invert_rgb) invert_rgb(x, y, r.w * scale, r.h * scale);
        if (global_settings.invert_endians) invert_endians(x, y, r.w * scale, r.h * scale);
        // Reset state machine and request the next frame
        rx_state = WAIT_SYNC_1;
        modeSetSend(REQUEST_FRAME);
        render();
        break;
      }
    }
  }
}

void camera_poll(){
  if (millis() - lastFrameTime > FRAME_TIMEOUT){
    renderStatic();
    RequestFrame();
    if (!video_starved){
      canvas.clear();
      canvas.setFont(NO_SIGNAL_FONT);
      canvas.setTextDatum(middle_center);
      canvas.setTextColor(ORANGE,BLACK);
    }
    canvas.drawString("No video",M5.Display.width()/2,M5.Display.height()/2);
    video_starved = true;
    pending_modeset = true;
    if (at_menu) return;
    render();
  }
}

Ticker camera_poll_ticker(camera_poll,FRAME_TIMEOUT,0,MILLIS);
void take_photo(){
  
}

void button_press(){
  M5Cardputer.Speaker.tone(CAMERA_PRESS,100);
  current_settings = &photo_settings;
  apply_modeset();
  holding_shutter = true;
}
void button_release(){
  M5Cardputer.Speaker.tone(CAMERA_RELEASE,100);
  current_settings = &viewfinder_settings;
  apply_modeset();
  holding_shutter = false;
}

// TODO: make ticker start and stop during disconects, and make it faster for a cooler effect.
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  canvas.setBaseColor(BLACK);
  M5Cardputer.Display.setFont(UI_FONT);
  canvas.createSprite(240, 135);
  Serial.begin(9600);
  Serial2.setRxBufferSize(40000);
  Serial2.begin(2000000,SERIAL_8N1,13,15);
  camera_poll_ticker.start();
  g0Button.setPressMs(0);
  g0Button.attachLongPressStart(button_press);
  g0Button.attachLongPressStop(button_release);
  keyHandler.SetupKeyboardCallback(OnKey);

  menu.begin(&canvas,OnUsage);
  menu.setTheme(&theme);
  menu.goToMenu(&main_menu);

  M5Cardputer.Speaker.tone(BEEP_1,100);
  delay(100);
  M5Cardputer.Speaker.tone(BEEP_2,100);
  apply_modeset();
  RequestFrame();
}

void loop() {
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();
  CameraTick();
  camera_poll_ticker.update();
  g0Button.tick();
}