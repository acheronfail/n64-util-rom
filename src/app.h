#ifndef APP_H
#define APP_H
#include <stdbool.h>
#include <stdint.h>
#include "pak.h"
#include "pak_write.h"
enum { BTN_A=1u<<0, BTN_B=1u<<1, BTN_START=1u<<2, BTN_Z=1u<<3,
    BTN_L=1u<<4, BTN_R=1u<<5, BTN_DU=1u<<6, BTN_DD=1u<<7,
    BTN_DL=1u<<8, BTN_DR=1u<<9, BTN_CU=1u<<10, BTN_CD=1u<<11,
    BTN_CL=1u<<12, BTN_CR=1u<<13 };
#define TRAIL_SIZE 128
#define PORT_COUNT 4
#define HOLD_MS 1200
typedef enum { TOOL_CONTROLLER, TOOL_RUMBLE, TOOL_AUDIO, TOOL_PAK, TOOL_COUNT } Tool;
typedef struct { int8_t x, y; } Point;
typedef struct {
    bool connected;
    uint16_t buttons;
    int8_t x, y;
} Input;
typedef struct {
    bool connected, start_armed;
    bool navigation_ready;
    uint16_t navigation_direction;
    unsigned navigation_repeat_ms;
    uint16_t buttons;
    int x, y, min_x, max_x, min_y, max_y;
    unsigned count, next;
    Point trail[TRAIL_SIZE];
} Controller;
typedef struct {
    bool testing, controls_armed, audio_playing, pak_refresh;
    bool calibration_notice;
    PakInspector pak;
    PakWrite pak_write;
    Tool tool;
    unsigned menu_index, pulse_ms, tone_index, audio_channel;
    bool rumble_supported[PORT_COUNT];
    unsigned active_port;
    /* Zero means no gesture; otherwise the port index plus one. */
    unsigned gesture_port, hold_ms;
    Controller ports[PORT_COUNT];
} App;
void app_update(App *app, const Input inputs[PORT_COUNT], unsigned elapsed_ms);
bool app_rumble_active(const App *app, unsigned port);
unsigned app_tone_hz(const App *app);
#endif
