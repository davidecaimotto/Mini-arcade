/*
  =====================================================================
   MINI ARCADE  -  6 one-button games with polished graphics
   ESP32-C3 SuperMini + 0.96" OLED display (SSD1306 128x64, I2C)
  =====================================================================

  CONTROLS (one button: external button on GPIO 1 or the BOOT button)
    Start screen  : press to enter
    Cover menu    : short tap = next game
                    hold = play (the bar fills up)
    During a game : hold 1.5 s = back to the menu
                    (4 s in LANDER, where you hold to brake)
    Game over     : short tap = play again   |   hold = menu

  GAMES
    FLAPPY  : tap to flap your wings and fly between the pipes
    TURBO   : perspective race; tap to switch lanes
    ROOFTOP : run across the rooftops; tap to jump over crates and gaps
    LANDER  : hold to fire the rockets and land gently
    DINO    : tap to jump over the cacti (day and night take turns)
    STACK   : tap to drop the block and build the tower

  Libraries (Library Manager): "Adafruit SSD1306" and "Adafruit GFX Library"

  WIRING
    Display GND -> GND      Display VCC -> 3V3
    Display SCL -> GPIO 4   Display SDA -> GPIO 3
    Button      -> GPIO 1 and GND (shared with the display)
*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>

// ================== HARDWARE SETTINGS ==================
#define OLED_ADDR 0x3C       // if the screen stays black, try 0x3D
#define PIN_SDA   3          // GPIO 3 -> display SDA
#define PIN_SCL   4          // GPIO 4 -> display SCL
#define BTN_PIN   1          // GPIO 1 -> external button (other leg to GND)
#define BTN_BOOT  9          // GPIO 9 = the board's BOOT button

const int      SCR_W = 128, SCR_H = 64;
const uint16_t INK = SSD1306_WHITE, PAPER = SSD1306_BLACK;   // pixel on / off
const uint32_t FRAME_MS = 33;      // ~30 frames per second
const uint32_t LONG_MS  = 550;     // long press
const uint32_t EXIT_MS  = 1500;    // hold time to leave a game

Adafruit_SSD1306 oled(SCR_W, SCR_H, &Wire, -1);
GFXcanvas1 fb(SCR_W, SCR_H);             // everything is drawn here, then copied to the display
GFXcanvas1 fb2(SCR_W, SCR_H);            // second page for menu transitions
Preferences prefs;

// ---- generated sprites (1 = pixel on) ----
const int BIRD_MID_W = 13, BIRD_MID_H = 11;
const uint8_t BIRD_MID[] PROGMEM = { 0x00, 0x00, 0x1F, 0x80, 0x7F, 0xE0, 0xFE, 0x70, 0xFE, 0xF0, 0xFF, 0xF8, 0x87, 0x80, 0xFF, 0xF0, 0x7F, 0xE0, 0x1F, 0x80, 0x00, 0x00 };
const int BIRD_UP_W = 13, BIRD_UP_H = 11;
const uint8_t BIRD_UP[] PROGMEM = { 0xC0, 0x00, 0xAF, 0xC0, 0x5F, 0xF0, 0x9F, 0x38, 0xBF, 0x78, 0xDF, 0xF8, 0xBF, 0x80, 0xFF, 0xF0, 0x7F, 0xE0, 0x1F, 0x80, 0x00, 0x00 };
const int BIRD_DOWN_W = 13, BIRD_DOWN_H = 11;
const uint8_t BIRD_DOWN[] PROGMEM = { 0x00, 0x00, 0x1F, 0x80, 0x7F, 0xE0, 0xFE, 0x70, 0xFE, 0xF0, 0xFF, 0xF8, 0xBF, 0x80, 0xDF, 0xF0, 0xAF, 0xE0, 0x5F, 0xC0, 0xC0, 0x00 };
const int CAR_W = 18, CAR_H = 9;
const uint8_t CAR[] PROGMEM = { 0x03, 0xFC, 0x00, 0x06, 0x46, 0x00, 0x0E, 0x43, 0x00, 0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xC0, 0xE7, 0xF9, 0xC0, 0xC3, 0xF0, 0xC0, 0x18, 0x06, 0x00, 0x00, 0x00, 0x00 };
const int CAR_BACK_W = 24, CAR_BACK_H = 14;
const uint8_t CAR_BACK[] PROGMEM = { 0x03, 0xFF, 0xC0, 0x06, 0x00, 0x60, 0x0C, 0x00, 0x30, 0x18, 0x00, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE1, 0xFF, 0x87, 0xE1, 0xFF, 0x87, 0xFF, 0xFF, 0xFF, 0xFE, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0xF0, 0x00, 0x3C, 0xF0, 0x00, 0x3C, 0x00, 0x00, 0x00 };
const int RUN1_W = 8, RUN1_H = 12;
const uint8_t RUN1[] PROGMEM = { 0x0E, 0x0E, 0x0C, 0x3C, 0x5D, 0x1C, 0x18, 0x3C, 0x64, 0x46, 0x82, 0x80 };
const int RUN2_W = 8, RUN2_H = 12;
const uint8_t RUN2[] PROGMEM = { 0x0E, 0x0E, 0x0C, 0x3C, 0x5D, 0x1C, 0x18, 0x18, 0x18, 0x28, 0x24, 0x36 };
const int RUNJ_W = 8, RUNJ_H = 12;
const uint8_t RUNJ[] PROGMEM = { 0x0E, 0x8E, 0x4C, 0x3E, 0x1D, 0x1C, 0x18, 0x3E, 0x62, 0x82, 0x00, 0x00 };
const int DINO1_W = 12, DINO1_H = 12;
const uint8_t DINO1[] PROGMEM = { 0x03, 0xE0, 0x06, 0xF0, 0x07, 0xF0, 0x07, 0x00, 0x8F, 0xE0, 0x9F, 0x00, 0xFF, 0x00, 0x7F, 0x00, 0x3F, 0x00, 0x13, 0x00, 0x11, 0x00, 0x19, 0x80 };
const int DINO2_W = 12, DINO2_H = 12;
const uint8_t DINO2[] PROGMEM = { 0x03, 0xE0, 0x06, 0xF0, 0x07, 0xF0, 0x07, 0x00, 0x8F, 0xE0, 0x9F, 0x00, 0xFF, 0x00, 0x7F, 0x00, 0x3F, 0x00, 0x1A, 0x00, 0x12, 0x00, 0x33, 0x00 };
const int DINOJ_W = 12, DINOJ_H = 12;
const uint8_t DINOJ[] PROGMEM = { 0x03, 0xE0, 0x06, 0xF0, 0x07, 0xF0, 0x07, 0x00, 0x8F, 0xE0, 0x9F, 0x00, 0xFF, 0x00, 0x7F, 0x00, 0x3F, 0x00, 0x33, 0x00, 0x61, 0x80, 0x00, 0x00 };
const int LANDER_W = 11, LANDER_H = 10;
const uint8_t LANDER[] PROGMEM = { 0x1F, 0x00, 0x31, 0x80, 0x2A, 0x80, 0x3F, 0x80, 0x7F, 0xC0, 0xFF, 0xE0, 0x20, 0x80, 0x40, 0x40, 0x40, 0x40, 0xE0, 0xE0 };
const int TROPHY_W = 7, TROPHY_H = 7;
const uint8_t TROPHY[] PROGMEM = { 0xFE, 0xBA, 0x7C, 0x38, 0x10, 0x38, 0x7C };
const int STAR_W = 5, STAR_H = 5;
const uint8_t STAR[] PROGMEM = { 0x20, 0x70, 0xF8, 0x70, 0x50 };

// =====================================================================
//  GLOBAL STATE
// =====================================================================
enum State  { SPLASH, MENU, SLIDE, LAUNCH, PLAYING, DYING, OVER };
enum GameId { G_FLAPPY = 0, G_TURBO, G_ROOF, G_LANDER, G_DINO, G_STACK, G_COUNT };
const char* const TITLES[G_COUNT] = { "FLAPPY", "TURBO", "ROOFTOP", "LANDER", "DINO", "STACK" };
const char* const HINTS[G_COUNT]  = { "tap: flap", "tap: lane", "tap: jump", "hold: thrust", "tap: jump", "tap: drop" };

State    state = SPLASH;
int      sel = 0, cur = 0, slideTo = 0;
uint32_t slideBaseT = 0;
uint32_t score = 0, hi[G_COUNT] = {0};
uint32_t frameCount = 0, stateFrames = 0, gameStartMs = 0, overMs = 0, lastFrame = 0;
bool     newRecord = false;
int      shakeT = 0, flashT = 0;
bool     invMode = false, invState = false;

// ================== BUTTON ==================
struct Button {
  bool down = false, pressed = false, shortTap = false, longHold = false, longFired = false;
  uint32_t pressStart = 0;
};
Button btn;

void readButton(uint32_t now) {
  bool raw = (digitalRead(BTN_PIN) == LOW) || (digitalRead(BTN_BOOT) == LOW);
  btn.pressed = false; btn.shortTap = false; btn.longHold = false;
  if (raw && !btn.down) {
    btn.down = true; btn.pressed = true; btn.longFired = false; btn.pressStart = now;
  } else if (raw && btn.down) {
    if (!btn.longFired && now - btn.pressStart >= LONG_MS) { btn.longHold = true; btn.longFired = true; }
  } else if (!raw && btn.down) {
    btn.down = false;
    if (!btn.longFired) btn.shortTap = true;
  }
}

// =====================================================================
//  DRAWING HELPERS
// =====================================================================
float frand(float a, float b) { return a + (b - a) * (random(10001) / 10000.0f); }

// dither patterns (simulated grey): 1 = 12%, 2 = 25%, 3 = 50%, 4 = 75%
bool pat(int x, int y, int lvl) {
  x &= 3; y &= 3;
  switch (lvl) {
    case 1: return (x == 0 && y == 0) || (x == 2 && y == 2);
    case 2: return ((x & 1) == 0) && ((y & 1) == 0);
    case 3: return ((x + y) & 1) == 0;
    case 4: return !(((x & 1) == 1) && ((y & 1) == 1));
  }
  return false;
}

void dith(Adafruit_GFX &g, int x, int y, int w, int h, int lvl, int ax) {
  int x0 = max(x, 0), x1 = min(x + w, SCR_W), y0 = max(y, 0), y1 = min(y + h, SCR_H);
  for (int yy = y0; yy < y1; yy++)
    for (int xx = x0; xx < x1; xx++)
      if (pat(xx + ax, yy, lvl)) g.drawPixel(xx, yy, INK);
}

int textW(const char *s, int size) { int n = strlen(s); return n ? n * 6 * size - size : 0; }

void txt(Adafruit_GFX &g, const char *s, int x, int y, int size, uint16_t c) {
  g.setTextWrap(false); g.setTextSize(size); g.setTextColor(c); g.setCursor(x, y); g.print(s);
}
void txtC(Adafruit_GFX &g, const char *s, int cx, int y, int size, uint16_t c) { txt(g, s, cx - textW(s, size) / 2, y, size, c); }

// white text with a black outline: readable over any background
void txtOut(Adafruit_GFX &g, const char *s, int x, int y, int size) {
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++)
      if (dx || dy) txt(g, s, x + dx, y + dy, size, PAPER);
  txt(g, s, x, y, size, INK);
}
void txtOutC(Adafruit_GFX &g, const char *s, int cx, int y, int size) { txtOut(g, s, cx - textW(s, size) / 2, y, size); }

// big title: thick black outline + white letters
void title(Adafruit_GFX &g, const char *s, int cx, int y) {
  int x = cx - textW(s, 2) / 2;
  for (int dy = -2; dy <= 2; dy++)
    for (int dx = -2; dx <= 2; dx++)
      if (dx || dy) txt(g, s, x + dx, y + dy, 2, PAPER);
  txt(g, s, x, y, 2, INK);
  txt(g, s, x + 1, y, 2, INK);          // bolder letters
}

void spr(Adafruit_GFX &g, int x, int y, const uint8_t *b, int w, int h) { g.drawBitmap(x, y, b, w, h, INK); }

// sprite with a black outline, to stand out from the background
void sprOut(Adafruit_GFX &g, int x, int y, const uint8_t *b, int w, int h) {
  g.drawBitmap(x - 1, y, b, w, h, PAPER); g.drawBitmap(x + 1, y, b, w, h, PAPER);
  g.drawBitmap(x, y - 1, b, w, h, PAPER); g.drawBitmap(x, y + 1, b, w, h, PAPER);
  g.drawBitmap(x, y, b, w, h, INK);
}

bool sprBit(const uint8_t *b, int w, int sx, int sy) {
  int bpr = (w + 7) / 8;
  return pgm_read_byte(b + sy * bpr + sx / 8) & (0x80 >> (sx & 7));
}

// scaled sprite (for perspective and menu covers)
void sprScale(Adafruit_GFX &g, int x, int y, const uint8_t *b, int w, int h, float s, bool outline) {
  int dw = max(1, (int)(w * s + 0.5f)), dh = max(1, (int)(h * s + 0.5f));
  for (int pass = outline ? 0 : 1; pass < 2; pass++) {
    for (int j = 0; j < dh; j++) {
      int sy = j * h / dh;
      for (int i = 0; i < dw; i++) {
        int sx = i * w / dw;
        if (!sprBit(b, w, sx, sy)) continue;
        if (pass == 0) {
          g.drawPixel(x + i - 1, y + j, PAPER); g.drawPixel(x + i + 1, y + j, PAPER);
          g.drawPixel(x + i, y + j - 1, PAPER); g.drawPixel(x + i, y + j + 1, PAPER);
        } else {
          g.drawPixel(x + i, y + j, INK);
        }
      }
    }
  }
}

// outlined cloud
void cloud(Adafruit_GFX &g, int x, int y) {
  g.fillCircle(x + 5, y + 5, 4, INK); g.fillCircle(x + 11, y + 3, 5, INK); g.fillCircle(x + 17, y + 5, 4, INK);
  g.fillRect(x + 5, y + 5, 13, 5, INK);
  g.fillCircle(x + 5, y + 5, 3, PAPER); g.fillCircle(x + 11, y + 3, 4, PAPER); g.fillCircle(x + 17, y + 5, 3, PAPER);
  g.fillRect(x + 5, y + 5, 13, 4, PAPER);
}

// ================== PARTICLES ==================
struct Part { float x, y, vx, vy, gr; int16_t life; uint8_t kind, w, h; };
const int NP = 48;
Part parts[NP];

void partsClear() { for (int i = 0; i < NP; i++) parts[i].life = 0; }

// kind: 0 = dot, 1 = small square, 2 = spark, 3 = rectangular chip, 4 = star
void addPart(float x, float y, float vx, float vy, int life, int kind, float gr) {
  int k = -1;
  for (int i = 0; i < NP; i++) if (parts[i].life <= 0) { k = i; break; }
  if (k < 0) k = random(NP);
  parts[k].x = x; parts[k].y = y; parts[k].vx = vx; parts[k].vy = vy; parts[k].gr = gr;
  parts[k].life = life; parts[k].kind = kind; parts[k].w = 1; parts[k].h = 1;
}

void addChip(float x, float y, float vx, int w, int h) {
  addPart(x, y, vx, -0.5f, 45, 3, 0.25f);
  for (int i = 0; i < NP; i++)
    if (parts[i].kind == 3 && parts[i].x == x && parts[i].y == y && parts[i].life == 45) { parts[i].w = w; parts[i].h = h; break; }
}

void burst(float x, float y, int n, float spd, int kind, float gr) {
  for (int i = 0; i < n; i++) {
    float a = frand(0, 6.283f), v = frand(spd * 0.4f, spd);
    addPart(x, y, cosf(a) * v, sinf(a) * v - spd * 0.3f, random(14, 30), kind, gr);
  }
}

void partsUpdate() {
  for (int i = 0; i < NP; i++) {
    Part &p = parts[i];
    if (p.life <= 0) continue;
    p.x += p.vx; p.y += p.vy; p.vy += p.gr; p.life--;
    if (p.y > SCR_H + 8 || p.x < -12 || p.x > SCR_W + 12) p.life = 0;
  }
}

void partsDraw(Adafruit_GFX &g) {
  for (int i = 0; i < NP; i++) {
    Part &p = parts[i];
    if (p.life <= 0) continue;
    int x = (int)p.x, y = (int)p.y;
    switch (p.kind) {
      case 0: g.drawPixel(x, y, INK); break;
      case 1: g.fillRect(x, y, 2, 2, INK); break;
      case 2:
        g.drawPixel(x, y, INK);
        if (p.life > 8) { g.drawPixel(x - 1, y, INK); g.drawPixel(x + 1, y, INK); g.drawPixel(x, y - 1, INK); g.drawPixel(x, y + 1, INK); }
        break;
      case 3:
        g.fillRect(x, y, p.w, p.h, PAPER); g.drawRect(x, y, p.w, p.h, INK);
        if (p.h > 2) dith(g, x + 1, y + 1, p.w - 2, p.h - 2, 2, 0);
        break;
      case 4: g.drawBitmap(x - 2, y - 2, STAR, STAR_W, STAR_H, INK); break;
    }
  }
}

// ================== EFFECTS AND DISPLAY OUTPUT ==================
void setInvert(bool v) { if (v != invState) { invState = v; oled.invertDisplay(v); } }

// fast copy of a page (row-major) into the display buffer (column pages)
void blit(GFXcanvas1 &src, int ox, int oy) {
  uint8_t *dst = oled.getBuffer();
  const uint8_t *s = src.getBuffer();
  for (int y = 0; y < SCR_H; y++) {
    int yy = y + oy;
    if (yy < 0 || yy >= SCR_H) continue;
    uint8_t bit = 1 << (yy & 7);
    uint8_t *row = dst + (yy >> 3) * SCR_W;
    for (int xb = 0; xb < SCR_W / 8; xb++) {
      uint8_t b = s[y * (SCR_W / 8) + xb];
      if (!b) continue;
      for (int k = 0; k < 8; k++) {
        if (!(b & (0x80 >> k))) continue;
        int xx = xb * 8 + k + ox;
        if (xx >= 0 && xx < SCR_W) row[xx] |= bit;
      }
    }
  }
}

void present() {
  int sx = 0, sy = 0;
  if (shakeT > 0) { sx = random(-2, 3); sy = random(-2, 3); shakeT--; }
  oled.clearDisplay();
  blit(fb, sx, sy);
  setInvert((flashT > 0) != invMode);
  if (flashT > 0) flashT--;
  oled.display();
}

void presentSlide(int off) {
  oled.clearDisplay();
  blit(fb, off, 0);
  blit(fb2, off + SCR_W, 0);
  setInvert(false);
  oled.display();
}

// dims the screen (for the game over panel)
void dimFb() {
  uint8_t *b = fb.getBuffer();
  for (int y = 0; y < SCR_H; y++) {
    uint8_t m = (y & 1) ? 0x55 : 0xAA;
    for (int xb = 0; xb < SCR_W / 8; xb++) b[y * (SCR_W / 8) + xb] &= m;
  }
}

void crashFx(float x, float y) {
  shakeT = 10; flashT = 2;
  burst(x, y, 14, 2.5f, 1, 0.15f);
  burst(x, y, 6, 1.5f, 2, 0.0f);
}

// ================== SHARED BACKGROUNDS ==================
uint8_t cityH[64];        // distant skyline (64 columns of 4 px = 256 px, repeating)
uint8_t starX[24], starY[24];

void genWorld() {
  int c = 0;
  while (c < 64) {
    int w = random(2, 6), h = random(6, 20);
    for (int i = 0; i < w && c < 64; i++) cityH[c++] = h;
  }
  for (int i = 0; i < 24; i++) { starX[i] = random(SCR_W); starY[i] = random(10, 48); }
}

// dithered distant skyline, standing on row "base"
void drawCity(Adafruit_GFX &g, float off, int base, int lvl, int scaleH) {
  int o = (int)off;
  int sh = o & 3;
  for (int i = 0; i <= 33; i++) {
    int col = ((i + (o >> 2)) % 64 + 64) % 64;
    int h = cityH[col] * scaleH / 4;
    int x = i * 4 - sh;
    dith(g, x, base - h, 4, h, lvl, o);
    for (int k = 0; k < 4; k += 2) g.drawPixel(x + k, base - h, INK);
  }
}

void drawStars(Adafruit_GFX &g, int maxY) {
  for (int i = 0; i < 24; i++) {
    if (starY[i] >= maxY) continue;
    if ((frameCount / 8 + i) % 11 == 0) continue;             // twinkle
    g.drawPixel(starX[i], starY[i], INK);
    if (i % 7 == 0 && (frameCount / 6 + i) % 4 == 0) {         // a few bigger stars
      g.drawPixel(starX[i] - 1, starY[i], INK); g.drawPixel(starX[i] + 1, starY[i], INK);
      g.drawPixel(starX[i], starY[i] - 1, INK); g.drawPixel(starX[i], starY[i] + 1, INK);
    }
  }
}

// HUD: score at the top right (small, outlined)
void hudScore(Adafruit_GFX &g, uint32_t v, const char *suffix) {
  char b[16];
  snprintf(b, sizeof b, "%lu%s", (unsigned long)v, suffix);
  txtOut(g, b, SCR_W - 1 - textW(b, 1), 1, 1);
}

// =====================================================================
//  GAME 1: FLAPPY  (tap = flap)
// =====================================================================
const int   F_X = 26, F_GROUND = 57, F_PW = 14, F_GAP = 30, F_SPACING = 66;
const float F_FLAP = -3.0f, F_GRAV = 0.3f;

float fY, fVy, fScroll, fPx[3];
int   fPg[3], fWingT;
bool  fPs[3], fStarted;

int fLastGap = 30;
void flappyPipe(int i, float x) {
  // the gap moves at most 16 px from the previous pipe: always reachable
  int g = fLastGap + random(-16, 17);
  if (g < 19) g = 19;
  if (g > 42) g = 42;
  fPx[i] = x; fPg[i] = g; fPs[i] = false; fLastGap = g;
}

void flappyInit() {
  fY = 26; fVy = 0; fScroll = 0; fStarted = false; fWingT = 0; fLastGap = 30;
  for (int i = 0; i < 3; i++) flappyPipe(i, SCR_W + 20 + i * F_SPACING);
}

bool flappyUpdate() {
  float speed = fminf(2.8f, 1.8f + score * 0.03f);
  fScroll += speed;
  if (fWingT > 0) fWingT--;
  if (!fStarted) {
    fY = 26 + sinf(frameCount * 0.15f) * 3;
    if (btn.pressed) { fStarted = true; fVy = F_FLAP; fWingT = 8; }
    return true;
  }
  if (btn.pressed) {
    fVy = F_FLAP; fWingT = 8;
    addPart(F_X, fY + 5, -1.2f, 0.3f, 8, 0, 0); addPart(F_X, fY + 7, -1.0f, 0.6f, 8, 0, 0);
  }
  fVy += F_GRAV;
  if (fVy > 4.5f) fVy = 4.5f;
  fY += fVy;
  if (fY < 0) { fY = 0; if (fVy < 0) fVy = 0; }

  for (int i = 0; i < 3; i++) {
    fPx[i] -= speed;
    if (!fPs[i] && fPx[i] + F_PW < F_X) {
      fPs[i] = true; score++;
      addPart(F_X + 6, fY - 2, 0, -0.6f, 16, 4, 0);
    }
    if (fPx[i] + F_PW + 2 < 0) {
      float m = 0;
      for (int j = 0; j < 3; j++) m = fmaxf(m, fPx[j]);
      flappyPipe(i, m + F_SPACING);
    }
  }

  int hx1 = F_X + 2, hx2 = F_X + 11, hy1 = (int)fY + 2, hy2 = (int)fY + 9;
  if (hy2 >= F_GROUND) { crashFx(F_X + 6, fY + 5); return false; }
  for (int i = 0; i < 3; i++) {
    if (hx2 > fPx[i] - 1 && hx1 < fPx[i] + F_PW + 1) {
      int top = fPg[i] - F_GAP / 2, bot = fPg[i] + F_GAP / 2;
      if (hy1 < top || hy2 > bot) { crashFx(F_X + 6, fY + 5); return false; }
    }
  }
  return true;
}

void pipeSeg(Adafruit_GFX &g, int x, int y, int h) {
  if (h <= 0) return;
  g.fillRect(x, y, F_PW, h, PAPER);
  dith(g, x + 1, y, F_PW - 2, h, 1, 0);
  g.drawFastVLine(x, y, h, INK); g.drawFastVLine(x + F_PW - 1, y, h, INK);
  g.fillRect(x + 2, y, 2, h, INK);                    // highlight
  for (int yy = y; yy < y + h; yy += 2) g.drawPixel(x + F_PW - 3, yy, INK);
}

void pipeCap(Adafruit_GFX &g, int x, int y) {
  g.fillRect(x - 2, y, F_PW + 4, 5, PAPER);
  g.drawRect(x - 2, y, F_PW + 4, 5, INK);
  g.fillRect(x, y + 1, 2, 3, INK);
  g.drawPixel(x + F_PW - 1, y + 2, INK);
}

void pipePair(Adafruit_GFX &g, int x, int gapC) {
  int top = gapC - F_GAP / 2, bot = gapC + F_GAP / 2;
  pipeSeg(g, x, 0, top - 5); pipeCap(g, x, top - 5);
  pipeCap(g, x, bot); pipeSeg(g, x, bot + 5, F_GROUND - bot - 5);
}

void flappyGround(Adafruit_GFX &g, float scroll) {
  g.fillRect(0, F_GROUND, SCR_W, SCR_H - F_GROUND, PAPER);
  g.drawFastHLine(0, F_GROUND, SCR_W, INK);
  g.drawFastHLine(0, F_GROUND + 2, SCR_W, INK);
  int s = (int)scroll;
  for (int y = F_GROUND + 3; y < SCR_H; y++)
    for (int x = 0; x < SCR_W; x++)
      if (((x + s + y) & 7) < 2) g.drawPixel(x, y, INK);
}

void flappySky(Adafruit_GFX &g, float scroll) {
  drawCity(g, scroll * 0.25f, F_GROUND, 1, 4);
  int o = (int)(scroll * 0.5f);
  for (int i = 0; i < 3; i++) {
    int x = ((i * 53 - o) % 160 + 160) % 160 - 24;
    cloud(g, x, 8 + (i * 11) % 18);
  }
}

void birdDraw(Adafruit_GFX &g, int x, int y, int wingT, bool idle) {
  const uint8_t *b = BIRD_MID;
  if (idle) {
    int f = (frameCount / 4) % 4;
    b = (f == 1) ? BIRD_UP : (f == 3) ? BIRD_DOWN : BIRD_MID;
  } else if (wingT > 4) b = BIRD_UP;
  else if (wingT > 0) b = BIRD_DOWN;
  sprOut(g, x, y, b, BIRD_MID_W, BIRD_MID_H);
}

void flappyDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  flappySky(g, fScroll);
  for (int i = 0; i < 3; i++) pipePair(g, (int)fPx[i], fPg[i]);
  flappyGround(g, fScroll);
  birdDraw(g, F_X, (int)fY - 1, fWingT, !fStarted);
  partsDraw(g);
  if (fStarted) {
    char b[12]; snprintf(b, sizeof b, "%lu", (unsigned long)score);
    txtOutC(g, b, SCR_W / 2, 3, 2);
  } else if ((frameCount / 10) % 3) {
    txtOutC(g, "tap to fly", SCR_W / 2, 44, 1);
  }
}

void flappyCover(Adafruit_GFX &g, uint32_t t) {
  g.fillScreen(PAPER);
  flappySky(g, t * 1.5f);
  pipePair(g, 108, 34);
  flappyGround(g, t * 1.5f);
  int f = (t / 4) % 4;
  const uint8_t *b = (f == 1) ? BIRD_UP : (f == 3) ? BIRD_DOWN : BIRD_MID;
  sprScale(g, 40, 27 + (int)(sinf(t * 0.15f) * 3), b, BIRD_MID_W, BIRD_MID_H, 2.0f, true);
}

// =====================================================================
//  GAME 2: TURBO  (perspective race, tap = switch lane)
// =====================================================================
const int   T_HY = 24;                 // horizon
const float T_PD = 1.03f;              // depth of your car
struct TCar { float d; int8_t lane; bool on; };
TCar  tCars[6];
float tDist, tSpd, tLp, tCurve, tBg, tLastD, tPendExtra;
int   tLane, tLastLane, tPendLane, tCount, tSunOff = 0;
uint8_t tMount[32];

void turboInit() {
  tDist = 0; tSpd = 0.07f; tLane = 1; tLp = 1; tCurve = 0; tBg = 0;
  tLastD = 0; tLastLane = 1; tPendLane = -1; tPendExtra = 10; tCount = 80;
  for (int i = 0; i < 6; i++) tCars[i].on = false;
  for (int i = 0; i < 32; i++) tMount[i] = random(3, 12);
}

float tP(int y) { return (y - T_HY) / 39.0f; }
float tHw(float p) { return 3 + 50 * p; }
float tCx(float p) { float q = 1 - p; return 64 + tCurve * 30 * q * q; }

bool turboUpdate() {
  if (tCount > 0) {
    tCount--;
    return true;
  }
  tSpd = fminf(0.14f, 0.07f + tDist / 2500.0f);
  tDist += tSpd;
  score = (uint32_t)(tDist * 8);
  tCurve = sinf(tDist * 0.045f) * 0.7f + sinf(tDist * 0.017f) * 0.3f;
  tBg += tCurve * tSpd * 8;

  if (btn.pressed) tLane = -tLane;
  if (tLp < tLane) tLp = fminf((float)tLane, tLp + 0.2f);
  if (tLp > tLane) tLp = fmaxf((float)tLane, tLp - 0.2f);

  for (int i = 0; i < 6; i++) {
    if (!tCars[i].on) continue;
    tCars[i].d -= tSpd;
    if (tCars[i].d < 0.6f) tCars[i].on = false;
  }
  tLastD -= tSpd;

  // new cars: one arriving in the other lane always leaves time to move over
  float need = (tPendLane != tLastLane ? 32 : 12) * tSpd + tPendExtra * tSpd;
  if (16.0f - tLastD >= need) {
    for (int i = 0; i < 6; i++) {
      if (tCars[i].on) continue;
      tCars[i].on = true; tCars[i].d = 16.0f; tCars[i].lane = tPendLane;
      tLastD = 16.0f; tLastLane = tPendLane;
      tPendLane = random(2) ? 1 : -1;
      tPendExtra = random(0, 28);
      break;
    }
  }

  for (int i = 0; i < 6; i++) {
    if (!tCars[i].on) continue;
    if (tCars[i].d >= T_PD - 0.12f && tCars[i].d <= T_PD + 0.12f && fabsf(tLp - tCars[i].lane) < 0.75f) {
      crashFx(tCx(0.97f) + tLp * 0.5f * tHw(0.974f), 55);
      return false;
    }
  }
  if (frameCount % 3 == 0) {
    float px = tCx(0.974f) + tLp * 0.5f * tHw(0.974f);
    addPart(px - 7, 61, frand(-0.4f, 0.1f), frand(0.2f, 0.6f), 10, 0, 0);
    addPart(px + 7, 61, frand(-0.1f, 0.4f), frand(0.2f, 0.6f), 10, 0, 0);
  }
  return true;
}

void turboSky(Adafruit_GFX &g) {
  drawStars(g, T_HY - 4);
  int sx = 64 - (int)(tCurve * 10) + tSunOff;
  g.fillCircle(sx, T_HY - 1, 13, INK);
  int gaps[5] = { 2, 5, 8, 11, 14 };
  for (int k = 0; k < 5; k++) {
    int y = T_HY - gaps[k];
    int th = (k < 2) ? 2 : 1;
    g.fillRect(sx - 14, y, 29, th, PAPER);
  }
  // mountains
  int o = (int)tBg;
  for (int x = 0; x < SCR_W; x++) {
    int wx = x + o;
    int i0 = ((wx >> 3) % 32 + 32) % 32, i1 = (i0 + 1) % 32;
    float f = (wx & 7) / 8.0f;
    int h = (int)(tMount[i0] * (1 - f) + tMount[i1] * f);
    for (int y = T_HY - h; y < T_HY; y++) if (pat(wx, y, 2)) g.drawPixel(x, y, INK);
    g.drawPixel(x, T_HY - h, INK);
  }
  g.drawFastHLine(0, T_HY, SCR_W, INK);
}

void turboRoad(Adafruit_GFX &g) {
  g.fillRect(0, T_HY + 1, SCR_W, SCR_H - T_HY - 1, PAPER);
  for (int y = T_HY + 1; y < SCR_H; y++) {
    float p = tP(y), d = 1.0f / p;
    int cx = (int)tCx(p), hw = (int)tHw(p);
    bool seg = ((int)((d + tDist) * 1.3f)) & 1;
    int left = cx - hw, right = cx + hw;
    int gm = seg ? 3 : 7;                            // grass: two alternating densities
    for (int x = 0; x < left - 5; x++) if (((x + 2 * y) & gm) == 0) g.drawPixel(x, y, INK);
    for (int x = right + 6; x < SCR_W; x++) if (((x + 2 * y) & gm) == 0) g.drawPixel(x, y, INK);
    int rw = 1 + (int)(4 * p);
    if (seg) { g.drawFastHLine(left - rw, y, rw, INK); g.drawFastHLine(right + 1, y, rw, INK); }
    else { g.drawPixel(left - 1, y, INK); g.drawPixel(right + 1, y, INK); }
    if (!seg) { int cw = max(1, (int)(2.5f * p)); g.drawFastHLine(cx - cw / 2, y, cw, INK); }
  }
}

void turboCar(Adafruit_GFX &g, float d, int lane) {
  float p = 1.0f / d;
  if (p < 0.03f) return;
  int yb = T_HY + (int)(39 * p);
  float s = T_PD / d;
  float x = tCx(p) + lane * 0.5f * tHw(p);
  if (s < 0.2f) { g.fillRect((int)x - 1, yb - 2, 3, 2, INK); return; }
  sprScale(g, (int)(x - 12 * s), (int)(yb - 14 * s), CAR_BACK, CAR_BACK_W, CAR_BACK_H, s, true);
}

void turboDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  turboSky(g);
  turboRoad(g);
  int order[6] = { 0, 1, 2, 3, 4, 5 };
  for (int a = 0; a < 6; a++)
    for (int b = a + 1; b < 6; b++)
      if (tCars[order[b]].d > tCars[order[a]].d) { int t = order[a]; order[a] = order[b]; order[b] = t; }
  for (int k = 0; k < 6; k++) if (tCars[order[k]].on) turboCar(g, tCars[order[k]].d, tCars[order[k]].lane);
  int px = (int)(tCx(0.974f) + tLp * 0.5f * tHw(0.974f)) - 12;
  int py = 48 + ((tSpd > 0.1f && (frameCount & 2)) ? 1 : 0);
  sprOut(g, px, py, CAR_BACK, CAR_BACK_W, CAR_BACK_H);
  partsDraw(g);

  char b[16];
  snprintf(b, sizeof b, "%d km/h", tCount > 0 ? 0 : (int)(tSpd * 1700));
  txtOut(g, b, 1, 1, 1);
  hudScore(g, score, "");
  if (tCount > 0) {
    const char *c = tCount > 60 ? "3" : tCount > 40 ? "2" : tCount > 20 ? "1" : "GO!";
    txtOutC(g, c, SCR_W / 2, 26, 3);
  }
}

void turboCover(Adafruit_GFX &g, uint32_t t) {
  float sd = tDist, sc = tCurve, sb = tBg;
  tDist = t * 0.08f; tCurve = sinf(t * 0.02f) * 0.8f; tBg = t * 0.3f; tSunOff = 42;
  if (tMount[0] == 0) for (int i = 0; i < 32; i++) tMount[i] = 3 + (i * 7) % 9;
  g.fillScreen(PAPER);
  turboSky(g);
  turboRoad(g);
  turboCar(g, 2.2f, -1);
  turboCar(g, 6.0f, 1);
  sprOut(g, (int)(tCx(0.974f) + 0.5f * tHw(0.974f)) - 12, 40, CAR_BACK, CAR_BACK_W, CAR_BACK_H);
  tDist = sd; tCurve = sc; tBg = sb; tSunOff = 0;
}

// =====================================================================
//  GAME 3: ROOFTOP  (rooftop run, tap = jump)
// =====================================================================
const int   C_X = 24, C_W = 6, C_H = 10, C_CRATE_W = 8, C_CRATE_H = 6, C_BLD_N = 5;
const float C_GRAV = 0.55f, C_JUMP = 4.6f;
const int   C_DTOP[5] = { -6, 0, 0, 5, 9 };

struct CBld { float x; int w; int top; int crate; };
CBld  cB[C_BLD_N];
float cY, cVy, cSpeed, cDist;
bool  cGround, cFalling;
int   cCoyote;
uint8_t midW[10], midH[10];

void roofGen(int i, int prev) {
  float air = 16.7f * cSpeed;
  int maxGap = (int)(air * 0.7f);
  if (maxGap < 20) maxGap = 20;
  int gap = random(16, maxGap + 1);
  int top = cB[prev].top + C_DTOP[random(5)];
  if (top < 36) top = 36;
  if (top > 54) top = 54;
  cB[i].x = cB[prev].x + cB[prev].w + gap;
  cB[i].top = top;
  if (random(100) < 55) {
    int leftM = (int)(23 * cSpeed) - 6;
    int rightM = 20 + (int)(11 * cSpeed);
    int off = leftM + random(0, 20);
    cB[i].crate = off;
    cB[i].w = off + C_CRATE_W + rightM + random(0, 30);
  } else {
    int minW = (int)air + 4;
    cB[i].w = minW + random(0, 80);
    cB[i].crate = -1;
  }
}

void roofMidGen() {
  for (int i = 0; i < 10; i++) { midW[i] = random(18, 34); midH[i] = random(14, 30); }
}

void roofInit() {
  cSpeed = 3.0f; cDist = 0; cGround = true; cFalling = false; cCoyote = 0; cVy = 0;
  cB[0].x = -20; cB[0].w = 220; cB[0].top = 44; cB[0].crate = -1;
  cY = cB[0].top;
  for (int i = 1; i < C_BLD_N; i++) roofGen(i, i - 1);
  roofMidGen();
}

bool roofUpdate() {
  cSpeed = fminf(5.2f, 3.0f + cDist / 3000.0f);
  cDist += cSpeed;
  score = (uint32_t)(cDist / 15);

  for (int i = 0; i < C_BLD_N; i++) cB[i].x -= cSpeed;
  for (int i = 0; i < C_BLD_N; i++) {
    if (cB[i].x + cB[i].w < -6) {
      int prev = (i + 1) % C_BLD_N;
      for (int j = 0; j < C_BLD_N; j++) if (j != i && cB[j].x > cB[prev].x) prev = j;
      roofGen(i, prev);
    }
  }

  if (cGround) cCoyote = 3;
  else if (cCoyote > 0) cCoyote--;
  if (btn.pressed && (cGround || cCoyote > 0)) {
    cVy = -C_JUMP; cGround = false; cCoyote = 0;
    addPart(C_X + 1, cY - 1, -1.5f, -0.3f, 10, 0, 0);
    addPart(C_X + 4, cY - 1, -1.0f, -0.5f, 10, 0, 0);
  }

  float prevFeet = cY;
  if (!cGround) { cVy += C_GRAV; cY += cVy; }

  int px1 = C_X + 1, px2 = C_X + C_W - 2;
  bool supported = false;
  for (int i = 0; i < C_BLD_N; i++) {
    if (!(cB[i].x < px2 && cB[i].x + cB[i].w > px1)) continue;
    int top = cB[i].top;
    if (cGround) { cY = top; supported = true; }
    else if (cVy >= 0 && prevFeet <= top + 1 && cY >= top) {
      cY = top; cVy = 0; cGround = true; supported = true;
      for (int k = 0; k < 4; k++) addPart(C_X + 3, top - 1, frand(-1.8f, 0.6f), frand(-0.8f, -0.2f), 10, 0, 0.08f);
    } else if (cY > top + 3) {
      crashFx(C_X + 3, cY - 5);
      return false;
    }
  }
  if (cGround && !supported) { cGround = false; cVy = 0; }
  if (cY > SCR_H + C_H) return false;
  if (cGround && frameCount % 5 == 0) addPart(C_X + 1, cY - 1, -cSpeed * 0.4f, -0.2f, 6, 0, 0);

  for (int i = 0; i < C_BLD_N; i++) {
    if (cB[i].crate < 0) continue;
    int cx = (int)cB[i].x + cB[i].crate;
    int cy = cB[i].top - C_CRATE_H;
    bool hitX = (px2 >= cx) && (px1 <= cx + C_CRATE_W - 1);
    bool hitY = ((int)cY - 1 >= cy) && ((int)cY - C_H + 2 <= cy + C_CRATE_H - 1);
    if (hitX && hitY) { crashFx(cx + 2, cy + 3); return false; }
  }
  return true;
}

void roofBack(Adafruit_GFX &g, float dist, int my) {
  // moon
  g.fillCircle(104, my, 7, INK);
  g.fillCircle(101, my - 2, 1, PAPER); g.fillRect(106, my + 2, 2, 1, PAPER); g.drawPixel(103, my + 4, PAPER);
  // distant skyline
  drawCity(g, dist * 0.12f, SCR_H, 1, 8);
  // middle buildings with lit windows
  int o = (int)(dist * 0.35f);
  int tot = 0;
  for (int i = 0; i < 10; i++) tot += midW[i];
  if (tot <= 0) tot = 1;
  int x = -(o % tot);
  for (int rep = 0; rep < 3; rep++) {
    for (int i = 0; i < 10; i++) {
      int w = midW[i], h = midH[i] + 4;
      if (x < SCR_W && x + w > 0) {
        int top = SCR_H - h;
        g.fillRect(x, top, w - 2, h, PAPER);
        for (int xx = x; xx < x + w - 2; xx += 2) g.drawPixel(xx, top, INK);      // dotted roof line
        for (int yy = top; yy < SCR_H; yy += 2) { g.drawPixel(x, yy, INK); g.drawPixel(x + w - 3, yy, INK); }
        for (int wy = top + 3; wy < SCR_H - 2; wy += 4)
          for (int wx = x + 3; wx < x + w - 5; wx += 4)
            if (((wx - x) * 7 + wy * 13 + i * 5) % 6 == 0) g.drawPixel(wx, wy, INK);
        if (i % 3 == 0) {
          for (int yy = top - 5; yy < top; yy += 2) g.drawPixel(x + w / 2, yy, INK);
          if ((frameCount / 15) % 2) g.drawPixel(x + w / 2, top - 7, INK);
        }
      }
      x += w;
    }
  }
}

void roofBuilding(Adafruit_GFX &g, int x0, int w, int top, int crate) {
  int x1 = x0 + w;
  if (x1 < 0 || x0 > SCR_W - 1) return;
  g.fillRect(x0, top, w, SCR_H - top, PAPER);
  g.fillRect(x0, top, w, 2, INK);
  g.drawFastVLine(x0, top, SCR_H - top, INK);
  g.drawFastVLine(x1 - 1, top, SCR_H - top, INK);
  dith(g, x0 + 1, top + 2, w - 2, 2, 3, 0);                       // cornice
  for (int wy = top + 7; wy < SCR_H; wy += 7)
    for (int wx = x0 + 4; wx < x1 - 5; wx += 6) {
      if (wx < -3 || wx > SCR_W) continue;
      int hsh = ((wx - x0) * 31 + wy * 17 + w * 7 + top * 3) % 7;
      if (hsh < 3) g.fillRect(wx, wy, 3, 3, INK);
      else g.drawRect(wx, wy, 3, 3, INK);
    }
  if (crate >= 0) {
    int cx = x0 + crate, cy = top - C_CRATE_H;
    g.fillRect(cx, cy, C_CRATE_W, C_CRATE_H, INK);
    g.drawLine(cx + 1, cy + 1, cx + C_CRATE_W - 2, cy + C_CRATE_H - 2, PAPER);
    g.drawLine(cx + C_CRATE_W - 2, cy + 1, cx + 1, cy + C_CRATE_H - 2, PAPER);
  }
}

void runnerDraw(Adafruit_GFX &g, int x, int feet, bool ground) {
  const uint8_t *b = !ground ? RUNJ : ((frameCount / 3) % 2 ? RUN1 : RUN2);
  sprOut(g, x, feet - RUN1_H, b, RUN1_W, RUN1_H);
}

void roofDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  roofBack(g, cDist, 13);
  for (int i = 0; i < C_BLD_N; i++) roofBuilding(g, (int)cB[i].x, cB[i].w, cB[i].top, cB[i].crate);
  runnerDraw(g, C_X - 1, (int)cY, cGround);
  partsDraw(g);
  hudScore(g, score, "m");
}

void roofCover(Adafruit_GFX &g, uint32_t t) {
  if (midW[0] == 0) for (int i = 0; i < 10; i++) { midW[i] = 20 + (i * 7) % 14; midH[i] = 14 + (i * 11) % 16; }
  g.fillScreen(PAPER);
  roofBack(g, t * 2.0f, 30);
  roofBuilding(g, -10, 70, 48, -1);
  roofBuilding(g, 86, 60, 44, 20);
  float ph = (t % 40) / 40.0f;
  int jx = 34 + (int)(ph * 46);
  int jy = 46 - (int)(sinf(ph * 3.1416f) * 14);
  sprScale(g, jx, jy - 24, RUNJ, RUN1_W, RUN1_H, 2.0f, true);
}

// =====================================================================
//  GAME 4: LANDER  (hold = retro-rockets)
// =====================================================================
const float L_GRAV = 0.035f, L_THRUST = 0.09f, L_SAFE_VY = 1.1f;
const int   L_GROUND = 58;

float   lX, lY, lVy, lVxMag, lFuel, lFuelMax;
int     lDir, lPadL, lPadR, lPadW, lLevel, lLandT, lCrashT;
bool    lThrust, lPerfect, lCoverReady = false;
uint8_t lTer[SCR_W];

void landerTerrain() {
  int h = L_GROUND;
  for (int x = 0; x < SCR_W; x++) {
    h += random(-2, 3);
    if (h < 54) h = 54;
    if (h > 62) h = 62;
    lTer[x] = h;
  }
  for (int x = lPadL - 3; x <= lPadR + 3; x++) if (x >= 0 && x < SCR_W) lTer[x] = L_GROUND;
  for (int k = 1; k <= 10; k++) {
    float f = k / 10.0f;
    int xl = lPadL - 3 - k, xr = lPadR + 3 + k;
    if (xl >= 0) lTer[xl] = (uint8_t)(L_GROUND * (1 - f) + lTer[xl] * f);
    if (xr < SCR_W) lTer[xr] = (uint8_t)(L_GROUND * (1 - f) + lTer[xr] * f);
  }
}

void landerLevel() {
  lPadW = 26 - 2 * lLevel;            if (lPadW < 14) lPadW = 14;
  lVxMag = 0.7f + 0.04f * lLevel;     if (lVxMag > 1.0f) lVxMag = 1.0f;
  lFuelMax = 80 - 4 * lLevel;         if (lFuelMax < 45) lFuelMax = 45;
  lFuel = lFuelMax;
  lDir = random(2) ? 1 : -1;
  lY = 26; lVy = 0; lThrust = false;
  const int M = 8;
  float d = (L_GROUND - 1) - lY;
  float tFree = sqrtf(2 * d / L_GRAV);
  float dx = lVxMag * (tFree + 20 + random(0, 41));
  float maxDx = 127 - 2 * M - lPadW / 2.0f;
  if (dx > maxDx) dx = maxDx;
  int padC;
  if (lDir > 0) {
    int hiX = (int)(127 - M - lPadW / 2 - dx);
    if (hiX < M) hiX = M;
    lX = M + random(0, hiX - M + 1);
    padC = (int)(lX + dx);
  } else {
    int loX = (int)(M + lPadW / 2 + dx);
    if (loX > 127 - M) loX = 127 - M;
    lX = loX + random(0, 127 - M - loX + 1);
    padC = (int)(lX - dx);
  }
  lPadL = padC - lPadW / 2;
  lPadR = lPadL + lPadW - 1;
  landerTerrain();
}

void landerInit() { lLevel = 0; lLandT = 0; lCrashT = 0; lPerfect = false; lCoverReady = false; landerLevel(); }

bool landerUpdate() {
  if (lCrashT > 0) { lCrashT--; return lCrashT > 0; }
  if (lLandT > 0) {
    lLandT--;
    if (lLandT % 6 == 0) addPart(lX + frand(-6, 6), lY - 12, 0, -0.4f, 14, 4, 0);
    if (lLandT == 0) { lLevel++; landerLevel(); }
    return true;
  }
  lThrust = btn.down && btn.pressStart > gameStartMs && lFuel > 0;
  float ay = L_GRAV;
  if (lThrust) {
    ay -= L_THRUST; lFuel -= 1;
    addPart(lX + frand(-1.5f, 1.5f), lY + 1, frand(-0.4f, 0.4f) - lDir * lVxMag * 0.5f, frand(1.0f, 2.0f) + lVy, 7, 0, 0);
  }
  lVy += ay; lY += lVy; lX += lDir * lVxMag;
  if (lY < 22) { lY = 22; if (lVy < 0) lVy = 0; }
  if (lX < 4 || lX > 123) { lCrashT = 18; lThrust = false; crashFx(lX, lY - 4); return true; }

  int cx = (int)lX;
  bool touch = false;
  for (int dx = -4; dx <= 4; dx++) if (lY >= lTer[cx + dx] - 1) touch = true;
  if (touch) {
    lThrust = false;
    if (cx >= lPadL && cx <= lPadR && lVy <= L_SAFE_VY) {
      lPerfect = lVy < 0.5f;
      lY = L_GROUND - 1; lVy = 0; lLandT = 45;
      score += 100 + (uint32_t)(lFuel / 2) + (lPerfect ? 50 : 0);
      for (int k = 0; k < 6; k++) addPart(lX + frand(-5, 5), L_GROUND - 1, frand(-1.2f, 1.2f), frand(-0.6f, -0.1f), 12, 0, 0.05f);
    } else {
      lCrashT = 18;
      crashFx(lX, lY - 4);
      burst(lX, lY - 4, 10, 2.0f, 1, 0.12f);
    }
  }
  return true;
}

void landerBack(Adafruit_GFX &g) {
  drawStars(g, 50);
  // the Earth on the horizon, half in shadow
  g.fillCircle(112, 27, 6, INK);
  for (int y = 21; y <= 33; y++)
    for (int x = 106; x <= 111; x++)
      if (pat(x, y, 3)) g.drawPixel(x, y, PAPER);
  g.drawCircle(112, 27, 6, INK);
}

void landerGround(Adafruit_GFX &g) {
  for (int x = 0; x < SCR_W; x++) {
    int t = lTer[x];
    g.drawFastVLine(x, t + 1, SCR_H - t - 1, PAPER);
    for (int y = t + 1; y < SCR_H; y++) if (pat(x, y, (y - t) < 3 ? 3 : 1)) g.drawPixel(x, y, INK);
    g.drawPixel(x, t, INK);
  }
  g.fillRect(lPadL, L_GROUND, lPadW, 2, INK);
  dith(g, lPadL, L_GROUND + 2, lPadW, SCR_H - L_GROUND - 2, 3, 0);
  g.drawFastVLine(lPadL, L_GROUND - 6, 6, INK);
  g.drawFastVLine(lPadR, L_GROUND - 6, 6, INK);
  bool blink = (frameCount / 6) % 2;
  if (blink) { g.fillRect(lPadL - 1, L_GROUND - 8, 3, 2, INK); g.fillRect(lPadR - 1, L_GROUND - 8, 3, 2, INK); }
  else { g.drawPixel(lPadL, L_GROUND - 7, INK); g.drawPixel(lPadR, L_GROUND - 7, INK); }
}

void landerShip(Adafruit_GFX &g, int x, int y, bool thrust) {
  if (thrust) {
    int fl = 3 + random(0, 4);
    g.fillTriangle(x - 2, y - 3, x + 2, y - 3, x, y - 3 + fl + 2, INK);
    g.drawPixel(x, y - 3 + fl + 3, (frameCount & 1) ? INK : PAPER);
  }
  sprOut(g, x - 5, y - 9, LANDER, LANDER_W, LANDER_H);
}

void landerDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  landerBack(g);
  landerGround(g);
  partsDraw(g);
  int x = (int)lX, y = (int)lY;
  if (lCrashT == 0) landerShip(g, x, y, lThrust);

  // gauges: fuel and descent speed (the mark is the safe limit)
  txtOut(g, "F", 0, 9, 1);
  g.fillRect(7, 9, 34, 7, PAPER); g.drawRect(7, 9, 34, 7, INK);
  int fw = (int)(lFuel / lFuelMax * 30);
  if (fw > 0) { g.fillRect(9, 11, fw, 3, INK); }
  txtOut(g, "V", 85, 9, 1);
  g.fillRect(92, 9, 35, 7, PAPER); g.drawRect(92, 9, 35, 7, INK);
  float v = lVy < 0 ? 0 : (lVy > 2.5f ? 2.5f : lVy);
  int vw = (int)(v / 2.5f * 31);
  int tick = 94 + (int)(L_SAFE_VY / 2.5f * 31);
  for (int i = 0; i < vw; i++) g.drawFastVLine(94 + i, 11, 3, (94 + i < tick || (frameCount & 2)) ? INK : PAPER);
  g.drawFastVLine(tick, 8, 9, INK);

  char b[16]; snprintf(b, sizeof b, "LVL %d", lLevel + 1);
  txtOut(g, b, 1, 1, 1);
  hudScore(g, score, "");
  if (lLandT > 0) {
    txtOutC(g, lPerfect ? "PERFECT!" : "LANDED!", SCR_W / 2, 28, 1);
    if (lPerfect) txtOutC(g, "+50", SCR_W / 2, 38, 1);
  } else if (lLevel == 0 && frameCount < 80) {
    txtOutC(g, "hold = rockets", SCR_W / 2, 36, 1);
  }
}

void landerCover(Adafruit_GFX &g, uint32_t t) {
  int sl = lPadL, sr = lPadR, sw = lPadW;
  lPadL = 76; lPadR = 95; lPadW = 20;
  if (!lCoverReady) { landerTerrain(); lCoverReady = true; }
  g.fillScreen(PAPER);
  landerBack(g);
  landerGround(g);
  int y = 40 + (int)(sinf(t * 0.08f) * 3);
  if ((t / 3) % 3 != 0) {
    int fl = 6 + random(0, 5);
    g.fillTriangle(55, y - 5, 65, y - 5, 60, y - 5 + fl + 4, INK);
    g.fillTriangle(58, y - 5, 62, y - 5, 60, y - 2 + fl / 2, PAPER);
  }
  sprScale(g, 49, y - 20, LANDER, LANDER_W, LANDER_H, 2.0f, true);
  for (int k = 0; k < 3; k++) g.drawPixel(60 + random(-3, 4), y + 6 + random(0, 6), INK);
  lPadL = sl; lPadR = sr; lPadW = sw;
}

// =====================================================================
//  GAME 5: DINO  (tap = jump; day and night take turns)
// =====================================================================
const int   D_GROUND = 56, D_X = 14;
const float D_GRAV = 0.5f, D_JUMP = 4.6f, D_START = 2.5f, D_MAXV = 5.0f;
const int   D_CW[3] = { 5, 7, 11 };      // width of each cactus type
const int   D_CH[3] = { 10, 13, 10 };    // height

struct DObs { float x; int8_t type; };
DObs  dObs[2];
float dHeight, dVel, dSpeed, dScroll;
bool  dOnGround;
uint32_t dStartMs;

void dinoSpawn(int i, float x) { dObs[i].x = x; dObs[i].type = random(3); }

void dinoInit() {
  dHeight = 0; dVel = 0; dOnGround = true; dSpeed = D_START; dScroll = 0;
  dStartMs = millis();
  dinoSpawn(0, SCR_W + 30);
  dinoSpawn(1, dObs[0].x + random(70, 120));
}

bool dinoHit(int i) {
  int t = dObs[i].type;
  int px1 = D_X + 3, px2 = D_X + 9;
  int feet = D_GROUND - (int)dHeight;
  int ox1 = (int)dObs[i].x + 1, ox2 = (int)dObs[i].x + D_CW[t] - 2;
  int oTop = D_GROUND - D_CH[t];
  return (px2 >= ox1 && px1 <= ox2 && feet > oTop + 1);
}

bool dinoUpdate() {
  if (btn.pressed && dOnGround) {
    dVel = D_JUMP; dOnGround = false;
    addPart(D_X + 4, D_GROUND - 1, -1.0f, -0.4f, 8, 0, 0);
  }
  if (!dOnGround) {
    dHeight += dVel; dVel -= D_GRAV;
    if (dHeight <= 0) {
      dHeight = 0; dVel = 0; dOnGround = true;
      for (int k = 0; k < 3; k++) addPart(D_X + 5, D_GROUND - 1, frand(-1.5f, 0.5f), frand(-0.7f, -0.2f), 9, 0, 0.08f);
    }
  }
  score = (millis() - dStartMs) / 100;
  invMode = ((score / 400) % 2) == 0;            // day: inverted screen
  dSpeed = fminf(D_MAXV, D_START + score / 200.0f);
  dScroll += dSpeed;
  for (int i = 0; i < 2; i++) {
    dObs[i].x -= dSpeed;
    if (dObs[i].x + D_CW[dObs[i].type] < 0) {
      int other = 1 - i;
      float base = fmaxf(dObs[other].x, (float)SCR_W);
      dinoSpawn(i, base + random(60, 110) + dSpeed * 8);
    }
  }
  if (dOnGround && frameCount % 6 == 0) addPart(D_X + 2, D_GROUND - 1, -dSpeed * 0.5f, -0.3f, 6, 0, 0);
  for (int i = 0; i < 2; i++) if (dinoHit(i)) { crashFx(D_X + 6, D_GROUND - 6 - dHeight); return false; }
  return true;
}

void cactus(Adafruit_GFX &g, int x, int type) {
  int base = D_GROUND;
  if (type == 2) { cactus(g, x, 0); cactus(g, x + 6, 0); return; }
  int h = D_CH[type], w = D_CW[type];
  int tx = x + w / 2 - 1;
  g.fillRect(tx - 1, base - h - 1, 5, h + 2, PAPER);
  g.fillRect(tx, base - h + 1, 3, h - 1, INK);
  g.drawFastHLine(tx + 1, base - h, 1, INK);
  g.drawPixel(tx, base - h + 1, PAPER); g.drawPixel(tx + 2, base - h + 1, PAPER);
  if (type == 1) {
    g.fillRect(x, base - h + 4, 2, 5, INK);   g.drawFastHLine(x, base - h + 8, 3, INK);
    g.fillRect(x + w - 2, base - h + 2, 2, 5, INK); g.drawFastHLine(x + w - 3, base - h + 6, 3, INK);
  } else {
    g.fillRect(x, base - h + 3, 1, 3, INK); g.drawPixel(x + 1, base - h + 5, INK);
    g.fillRect(x + w - 1, base - h + 2, 1, 3, INK); g.drawPixel(x + w - 2, base - h + 4, INK);
  }
  g.drawPixel(tx + 1, base - h + 4, PAPER); g.drawPixel(tx + 1, base - h + 8, PAPER);
}

void dinoBack(Adafruit_GFX &g, float scroll, bool night) {
  if (night) {
    drawStars(g, 40);
    g.fillCircle(100, 12, 5, INK); g.fillCircle(103, 10, 4, PAPER);
  } else {
    g.drawCircle(100, 12, 5, INK);
    for (int k = 0; k < 8; k++) {
      float a = k * 0.785f + frameCount * 0.02f;
      g.drawPixel(100 + (int)(cosf(a) * 9), 12 + (int)(sinf(a) * 9), INK);
    }
  }
  int o = (int)(scroll * 0.25f);
  for (int i = 0; i < 3; i++) {
    int x = ((i * 61 - o) % 170 + 170) % 170 - 24;
    cloud(g, x, 14 + (i * 9) % 14);
  }
  // distant hills
  int ho = (int)(scroll * 0.12f);
  for (int x = 0; x < SCR_W; x++) {
    float wx = x + ho;
    int h = 8 + (int)(sinf(wx * 0.045f) * 4 + sinf(wx * 0.11f) * 2);
    for (int y = D_GROUND - h; y < D_GROUND; y++) if (pat(x + ho, y, 1)) g.drawPixel(x, y, INK);
    g.drawPixel(x, D_GROUND - h, INK);
  }
}

void dinoGround(Adafruit_GFX &g, float scroll) {
  g.fillRect(0, D_GROUND, SCR_W, SCR_H - D_GROUND, PAPER);
  int s = (int)scroll;
  for (int x = 0; x < SCR_W; x++) {
    int wx = x + s;
    int bump = ((wx * 7) % 53 == 0) ? 1 : 0;
    g.drawPixel(x, D_GROUND - bump, INK);
    if ((wx * 13) % 17 == 0) g.drawPixel(x, D_GROUND + 3, INK);
    if ((wx * 29) % 31 == 0) g.drawFastHLine(x, D_GROUND + 5, 2, INK);
  }
}

void dinoSprite(Adafruit_GFX &g, int x, int y, bool ground) {
  const uint8_t *b = !ground ? DINOJ : ((frameCount / 4) % 2 ? DINO1 : DINO2);
  sprOut(g, x, y, b, DINO1_W, DINO1_H);
}

void dinoDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  dinoBack(g, dScroll, !invMode);
  dinoGround(g, dScroll);
  for (int i = 0; i < 2; i++) cactus(g, (int)dObs[i].x, dObs[i].type);
  dinoSprite(g, D_X, D_GROUND - DINO1_H - (int)dHeight, dOnGround);
  partsDraw(g);
  char b[12]; snprintf(b, sizeof b, "%05lu", (unsigned long)score);
  txtOut(g, b, SCR_W - 1 - textW(b, 1), 1, 1);
}

void dinoCover(Adafruit_GFX &g, uint32_t t) {
  g.fillScreen(PAPER);
  dinoBack(g, t * 2.0f, true);
  dinoGround(g, t * 2.0f);
  cactus(g, 90, 1);
  cactus(g, 110, 2);
  float ph = (t % 30) / 30.0f;
  int jy = (int)(sinf(ph * 3.1416f) * 10);
  const uint8_t *b = jy > 1 ? DINOJ : ((t / 4) % 2 ? DINO1 : DINO2);
  sprScale(g, 30, D_GROUND - 24 - jy, b, DINO1_W, DINO1_H, 2.0f, true);
}

// =====================================================================
//  GAME 6: STACK  (tap = drop the block)
// =====================================================================
const int S_H = 6, S_STEP = 6, S_MOVING_Y = 18, S_MAX = 8;

int   sX[S_MAX], sW[S_MAX], sCount, mW, mDir, animOff, sCombo, sPerfT;
float mX, mSpeed;

void stackInit() {
  sCount = 1; sX[0] = 44; sW[0] = 40;
  mW = 40; mX = 0; mDir = 1; mSpeed = 1.8f; animOff = 0; sCombo = 0; sPerfT = 0;
}

bool stackUpdate() {
  if (animOff < 0) animOff += 1;
  if (sPerfT > 0) sPerfT--;
  // the block lands exactly where you see it, then moving resumes
  if (!btn.pressed) {
    mX += mDir * mSpeed;
    if (mX <= 0) { mX = 0; mDir = 1; }
    if (mX + mW >= SCR_W) { mX = SCR_W - mW; mDir = -1; }
  } else {
    int x = (int)mX, px = sX[0], pw = sW[0];
    int left = max(x, px), right = min(x + mW, px + pw);
    int overlap = right - left;
    if (overlap <= 0) {
      addChip(x, S_MOVING_Y, mDir * 0.5f, mW, S_H);
      shakeT = 8; flashT = 2;
      return false;
    }
    int nx, nw;
    if (abs(x - px) <= 2) {
      nx = px; nw = pw; sCombo++; sPerfT = 25;
      for (int k = 0; k < 5; k++) addPart(nx + frand(0, nw), S_MOVING_Y + S_STEP, frand(-1, 1), frand(-1.5f, -0.5f), 18, 2, 0.05f);
      if (sCombo >= 3 && nw < 40) {                   // streak of perfect drops: the block grows wider
        int gr = min(4, 40 - nw);
        nx -= gr / 2; nw += gr;
        if (nx < 0) nx = 0;
        if (nx + nw > SCR_W) nx = SCR_W - nw;
      }
    } else {
      sCombo = 0;
      nx = left; nw = overlap;
      if (x < px) addChip(x, S_MOVING_Y, -0.6f, px - x, S_H);                         // piece cut off on the left
      else addChip(px + pw, S_MOVING_Y, 0.6f, x + mW - (px + pw), S_H);                // piece cut off on the right
    }
    for (int i = S_MAX - 1; i > 0; i--) { sX[i] = sX[i - 1]; sW[i] = sW[i - 1]; }
    sX[0] = nx; sW[0] = nw;
    if (sCount < S_MAX) sCount++;
    score++;
    mW = nw;
    mDir = (score % 2) ? -1 : 1;
    mX = (mDir > 0) ? 0 : SCR_W - mW;
    mSpeed = fminf(4.5f, 1.8f + score * 0.08f);
    animOff = -S_STEP;
  }
  return true;
}

void block(Adafruit_GFX &g, int x, int y, int w) {
  g.fillRect(x, y, w, S_H, PAPER);
  g.fillRect(x, y, w, 2, INK);                       // top face
  dith(g, x + 1, y + 2, w - 2, S_H - 3, 2, 0);      // shaded front
  g.drawRect(x, y, w, S_H, INK);
  g.drawFastVLine(x + w - 2, y + 2, S_H - 3, INK);
}

void stackBack(Adafruit_GFX &g, int sc) {
  drawStars(g, 60);
  int base = SCR_H + min(sc, 12) * 4 - 2;            // the city sinks as the tower rises
  if (base < SCR_H + 30) drawCity(g, 0, base, 2, 6);
  int o = frameCount / 3;
  cloud(g, ((40 - o) % 170 + 170) % 170 - 24, 26);
  cloud(g, ((130 - o / 2) % 170 + 170) % 170 - 24, 40);
}

void stackTower(Adafruit_GFX &g, int movX, int movW, int cnt, const int *xs, const int *ws, int off) {
  int cx = movX + movW / 2;
  g.drawFastVLine(cx, 0, S_MOVING_Y + off - 3, INK);   // crane rope
  g.drawRect(cx - 2, S_MOVING_Y + off - 4, 5, 3, INK);
  block(g, movX, S_MOVING_Y + off, movW);
  for (int k = 0; k < cnt; k++) block(g, xs[k], S_MOVING_Y + S_STEP * (k + 1) + off, ws[k]);
}

void stackDraw(Adafruit_GFX &g) {
  g.fillScreen(PAPER);
  stackBack(g, score);
  stackTower(g, (int)mX, mW, sCount, sX, sW, animOff);
  partsDraw(g);
  char b[12]; snprintf(b, sizeof b, "%lu", (unsigned long)score);
  txtOut(g, b, SCR_W - 2 - textW(b, 2), 2, 2);
  if (sPerfT > 0) {
    txtOutC(g, "PERFECT!", 40, 4, 1);
    if (sCombo >= 2) { snprintf(b, sizeof b, "x%d", sCombo); txtOutC(g, b, 40, 12, 1); }
  }
}

void stackCover(Adafruit_GFX &g, uint32_t t) {
  g.fillScreen(PAPER);
  stackBack(g, 0);
  int xs[6] = { 44, 46, 43, 45, 44, 42 }, ws[6] = { 40, 38, 40, 38, 40, 42 };
  int mx = 44 + (int)(sinf(t * 0.07f) * 36);
  stackTower(g, mx, 38, 6, xs, ws, 2);
}

// =====================================================================
//  GAME DISPATCH
// =====================================================================
void gameInit(int g) {
  switch (g) {
    case G_FLAPPY: flappyInit(); break;
    case G_TURBO:  turboInit();  break;
    case G_ROOF:   roofInit();   break;
    case G_LANDER: landerInit(); break;
    case G_DINO:   dinoInit();   break;
    case G_STACK:  stackInit();  break;
  }
}
bool gameUpdate(int g) {
  switch (g) {
    case G_FLAPPY: return flappyUpdate();
    case G_TURBO:  return turboUpdate();
    case G_ROOF:   return roofUpdate();
    case G_LANDER: return landerUpdate();
    case G_DINO:   return dinoUpdate();
    case G_STACK:  return stackUpdate();
  }
  return false;
}
void gameDraw(Adafruit_GFX &g, int id) {
  switch (id) {
    case G_FLAPPY: flappyDraw(g); break;
    case G_TURBO:  turboDraw(g);  break;
    case G_ROOF:   roofDraw(g);   break;
    case G_LANDER: landerDraw(g); break;
    case G_DINO:   dinoDraw(g);   break;
    case G_STACK:  stackDraw(g);  break;
  }
}
void coverArt(Adafruit_GFX &g, int id, uint32_t t) {
  switch (id) {
    case G_FLAPPY: flappyCover(g, t); break;
    case G_TURBO:  turboCover(g, t);  break;
    case G_ROOF:   roofCover(g, t);   break;
    case G_LANDER: landerCover(g, t); break;
    case G_DINO:   dinoCover(g, t);   break;
    case G_STACK:  stackCover(g, t);  break;
  }
}

// =====================================================================
//  MENU COVERS
// =====================================================================
void coverFrame(Adafruit_GFX &g, int id, uint32_t t, float hold) {
  coverArt(g, id, t);
  title(g, TITLES[id], SCR_W / 2, 3);

  // bottom: high score (or hint) on the left, page dots on the right, on small badges
  if ((t / 45) % 2 == 0) {
    char b[12]; snprintf(b, sizeof b, "%lu", (unsigned long)hi[id]);
    int w = 12 + textW(b, 1);
    g.fillRoundRect(0, 54, w + 3, 10, 2, PAPER);
    g.drawRoundRect(0, 54, w + 3, 10, 2, INK);
    g.drawBitmap(2, 56, TROPHY, TROPHY_W, TROPHY_H, INK);
    txt(g, b, 11, 56, 1, INK);
  } else {
    int w = textW(HINTS[id], 1);
    g.fillRoundRect(0, 54, w + 5, 10, 2, PAPER);
    g.drawRoundRect(0, 54, w + 5, 10, 2, INK);
    txt(g, HINTS[id], 2, 56, 1, INK);
  }
  g.fillRoundRect(SCR_W - 6 * G_COUNT - 4, 54, 6 * G_COUNT + 4, 10, 2, PAPER);
  g.drawRoundRect(SCR_W - 6 * G_COUNT - 4, 54, 6 * G_COUNT + 4, 10, 2, INK);
  for (int i = 0; i < G_COUNT; i++) {
    int x = SCR_W - 6 * (G_COUNT - i) - 1;
    if (i == id) g.fillRect(x, 57, 4, 4, INK);
    else g.drawPixel(x + 1, 59, INK);
  }

  // button held: the bar fills up until the game starts
  if (hold > 0) {
    int w = (int)(hold * 100);
    g.fillRoundRect(12, 39, 104, 12, 3, PAPER);
    g.drawRoundRect(12, 39, 104, 12, 3, INK);
    g.fillRect(14, 41, w, 8, INK);
    // label that inverts where the bar is filled
    int tx = SCR_W / 2 - textW("PLAY!", 1) / 2;
    fb2.fillScreen(PAPER);
    txt(fb2, "PLAY!", tx, 41, 1, INK);
    uint8_t *tb = fb2.getBuffer();
    for (int yy = 41; yy < 49; yy++)
      for (int xx = tx; xx < tx + 36; xx++)
        if (tb[yy * (SCR_W / 8) + xx / 8] & (0x80 >> (xx & 7))) g.drawPixel(xx, yy, xx < 14 + w ? PAPER : INK);
  } else if ((t / 12) % 4 != 0) {
    // arrow inviting you to press
    int ax = SCR_W - 8 + ((t / 4) % 3);
    g.fillTriangle(ax, 44, ax, 50, ax + 3, 47, INK);
  }
}

// =====================================================================
//  START SCREEN
// =====================================================================
void splashDraw(Adafruit_GFX &g, uint32_t t) {
  g.fillScreen(PAPER);
  drawStars(g, 64);
  // perspective grid at the bottom
  for (int i = 0; i < 6; i++) {
    int y = 46 + (i * i) / 2 + i * 2 + (int)((t % 8) * (i + 1) / 8);
    if (y < SCR_H) g.drawFastHLine(0, y, SCR_W, INK);
  }
  for (int k = -6; k <= 6; k++) g.drawLine(64 + k * 6, 46, 64 + k * 26, 63, INK);
  g.fillRect(0, 38, SCR_W, 8, PAPER);
  g.drawFastHLine(0, 46, SCR_W, INK);

  int drop = t < 15 ? (15 - t) * 3 : 0;
  txtOutC(g, "- MINI -", SCR_W / 2, 3 - drop, 1);
  // embossed ARCADE logo: shadow, black gap, white letters
  const char *a = "ARCADE";
  int x = SCR_W / 2 - textW(a, 3) / 2 - 1;
  int y = 13 + (t < 20 ? 0 : (int)(sinf(t * 0.1f) * 1.5f));
  txt(g, a, x + 2, y + 2, 3, INK);
  txt(g, a, x + 1, y + 1, 3, PAPER);
  txt(g, a, x + 1, y, 3, PAPER);
  txt(g, a, x, y + 1, 3, PAPER);
  txt(g, a, x, y, 3, INK);
  // character parade along the horizon line
  int px = (int)((t * 2) % 230) - 70;
  sprOut(g, px, 36, BIRD_MID, BIRD_MID_W, BIRD_MID_H);
  sprOut(g, px + 20, 34, (t / 4) % 2 ? DINO1 : DINO2, DINO1_W, DINO1_H);
  sprOut(g, px + 38, 34, (t / 3) % 2 ? RUN1 : RUN2, RUN1_W, RUN1_H);
  sprOut(g, px + 52, 36, LANDER, LANDER_W, LANDER_H);

  if (t > 25 && (t / 10) % 3) {
    g.fillRect(17, 51, 94, 12, PAPER);
    g.drawRoundRect(17, 51, 94, 12, 3, INK);
    txtC(g, "press button", SCR_W / 2, 53, 1, INK);
  }
}

// =====================================================================
//  GAME OVER
// =====================================================================
void overDraw(Adafruit_GFX &g, uint32_t t) {
  gameDraw(g, cur);
  dimFb();
  g.fillRoundRect(6, 6, 116, 52, 5, PAPER);
  g.drawRoundRect(6, 6, 116, 52, 5, INK);
  g.drawRoundRect(8, 8, 112, 48, 4, INK);
  title(g, "GAME OVER", SCR_W / 2, 12);
  char b[24];
  snprintf(b, sizeof b, "score  %lu", (unsigned long)score);
  txtC(g, b, SCR_W / 2, 30, 1, INK);
  if (newRecord) {
    if ((t / 6) % 2) {
      txtC(g, "NEW RECORD!", SCR_W / 2, 39, 1, INK);
      g.drawBitmap(14, 39, STAR, STAR_W, STAR_H, INK);
      g.drawBitmap(SCR_W - 19, 39, STAR, STAR_W, STAR_H, INK);
    }
    if (t % 5 == 0) addPart(frand(14, 114), 30, frand(-0.5f, 0.5f), frand(-1.2f, -0.3f), 20, 4, 0.04f);
  } else {
    snprintf(b, sizeof b, "best   %lu", (unsigned long)hi[cur]);
    txtC(g, b, SCR_W / 2, 39, 1, INK);
  }
  for (int x = 14; x < 114; x += 2) g.drawPixel(x, 48, INK);
  txtC(g, (t / 45) % 2 ? "hold: menu" : "tap: again", SCR_W / 2, 49, 1, INK);
  partsDraw(g);
}

// =====================================================================
//  MAIN LOOP
// =====================================================================
void startGame(int g) {
  cur = g; score = 0; frameCount = 0; newRecord = false;
  invMode = false; shakeT = 0; flashT = 0;
  partsClear();
  gameStartMs = millis();
  gameInit(g);
  state = PLAYING; stateFrames = 0;
}

void saveRecord() {
  if (score > hi[cur]) {
    hi[cur] = score; newRecord = true;
    char key[16]; snprintf(key, sizeof key, "hi%d", cur);
    prefs.putUInt(key, hi[cur]);
  }
}

void goMenu() { sel = cur; state = MENU; stateFrames = 0; invMode = false; partsClear(); }

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BTN_BOOT, INPUT_PULLUP);
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED display not found! Check the wiring and the address.");
    for (;;) delay(1000);
  }
  randomSeed(esp_random());
  genWorld();
  prefs.begin("arcade", false);
  for (int i = 0; i < G_COUNT; i++) {
    char key[16]; snprintf(key, sizeof key, "hi%d", i);
    hi[i] = prefs.getUInt(key, 0);
  }
  state = SPLASH; stateFrames = 0;
}

void loop() {
  uint32_t now = millis();
  if (now - lastFrame < FRAME_MS) return;
  lastFrame = now;
  readButton(now);
  stateFrames++;

  switch (state) {
    case SPLASH:
      frameCount++;
      splashDraw(fb, stateFrames);
      present();
      if (stateFrames > 10 && (btn.shortTap || btn.longHold)) { state = MENU; stateFrames = 0; btn.longFired = true; }
      break;

    case MENU: {
      frameCount++;
      float hold = 0;
      if (btn.down && !btn.longFired && stateFrames > 3) hold = (now - btn.pressStart) / (float)LONG_MS;
      if (hold > 1) hold = 1;
      coverFrame(fb, sel, stateFrames, hold);
      present();
      if (btn.shortTap && stateFrames > 3) { slideTo = (sel + 1) % G_COUNT; slideBaseT = stateFrames; state = SLIDE; stateFrames = 0; }
      else if (btn.longHold && stateFrames > 3) { state = LAUNCH; stateFrames = 0; }
      break;
    }

    case SLIDE: {
      frameCount++;
      const int N = 9;
      float tt = stateFrames / (float)N;
      if (tt > 1) tt = 1;
      float e = 1 - (1 - tt) * (1 - tt) * (1 - tt);
      coverFrame(fb, sel, slideBaseT + stateFrames, 0);
      coverFrame(fb2, slideTo, stateFrames, 0);
      presentSlide(-(int)(e * SCR_W));
      if (stateFrames >= N) { sel = slideTo; state = MENU; }
      break;
    }

    case LAUNCH: {
      frameCount++;
      coverFrame(fb, sel, stateFrames, 1);
      int r = stateFrames * 9;
      for (int k = 0; k < 3; k++) fb.drawCircle(SCR_W / 2, SCR_H / 2, r - k, PAPER);
      fb.fillCircle(SCR_W / 2, SCR_H / 2, max(0, r - 3), PAPER);
      present();
      if (stateFrames >= 9) startGame(sel);
      break;
    }

    case PLAYING: {
      uint32_t exitMs = (cur == G_LANDER) ? 4000 : EXIT_MS;
      if (btn.down && btn.pressStart > gameStartMs && now - btn.pressStart >= exitMs) { goMenu(); btn.longFired = true; break; }
      frameCount++;
      bool alive = gameUpdate(cur);
      partsUpdate();
      gameDraw(fb, cur);
      present();
      if (!alive) { saveRecord(); state = DYING; stateFrames = 0; }
      break;
    }

    case DYING:
      frameCount++;
      partsUpdate();
      gameDraw(fb, cur);
      present();
      if (stateFrames > 28) { state = OVER; stateFrames = 0; overMs = now; partsClear(); }
      break;

    case OVER:
      frameCount++;
      partsUpdate();
      overDraw(fb, stateFrames);
      present();
      if (btn.pressStart > overMs + 300) {
        if (btn.shortTap) startGame(cur);
        else if (btn.longHold) goMenu();
      }
      break;
  }
}
