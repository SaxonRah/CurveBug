#ifndef CONFIG_H
#define CONFIG_H

#include <raylib.h>
#include <stdbool.h>

typedef struct {
    char serial_port[256];
    int window_width;
    int window_height;
    
    Color bg_color;
    Color dut1_trace;
    Color dut2_trace;
    Color dut1_dimmed;
    Color dut2_dimmed;
    Color grid_bg;
    Color grid_color;
    Color crosshair;
    Color label_color;
    Color axis_color;
    Color border_color;
    
    char keybinds[8][32];  // pause, single, auto, fit, reset, mode, settings, quit
} Config;

void ConfigLoad(Config* config, const char* filename);
void ConfigSave(const Config* config, const char* filename);
void ConfigSetDefaults(Config* config);
void ConfigSetDarkMode(Config* config);
void ConfigSetLightMode(Config* config);

#endif