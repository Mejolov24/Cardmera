# Cardmera
Turn your cardputer into a camera with an ESP32-CAM

![thumbnail](https://github.com/Mejolov24/Cardmera/blob/main/Screenshots/UI.jpeg?raw=true)

Features and TODO:
- [x] Photos
- [ ] Videos
- [ ] Gallery
- [x] Custom effects
- [ ] Direct Write (Double buffering)

## Controls
ESC: Flash toggle
G0: Hold to preview resolution and flash, release for taking the photo.
OPT: open settings
## Settings
### Viewfinder, Photo and Video Settings
- Resolution
- JPEG Quality 0-63 (the *higher*, the worse quality)
- Direct Write (Disabled preview and writes directly to SD)
### Global Settings
- Brightness : -2/2
- Contrast : -2/2
- Sharpness : -2/2
- Auto White Balance : On/Off
- White Balance Mode : 0-4
- Auto WB Gain : On/Off
- Auto Exposure : On/Off
- Exposure Bias : -2/2
- Exposure Speed : Fast (200), Short (400), Normal (600), Long (800), Longer (1000)
- Enhanced Exposure : On/Off
- Manual Gain : 0/30
- Gain Ceiling : 0/6
- Denoise : 0/8
- Bad Pixel Correction : On/Off
- Lens Correction : On/Off
- White Pixel Correction : On/Off
- Downsampling : On/Off
- Mirror : On/Off
- Flip : On/Off
- Color Effect 0/6
### Other Settings
- Circuit Bending Tweaks (0/100 Values for effects)
- Circuit Bending (Layers and applied CB effects) *due to hardware limitations, photos with effects are just a screenshot of the screen buffer.*
- Volume : 0/100
- Charging Mode (Turns Off Screen, WARN: doesnt turn off camera power, unplug it please)