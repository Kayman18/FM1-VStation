#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "VStation"
#define APP_TAGLINE "RC"
#define APP_SPLASH_RGB RGB(80,160,240)
#define APP_PALETTE 3u
#define APP_BLOCK 256u
#define APP_CANVAS_PX 4096u /* strip-render UI; keeps audio-pool RAM for native reverb */
#define APP_TOUCH
#define APP_SPLASH
#define APP_SYSEX
enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BLE, OBJ_NOBJ };
#define APP_STORE_MAP {0xFC000u,0xDC000u,0xDE000u}
void app_splash(void);void app_init(void);void app_render(int32_t*,uint32_t,uint32_t);
void app_panic(void);void app_ui_init(void);void app_frame(void);
void app_input_only(void);void app_say(const char*,const char*);
int app_sysex(const uint8_t*,uint32_t);

/* UI-thread request: returns0 for malformed, busy or full queue.
   Accepted requests are applied by audio before render, then acknowledged to UI. */
int app_request_preset(const uint8_t*,unsigned);
unsigned app_preset_status(void);
