#include <Arduino.h>
#include <M5Cardputer.h>
#include <M5CADVKeyCB.h>
#include <M5Menu.h>
#include <Menu.h>
#include <OneButton.h>
#include <Ticker.h>
#include <SdFat.h>
#include <effects.h>
SdFat sd;
FsFile current_file;
OneButton g0Button(0, true);
M5CADVKeyCB keyHandler;
M5Menu menu;
M5Canvas canvas(&M5.Lcd);

bool force_flash = false;

ReceiveState rx_state = WAIT_SYNC_1;
uint32_t jpeg_length = 0;
uint32_t frame_bytes_read = 0;

#define MAX_IMG_SIZE 150000
#define BUFFER_A_START 0
#define BUFFER_A_END ((MAX_IMG_SIZE - 1) /2)
#define BUFFER_B_START ((MAX_IMG_SIZE) /2)
#define BUFFER_B_END MAX_IMG_SIZE
#define ACTION_WRITE_SD_BUFFER 1
#define ACTION_WRITE_SD_BUFFER_A 2
#define ACTION_WRITE_SD_BUFFER_B 3
uint8_t video_buffer[MAX_IMG_SIZE];

unsigned long lastFrameTime = 0;
const unsigned long FRAME_TIMEOUT = 500;
uint8_t frame_await_queque = 0;

bool at_menu = false;
bool holding_shutter = false;
bool IsSDValid = true;
bool out_of_memory = false;
uint16_t sd_files_amount = 0;
localCameraSettings* current_settings = &viewfinder_settings;
TaskHandle_t SDHandlerTask;

enum FrameState {
  FRAME_DISABLED,
  FRAME_NORMAL,
  FRAME_AWAIT_PHOTO,
};
FrameState _frame_state = FRAME_NORMAL;
enum Mode{
  MODE_PHOTO,
  MODE_VIDEO
};
Mode current_mode = MODE_PHOTO;
void modeSetSend(modetype mode,int8_t value = 0){
Serial2.write(0xFF);
Serial2.write(mode);
Serial2.write(static_cast<uint8_t>(value));
}

void RequestFrame(){
  rx_state = WAIT_SYNC_1;
  frame_bytes_read = 0;
  modeSetSend(REQUEST_FRAME);
}

void SetFrameState(FrameState state){
  _frame_state = state;
  switch (state)
  {
  case FRAME_DISABLED:
    rx_state = WAIT_SYNC_1;
    frame_bytes_read = 0;
    while (Serial2.available()) {Serial2.read();}
    break;
    break;
  
  default:
    break;
  }
}

void resolution_modeset(localCameraSettings* mode){
  current_settings = mode;
  SetFrameState(FRAME_DISABLED);
  modeSetSend(FR_SIZE,current_settings->FR_SIZE);
  modeSetSend(JPEG_QUALITY,current_settings->JPEG_QUALITY);
  SetFrameState(FRAME_NORMAL);
  RequestFrame();
}

void force_modeset(){
  // discard old camera data
  SetFrameState(FRAME_DISABLED);
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
  modeSetSend(AGC_GAIN,global_settings.AGC_GAIN);
  if(global_settings.GAIN_CEILING != 0) modeSetSend(GAIN_CEILING,global_settings.GAIN_CEILING);
  modeSetSend(AGC_GAIN,global_settings.AGC_GAIN);
  modeSetSend(DENOISE,global_settings.DENOISE);
  modeSetSend(RAW_GMA,global_settings.RAW_GMA);
  modeSetSend(BPC,global_settings.BPC);
  modeSetSend(WPC,global_settings.WPC);
  modeSetSend(DCW,global_settings.DCW);
  modeSetSend(LENS_CORR,global_settings.LENS_CORR);
  modeSetSend(MIRROR,global_settings.MIRROR);
  modeSetSend(FLIP,global_settings.FLIP);
  modeSetSend(SPECIAL,global_settings.SPECIAL);
  SetFrameState(FRAME_NORMAL);
  RequestFrame();
}

void OnUsage(M5Menu::MenuItem* item, M5Menu::Menu* menu){
    }

void OnKey(uint8_t key, bool pressed){
  Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();
  if (status.opt){
    at_menu = !at_menu;
    at_menu ? menu.open() : menu.close();
  }
  if (status.del){menu.process_input(M5Menu::Input::BACK);}
  if (status.enter) menu.process_input(M5Menu::Input::SELECT);
  if(!pressed) return;
  switch (key){

    case 53: // escape
    force_flash = !force_flash;
    modeSetSend(FLASH,force_flash);
    M5Cardputer.Speaker.tone((force_flash ? BEEP_1 : BEEP_2),100);
    break;
    
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

void update_SD_Count(){
  if(!IsSDValid) return;
  sd_files_amount = 0;
  FsFile root;
  FsFile file;

  if (!root.open("/DCMI")) return;

  while (file.openNext(&root, O_RDONLY)) {
    if (!file.isDir()) {
        sd_files_amount++;
    }
    file.close();
  }
  root.close();
}

void SDTask(void *pvParameters){
  uint32_t notificationValue = 0;
  char filename[256];
  for(;;){
      if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notificationValue, portMAX_DELAY) == pdPASS) {
        switch (current_mode)
        {
        case MODE_PHOTO:
        snprintf(filename, sizeof(filename), "/DCMI/Photo_%u.jpeg", sd_files_amount);
          break;
        
        default: break;
        }

        switch (notificationValue)
        {
        
        case ACTION_WRITE_SD_BUFFER:{
            current_file = sd.open(filename, O_WRITE | O_CREAT | O_TRUNC);
            current_file.write(video_buffer,jpeg_length);
            current_file.close();
            update_SD_Count();
            SetFrameState(FRAME_NORMAL);
            RequestFrame();
        }
          break;

        case ACTION_WRITE_SD_BUFFER_A:
          break;
        case ACTION_WRITE_SD_BUFFER_B:
          break;
        
        default: break;
        }
      }
  }
}

void SDWrite(){
  SetFrameState(FRAME_DISABLED);
  if(global_settings.direct_write){}
  else{xTaskNotify(SDHandlerTask, ACTION_WRITE_SD_BUFFER, eSetValueWithOverwrite);}
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

void take_photo(){
  SetFrameState(FRAME_NORMAL);
  frame_await_queque = 0;
  M5Cardputer.Speaker.tone(CAMERA_RELEASE,30);
  if(!IsSDValid) {resolution_modeset(&viewfinder_settings); return;}
  if(global_settings.direct_write){SetFrameState(FRAME_DISABLED);}
  SDWrite();
  resolution_modeset(&viewfinder_settings);
}

void camera_poll(){
  if(at_menu) return;
  if (millis() - lastFrameTime > FRAME_TIMEOUT){
    RequestFrame();
    renderStatic();
    canvas.setFont(NO_SIGNAL_FONT);
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(ORANGE,BLACK);
    canvas.drawString("No video",M5.Display.width()/2,M5.Display.height()/2);
    if (at_menu) return;
    render();
  }
}
Ticker camera_poll_ticker(camera_poll,FRAME_TIMEOUT,0,MILLIS);

void CameraTick(){
  if (at_menu) return;
  if (_frame_state == FRAME_DISABLED) return;
  while (Serial2.available() > 0) {
  switch (rx_state)
    {
    case WAIT_SYNC_1:{if (Serial2.read() == 0xAA){ rx_state = WAIT_SYNC_2;}break;}
    case WAIT_SYNC_2:{if (Serial2.read() == 0xBB){ rx_state = READ_LENGTH;} else{ rx_state = WAIT_SYNC_1;} break;}
    case READ_LENGTH:{
      if (!(Serial2.available() >= 4)) break;
      Serial2.readBytes((uint8_t*)&jpeg_length, 4);
      if (jpeg_length == 0) rx_state = WAIT_SYNC_1; // Corrupted header, restart
      if(jpeg_length > MAX_IMG_SIZE){
        canvas.setTextDatum(middle_center);
        canvas.setTextColor(RED,BLACK);
        canvas.setFont(NO_SIGNAL_FONT);
        canvas.drawString("Out of Memory!",M5.Display.width()/2,M5.Display.height()/2 - 14);
        canvas.drawString("Enable DW!",M5.Display.width()/2,M5.Display.height()/2 + 14);
        render();
        M5Cardputer.Speaker.tone(BEEP_1,100);
        delay(110);
        M5Cardputer.Speaker.tone(BEEP_1,100);
        delay(110);
        M5Cardputer.Speaker.tone(BEEP_1,200);
        resolution_modeset(&viewfinder_settings);
      }
      else {
        rx_state = READ_PAYLOAD;
        frame_bytes_read = 0;
      }
      break;
    }
    case READ_PAYLOAD:{   
        int bytes_available = Serial2.available();
        int bytes_to_read = min(bytes_available, (int)(jpeg_length - frame_bytes_read));
        Serial2.readBytes(&video_buffer[frame_bytes_read], bytes_to_read);
        frame_bytes_read += bytes_to_read;
        lastFrameTime = millis();
        if (frame_bytes_read < jpeg_length) return;

        Resolution r = frameSizes[current_settings->FR_SIZE];
        float scale = min((float)canvas.width() / r.w, (float)canvas.height() / r.h);
        int x = (canvas.width() - (r.w * scale)) / 2;
        int y = (canvas.height() - (r.h * scale)) / 2;

        canvas.drawJpg(video_buffer, jpeg_length, x, y,0,0,0, scale,scale);
        if (global_settings.invert_rgb) invert_rgb(x, y, r.w * scale, r.h * scale);
        if (global_settings.invert_endians) invert_endians(x, y, r.w * scale, r.h * scale);

        // Reset state machine and request the next frame
        if (_frame_state == FRAME_AWAIT_PHOTO){
          frame_await_queque ++;
          if(frame_await_queque >= global_settings.frame_await) take_photo();
          break;
        }
        RequestFrame();
        render();
        break;
    }
    default:
      break;
    }
  }
}

void button_press(){
  M5Cardputer.Speaker.tone(CAMERA_PRESS,30);
  if(!global_settings.direct_write) resolution_modeset(&photo_settings);
  frame_await_queque = 0;
  holding_shutter = true;
}
void button_release(){
  SetFrameState(FRAME_AWAIT_PHOTO);
  holding_shutter = false;
}

// TODO: make ticker start and stop during disconects, and make it faster for a cooler effect.
void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  xTaskCreatePinnedToCore(SDTask ,"SDHandlerTask",4096,NULL,0,&SDHandlerTask,0);

  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
  sd.begin(SD_SPI_CS_PIN, SD_SCK_MHZ(25));
  IsSDValid = (sd.card() != nullptr && sd.card()->errorCode() == 0);
  sd.mkdir("/AppData");
  sd.mkdir("/AppData/Cardmera");
  sd.mkdir("/DCMI");
  update_SD_Count();
  Serial.begin(9600);
  canvas.setBaseColor(BLACK);
  M5Cardputer.Display.setFont(UI_FONT);
  canvas.createSprite(240, 135);
  Serial2.setRxBufferSize(40000);
  Serial2.begin(2000000,SERIAL_8N1,13,15);
  camera_poll_ticker.start();
  g0Button.setPressMs(0);
  g0Button.attachLongPressStart(button_press);
  g0Button.attachLongPressStop(button_release);
  keyHandler.SetupKeyboardCallback(OnKey);

  menu.begin(&canvas,render,OnUsage);
  menu.setTheme(&theme);
  menu.goToMenu(&main_menu);
  set_volume();
  M5Cardputer.Speaker.tone(BEEP_1,100);
  delay(100);
  M5Cardputer.Speaker.tone(BEEP_2,100);
  force_modeset();

}

void loop() {
  M5Cardputer.update();
  keyHandler.KeyboardUpdate();
  CameraTick();
  camera_poll_ticker.update();
  g0Button.tick();
}