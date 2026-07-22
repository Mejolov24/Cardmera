#ifndef EFFECTS_H
    #define EFFECTS_H
#include <stdint.h>
#include <M5GFX.h>
extern M5Canvas canvas;

uint32_t fast_noise() {
    static uint32_t state = 0xACE1u; // Seed
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void invert_rgb(int x, int y, int w, int h) {
  uint16_t* pixels = (uint16_t*)canvas.getBuffer();

  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {

      int i = yy * canvas.width() + xx;

      uint16_t p = pixels[i];

      uint16_t r = (p >> 11) & 0x1F;
      uint16_t g = (p >> 5)  & 0x3F;
      uint16_t b = p & 0x1F;

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

      pixels[i] = (p >> 8) | (p << 8);
    }
  }
}
#endif