#include "config.h"
#include <stdio.h>
#include <string.h>

void ConfigSetDarkMode(Config* config) {
    config->bg_color = (Color){0, 0, 0, 255};
    config->dut1_trace = (Color){50, 150, 255, 255};
    config->dut2_trace = (Color){255, 50, 50, 255};
    config->dut1_dimmed = (Color){25, 75, 128, 255};
    config->dut2_dimmed = (Color){128, 25, 25, 255};
    config->grid_bg = (Color){30, 30, 30, 255};
    config->grid_color = (Color){50, 50, 50, 255};
    config->crosshair = (Color){255, 255, 50, 255};
    config->label_color = (Color){200, 200, 200, 255};
    config->axis_color = (Color){255, 255, 255, 255};
    config->border_color = (Color){100, 100, 100, 255};
}

void ConfigSetLightMode(Config* config) {
    config->bg_color = (Color){255, 255, 255, 255};
    config->dut1_trace = (Color){0, 100, 200, 255};
    config->dut2_trace = (Color){200, 0, 0, 255};
    config->dut1_dimmed = (Color){150, 180, 220, 255};
    config->dut2_dimmed = (Color){220, 150, 150, 255};
    config->grid_bg = (Color){240, 240, 240, 255};
    config->grid_color = (Color){200, 200, 200, 255};
    config->crosshair = (Color){180, 180, 0, 255};
    config->label_color = (Color){60, 60, 60, 255};
    config->axis_color = (Color){0, 0, 0, 255};
    config->border_color = (Color){100, 100, 100, 255};
}

void ConfigSetDefaults(Config* config) {
    strcpy(config->serial_port, "COM4");
    config->window_width = 1080;
    config->window_height = 1080;
    
    ConfigSetDarkMode(config);
    
    strcpy(config->keybinds[0], "P");
    strcpy(config->keybinds[1], "S");
    strcpy(config->keybinds[2], "A");
    strcpy(config->keybinds[3], "F");
    strcpy(config->keybinds[4], "R");
    strcpy(config->keybinds[5], "SPACE");
    strcpy(config->keybinds[6], "F1");
    strcpy(config->keybinds[7], "ESC");
}

void ConfigLoad(Config* config, const char* filename) {
    ConfigSetDefaults(config);
    
    FILE* f = fopen(filename, "r");
    if (!f) return;
    
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char key[128], value[384];
        if (sscanf(line, "%127[^=]=%383[^\n]", key, value) == 2) {
            if (strcmp(key, "serial_port") == 0) {
                strncpy(config->serial_port, value, sizeof(config->serial_port) - 1);
            } else if (strcmp(key, "window_width") == 0) {
                config->window_width = atoi(value);
            } else if (strcmp(key, "window_height") == 0) {
                config->window_height = atoi(value);
            } else if (strcmp(key, "bg_color") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->bg_color.r, &config->bg_color.g, &config->bg_color.b);
            } else if (strcmp(key, "dut1_trace") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->dut1_trace.r, &config->dut1_trace.g, &config->dut1_trace.b);
            } else if (strcmp(key, "dut2_trace") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->dut2_trace.r, &config->dut2_trace.g, &config->dut2_trace.b);
            } else if (strcmp(key, "dut1_dimmed") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->dut1_dimmed.r, &config->dut1_dimmed.g, &config->dut1_dimmed.b);
            } else if (strcmp(key, "dut2_dimmed") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->dut2_dimmed.r, &config->dut2_dimmed.g, &config->dut2_dimmed.b);
            } else if (strcmp(key, "grid_bg") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->grid_bg.r, &config->grid_bg.g, &config->grid_bg.b);
            } else if (strcmp(key, "grid_color") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->grid_color.r, &config->grid_color.g, &config->grid_color.b);
            } else if (strcmp(key, "crosshair") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->crosshair.r, &config->crosshair.g, &config->crosshair.b);
            } else if (strcmp(key, "label_color") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->label_color.r, &config->label_color.g, &config->label_color.b);
            } else if (strcmp(key, "axis_color") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->axis_color.r, &config->axis_color.g, &config->axis_color.b);
            } else if (strcmp(key, "border_color") == 0) {
                sscanf(value, "%hhu,%hhu,%hhu", &config->border_color.r, &config->border_color.g, &config->border_color.b);
            } else if (strncmp(key, "keybind_", 8) == 0) {
                int idx = atoi(key + 8);
                if (idx >= 0 && idx < 8) {
                    strncpy(config->keybinds[idx], value, 31);
                }
            }
        }
    }
    
    fclose(f);
}

void ConfigSave(const Config* config, const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) return;
    
    fprintf(f, "serial_port=%s\n", config->serial_port);
    fprintf(f, "window_width=%d\n", config->window_width);
    fprintf(f, "window_height=%d\n", config->window_height);
    
    fprintf(f, "bg_color=%d,%d,%d\n", config->bg_color.r, config->bg_color.g, config->bg_color.b);
    fprintf(f, "dut1_trace=%d,%d,%d\n", config->dut1_trace.r, config->dut1_trace.g, config->dut1_trace.b);
    fprintf(f, "dut2_trace=%d,%d,%d\n", config->dut2_trace.r, config->dut2_trace.g, config->dut2_trace.b);
    fprintf(f, "dut1_dimmed=%d,%d,%d\n", config->dut1_dimmed.r, config->dut1_dimmed.g, config->dut1_dimmed.b);
    fprintf(f, "dut2_dimmed=%d,%d,%d\n", config->dut2_dimmed.r, config->dut2_dimmed.g, config->dut2_dimmed.b);
    fprintf(f, "grid_bg=%d,%d,%d\n", config->grid_bg.r, config->grid_bg.g, config->grid_bg.b);
    fprintf(f, "grid_color=%d,%d,%d\n", config->grid_color.r, config->grid_color.g, config->grid_color.b);
    fprintf(f, "crosshair=%d,%d,%d\n", config->crosshair.r, config->crosshair.g, config->crosshair.b);
    fprintf(f, "label_color=%d,%d,%d\n", config->label_color.r, config->label_color.g, config->label_color.b);
    fprintf(f, "axis_color=%d,%d,%d\n", config->axis_color.r, config->axis_color.g, config->axis_color.b);
    fprintf(f, "border_color=%d,%d,%d\n", config->border_color.r, config->border_color.g, config->border_color.b);
    
    for (int i = 0; i < 8; i++) {
        fprintf(f, "keybind_%d=%s\n", i, config->keybinds[i]);
    }
    
    fclose(f);
}