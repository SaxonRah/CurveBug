// Define these BEFORE any Windows headers to prevent conflicts
#if defined(_WIN32)
    #define NOGDI
    #define NOUSER
#endif

#define RAYGUI_IMPLEMENTATION
#include "raylib.h"

// Undefine Windows conflicts after raylib
#if defined(_WIN32)
    #undef near
    #undef far
#endif

#include "raygui.h"
#include "serial.h"
#include "config.h"
#include "plotter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Settings tab enum
typedef enum {
    TAB_GENERAL = 0,
    TAB_COLORS,
    TAB_KEYBINDS
} SettingsTab;

// Helper for drawing tabs
int DrawTabs(Rectangle bounds, const char** tabs, int count, int active) {
    float tab_width = bounds.width / count;
    int clicked = -1;
    
    for (int i = 0; i < count; i++) {
        Rectangle tab_rect = {
            bounds.x + i * tab_width,
            bounds.y,
            tab_width,
            bounds.height
        };
        
        Color bg_color = (i == active) ? LIGHTGRAY : GRAY;
        DrawRectangleRec(tab_rect, bg_color);
        DrawRectangleLinesEx(tab_rect, 1, DARKGRAY);
        
        int text_width = MeasureText(tabs[i], 10);
        DrawText(tabs[i], 
                (int)(tab_rect.x + tab_rect.width/2 - text_width/2),
                (int)(tab_rect.y + tab_rect.height/2 - 5),
                10, BLACK);
        
        if (CheckCollisionPointRec(GetMousePosition(), tab_rect) && 
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            clicked = i;
        }
    }
    
    return clicked;
}

// Draw color preview
void DrawColorPreview(Rectangle bounds, Config* config) {
    // Background
    DrawRectangleRec(bounds, config->grid_bg);
    
    // Grid
    for (int i = 0; i <= 5; i++) {
        float x = bounds.x + (i * bounds.width) / 5.0f;
        float y = bounds.y + (i * bounds.height) / 5.0f;
        DrawLine((int)x, (int)bounds.y, (int)x, (int)(bounds.y + bounds.height), config->grid_color);
        DrawLine((int)bounds.x, (int)y, (int)(bounds.x + bounds.width), (int)y, config->grid_color);
    }
    
    // Crosshair
    float mid_x = bounds.x + bounds.width / 2;
    float mid_y = bounds.y + bounds.height / 2;
    DrawLine((int)mid_x, (int)bounds.y, (int)mid_x, (int)(bounds.y + bounds.height), config->crosshair);
    DrawLine((int)bounds.x, (int)mid_y, (int)(bounds.x + bounds.width), (int)mid_y, config->crosshair);
    
    // Sample traces
    Vector2 trace1_points[] = {
        {bounds.x + bounds.width * 0.2f, bounds.y + bounds.height * 0.7f},
        {bounds.x + bounds.width * 0.4f, bounds.y + bounds.height * 0.5f},
        {bounds.x + bounds.width * 0.6f, bounds.y + bounds.height * 0.3f},
        {bounds.x + bounds.width * 0.8f, bounds.y + bounds.height * 0.2f}
    };
    
    Vector2 trace2_points[] = {
        {bounds.x + bounds.width * 0.2f, bounds.y + bounds.height * 0.8f},
        {bounds.x + bounds.width * 0.4f, bounds.y + bounds.height * 0.6f},
        {bounds.x + bounds.width * 0.6f, bounds.y + bounds.height * 0.5f},
        {bounds.x + bounds.width * 0.8f, bounds.y + bounds.height * 0.4f}
    };
    
    for (int i = 0; i < 3; i++) {
        DrawLineEx(trace1_points[i], trace1_points[i+1], 2.0f, config->dut1_trace);
        DrawLineEx(trace2_points[i], trace2_points[i+1], 2.0f, config->dut2_trace);
    }
    
    // Labels
    DrawText("0", (int)(bounds.x + 5), (int)(mid_y - 10), 10, config->label_color);
    DrawText("V", (int)(bounds.x + bounds.width - 15), (int)(bounds.y + bounds.height - 15), 10, config->label_color);
    
    // Title
    DrawText("Preview", (int)(bounds.x + bounds.width/2 - 25), (int)(bounds.y + 5), 10, config->axis_color);
    
    // Border
    DrawRectangleLinesEx(bounds, 2, config->border_color);
}

void PlotViewInit(PlotView* view, Rectangle area) {
    view->area = area;
    view->auto_scale = false;
    view->zoom = 1.0f;
    view->pan_x = 0.0f;
    view->pan_y = 0.0f;
    view->dragging = false;
}

float MinFloat(float* arr, int count) {
    if (count == 0) return 0;
    float min_val = arr[0];
    for (int i = 1; i < count; i++) {
        if (arr[i] < min_val) min_val = arr[i];
    }
    return min_val;
}

float MaxFloat(float* arr, int count) {
    if (count == 0) return 0;
    float max_val = arr[0];
    for (int i = 1; i < count; i++) {
        if (arr[i] > max_val) max_val = arr[i];
    }
    return max_val;
}

void DrawTrace(ChannelData* ch, Color color, Rectangle r, 
               float x_min, float x_max, float y_min, float y_max) {
    for (int i = 0; i < ch->count - 1; i++) {
        float x1_norm = (ch->voltage[i] - x_min) / (x_max - x_min);
        float y1_norm = (ch->current[i] - y_min) / (y_max - y_min);
        float x2_norm = (ch->voltage[i+1] - x_min) / (x_max - x_min);
        float y2_norm = (ch->current[i+1] - y_min) / (y_max - y_min);
        
        float x1 = r.x + r.width - (x1_norm * r.width);
        float y1 = r.y + (y1_norm * r.height);
        float x2 = r.x + r.width - (x2_norm * r.width);
        float y2 = r.y + (y2_norm * r.height);
        
        DrawLineEx((Vector2){x1, y1}, (Vector2){x2, y2}, 1.5f, color);
    }
}

void PlotViewDraw(PlotView* view, CurveData* data, Config* config, bool single_channel) {
    Rectangle r = view->area;
    
    DrawRectangleRec(r, config->grid_bg);
    
    if (data->ch1_active->count == 0) {
        DrawText("No Data", (int)(r.x + r.width/2 - 40), (int)(r.y + r.height/2), 20, WHITE);
        return;
    }
    
    float x_min, x_max, y_min, y_max;
    
    if (view->auto_scale) {
        float* all_x = malloc(sizeof(float) * MAX_SAMPLES * 2);
        float* all_y = malloc(sizeof(float) * MAX_SAMPLES * 2);
        int idx = 0;
        
        for (int i = 0; i < data->ch1_active->count; i++) {
            all_x[idx] = data->ch1_active->voltage[i];
            all_y[idx] = data->ch1_active->current[i];
            idx++;
        }
        if (!single_channel) {
            for (int i = 0; i < data->ch2_active->count; i++) {
                all_x[idx] = data->ch2_active->voltage[i];
                all_y[idx] = data->ch2_active->current[i];
                idx++;
            }
        }
        
        x_min = MinFloat(all_x, idx);
        x_max = MaxFloat(all_x, idx);
        y_min = MinFloat(all_y, idx);
        y_max = MaxFloat(all_y, idx);
        
        float x_margin = (x_max - x_min) * 0.1f;
        float y_margin = (y_max - y_min) * 0.1f;
        x_min -= x_margin;
        x_max += x_margin;
        y_min -= y_margin;
        y_max += y_margin;
        
        free(all_x);
        free(all_y);
    } else {
        float base_x_min = 0;
        float base_x_max = ADC_MAX;
        float y_range = ADC_MAX - 700;
        float base_y_max = y_range / 8.0f;
        float base_y_min = -y_range * 7.0f / 8.0f;
        
        float x_range_visible = (base_x_max - base_x_min) / view->zoom;
        float y_range_visible = (base_y_max - base_y_min) / view->zoom;
        
        float x_center = (base_x_max + base_x_min) / 2.0f + view->pan_x;
        float y_center = (base_y_max + base_y_min) / 2.0f + view->pan_y;
        
        x_min = x_center - x_range_visible / 2.0f;
        x_max = x_center + x_range_visible / 2.0f;
        y_min = y_center - y_range_visible / 2.0f;
        y_max = y_center + y_range_visible / 2.0f;
    }
    
    if (x_max == x_min) x_max = x_min + 1;
    if (y_max == y_min) y_max = y_min + 1;
    
    for (int i = 0; i <= 10; i++) {
        float x = r.x + (i * r.width) / 10.0f;
        float y = r.y + (i * r.height) / 10.0f;
        DrawLine((int)x, (int)r.y, (int)x, (int)(r.y + r.height), config->grid_color);
        DrawLine((int)r.x, (int)y, (int)(r.x + r.width), (int)y, config->grid_color);
    }
    
    float zero_x_norm = (ADC_ORIGIN - x_min) / (x_max - x_min);
    float zero_y_norm = (0 - y_min) / (y_max - y_min);
    
    if (zero_x_norm >= 0 && zero_x_norm <= 1) {
        float x = r.x + r.width - (zero_x_norm * r.width);
        DrawLine((int)x, (int)r.y, (int)x, (int)(r.y + r.height), config->crosshair);
    }
    if (zero_y_norm >= 0 && zero_y_norm <= 1) {
        float y = r.y + (zero_y_norm * r.height);
        DrawLine((int)r.x, (int)y, (int)(r.x + r.width), (int)y, config->crosshair);
    }
    
    if (data->excitation_mode == 2 && data->ch1_std.count > 0 && data->ch1_weak.count > 0) {
        if (data->last_was_weak) {
            DrawTrace(&data->ch1_std, config->dut1_dimmed, r, x_min, x_max, y_min, y_max);
            if (!single_channel) DrawTrace(&data->ch2_std, config->dut2_dimmed, r, x_min, x_max, y_min, y_max);
            DrawTrace(&data->ch1_weak, config->dut1_trace, r, x_min, x_max, y_min, y_max);
            if (!single_channel) DrawTrace(&data->ch2_weak, config->dut2_trace, r, x_min, x_max, y_min, y_max);
        } else {
            DrawTrace(&data->ch1_weak, config->dut1_dimmed, r, x_min, x_max, y_min, y_max);
            if (!single_channel) DrawTrace(&data->ch2_weak, config->dut2_dimmed, r, x_min, x_max, y_min, y_max);
            DrawTrace(&data->ch1_std, config->dut1_trace, r, x_min, x_max, y_min, y_max);
            if (!single_channel) DrawTrace(&data->ch2_std, config->dut2_trace, r, x_min, x_max, y_min, y_max);
        }
    } else {
        DrawTrace(data->ch1_active, config->dut1_trace, r, x_min, x_max, y_min, y_max);
        if (!single_channel) {
            DrawTrace(data->ch2_active, config->dut2_trace, r, x_min, x_max, y_min, y_max);
        }
    }
    
    for (int i = 0; i <= 10; i += 5) {
        float x_val = x_min + (x_max - x_min) * (10 - i) / 10.0f;
        float label_x = r.x + (i * r.width) / 10.0f;
        DrawText(TextFormat("%d", (int)x_val), (int)(label_x - 20), (int)(r.y + r.height + 10), 16, config->label_color);
        
        float y_val = y_min + (y_max - y_min) * i / 10.0f;
        float label_y = r.y + (i * r.height) / 10.0f;
        DrawText(TextFormat("%d", (int)y_val), (int)(r.x - 50), (int)(label_y - 6), 16, config->label_color);
    }
    
    DrawText("DUT Voltage", (int)(r.x + r.width/2 - 50), (int)(r.y + r.height + 35), 20, config->axis_color);
    DrawText("Current", (int)(r.x - 80), (int)(r.y + r.height/2 + 10), 20, config->axis_color);
    
    int legend_x = (int)(r.x + 20);
    int legend_y = (int)(r.y + r.height - 40);
    
    DrawLineEx((Vector2){(float)legend_x, (float)legend_y}, 
               (Vector2){(float)(legend_x + 40), (float)legend_y}, 4.0f, config->dut1_trace);
    DrawText("DUT1 (CH1 - Black Lead)", legend_x + 50, legend_y - 6, 12, config->dut1_trace);
    
    if (!single_channel) {
        DrawLineEx((Vector2){(float)legend_x, (float)(legend_y - 30)}, 
                   (Vector2){(float)(legend_x + 40), (float)(legend_y - 30)}, 4.0f, config->dut2_trace);
        DrawText("DUT2 (CH2 - Red Lead)", legend_x + 50, legend_y - 36, 12, config->dut2_trace);
    }
    
    DrawRectangleLinesEx(r, 2, config->border_color);
}

void PlotViewHandleZoom(PlotView* view, float wheel) {
    if (!view->auto_scale) {
        if (wheel > 0) {
            view->zoom *= 1.2f;
        } else {
            view->zoom /= 1.2f;
            if (view->zoom < 0.1f) view->zoom = 0.1f;
        }
    }
}

void PlotViewReset(PlotView* view) {
    view->zoom = 1.0f;
    view->pan_x = 0.0f;
    view->pan_y = 0.0f;
}

void PlotViewFitData(PlotView* view, CurveData* data, bool single_channel) {
    if (data->ch1_active->count == 0) return;
    
    float* all_x = malloc(sizeof(float) * MAX_SAMPLES * 4);
    float* all_y = malloc(sizeof(float) * MAX_SAMPLES * 4);
    int idx = 0;
    
    if (data->excitation_mode == 2 && data->ch1_std.count > 0 && data->ch1_weak.count > 0) {
        for (int i = 0; i < data->ch1_std.count; i++) {
            all_x[idx] = data->ch1_std.voltage[i];
            all_y[idx] = data->ch1_std.current[i];
            idx++;
        }
        for (int i = 0; i < data->ch1_weak.count; i++) {
            all_x[idx] = data->ch1_weak.voltage[i];
            all_y[idx] = data->ch1_weak.current[i];
            idx++;
        }
        if (!single_channel) {
            for (int i = 0; i < data->ch2_std.count; i++) {
                all_x[idx] = data->ch2_std.voltage[i];
                all_y[idx] = data->ch2_std.current[i];
                idx++;
            }
            for (int i = 0; i < data->ch2_weak.count; i++) {
                all_x[idx] = data->ch2_weak.voltage[i];
                all_y[idx] = data->ch2_weak.current[i];
                idx++;
            }
        }
    } else {
        for (int i = 0; i < data->ch1_active->count; i++) {
            all_x[idx] = data->ch1_active->voltage[i];
            all_y[idx] = data->ch1_active->current[i];
            idx++;
        }
        if (!single_channel) {
            for (int i = 0; i < data->ch2_active->count; i++) {
                all_x[idx] = data->ch2_active->voltage[i];
                all_y[idx] = data->ch2_active->current[i];
                idx++;
            }
        }
    }
    
    float data_x_min = MinFloat(all_x, idx);
    float data_x_max = MaxFloat(all_x, idx);
    float data_y_min = MinFloat(all_y, idx);
    float data_y_max = MaxFloat(all_y, idx);
    
    free(all_x);
    free(all_y);
    
    float x_margin = (data_x_max - data_x_min) * 0.2f;
    float y_margin = (data_y_max - data_y_min) * 0.2f;
    data_x_min -= x_margin;
    data_x_max += x_margin;
    data_y_min -= y_margin;
    data_y_max += y_margin;
    
    float data_x_range = data_x_max - data_x_min;
    float data_y_range = data_y_max - data_y_min;
    
    float base_x_min = 0;
    float base_x_max = ADC_MAX;
    float y_range = ADC_MAX - 700;
    float base_y_max = y_range / 8.0f;
    float base_y_min = -y_range * 7.0f / 8.0f;
    
    float base_x_range = base_x_max - base_x_min;
    float base_y_range = base_y_max - base_y_min;
    
    float zoom_x = base_x_range / data_x_range;
    float zoom_y = base_y_range / data_y_range;
    
    view->zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    if (view->zoom > 1.0f) view->zoom = 1.0f;
    
    float data_x_center = (data_x_min + data_x_max) / 2.0f;
    float data_y_center = (data_y_min + data_y_max) / 2.0f;
    
    float base_x_center = (base_x_min + base_x_max) / 2.0f;
    float base_y_center = (base_y_min + base_y_max) / 2.0f;
    
    view->pan_x = data_x_center - base_x_center;
    view->pan_y = data_y_center - base_y_center;
}

bool AcquireData(SerialPort* port, CurveData* data) {
    if (!port->is_open) return false;
    
    char cmd;
    bool store_as_weak;
    
    if (data->excitation_mode == 0) {
        cmd = 'T';
        store_as_weak = false;
    } else if (data->excitation_mode == 1) {
        cmd = 'W';
        store_as_weak = true;
    } else {
        if (data->alt_use_weak) {
            cmd = 'W';
            store_as_weak = true;
        } else {
            cmd = 'T';
            store_as_weak = false;
        }
        data->alt_use_weak = !data->alt_use_weak;
    }
    
    SerialFlush(port);
    SerialWrite(port, &cmd, 1);
    
    uint8_t buffer[2016];
    int total_read = 0;
    clock_t start = clock();
    
    while (total_read < 2016) {
        if ((clock() - start) * 1000 / CLOCKS_PER_SEC > 1000) break;
        
        int n = SerialRead(port, buffer + total_read, 2016 - total_read);
        if (n > 0) total_read += n;
    }
    
    if (total_read != 2016) return false;
    
    uint16_t values[1008];
    for (int i = 0; i < 1008; i++) {
        values[i] = (buffer[i*2+1] << 8) | buffer[i*2];
        values[i] &= 0x0FFF;
    }
    
    ChannelData* ch1_dest = store_as_weak ? &data->ch1_weak : &data->ch1_std;
    ChannelData* ch2_dest = store_as_weak ? &data->ch2_weak : &data->ch2_std;
    
    ch1_dest->count = 0;
    ch2_dest->count = 0;
    
    for (int i = 0; i < 336; i++) {
        uint16_t drive = values[i * 3];
        uint16_t ch1_raw = values[i * 3 + 1];
        uint16_t ch2_raw = values[i * 3 + 2];
        
        ch1_dest->voltage[ch1_dest->count] = (float)ch1_raw;
        ch1_dest->current[ch1_dest->count] = (float)(drive - ch1_raw);
        ch1_dest->count++;
        
        ch2_dest->voltage[ch2_dest->count] = (float)ch2_raw;
        ch2_dest->current[ch2_dest->count] = (float)(drive - ch2_raw);
        ch2_dest->count++;
    }
    
    data->last_was_weak = store_as_weak;
    data->ch1_active = store_as_weak ? &data->ch1_weak : &data->ch1_std;
    data->ch2_active = store_as_weak ? &data->ch2_weak : &data->ch2_std;
    
    return true;
}

int main(void) {
    Config config;
    ConfigLoad(&config, "curvebug.cfg");
    
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(config.window_width, config.window_height, "CurveBug - raylib Edition");
    SetExitKey(0);
    SetTargetFPS(60);
    
    SerialPort port = {0};
    bool connected = SerialOpen(&port, config.serial_port, 115200);
    
    CurveData data = {0};
    data.ch1_active = &data.ch1_std;
    data.ch2_active = &data.ch2_std;
    data.excitation_mode = 0;
    
    PlotView view;
    PlotViewInit(&view, (Rectangle){150, 100, 900, 800});
    
    bool paused = false;
    bool single_channel = false;
    bool show_settings = false;
    int frame_count = 0;
    
    float acquire_timer = 0;
    
    // Settings state
    SettingsTab active_tab = TAB_GENERAL;
    char port_edit[256];
    char width_edit[32];
    char height_edit[32];
    char keybind_edits[8][32];
    
    bool port_edit_mode = false;
    bool width_edit_mode = false;
    bool height_edit_mode = false;
    bool keybind_edit_mode[8] = {false};
    
    int active_color_picker = -1;
    
    strcpy(port_edit, config.serial_port);
    snprintf(width_edit, sizeof(width_edit), "%d", config.window_width);
    snprintf(height_edit, sizeof(height_edit), "%d", config.window_height);
    
    for (int i = 0; i < 8; i++) {
        strcpy(keybind_edits[i], config.keybinds[i]);
    }
    
    while (!WindowShouldClose()) {
        int screen_w = GetScreenWidth();
        int screen_h = GetScreenHeight();
        
        view.area = (Rectangle){
            150, 100,
            (float)(screen_w - 200),
            (float)(screen_h - 200)
        };
        
        float dt = GetFrameTime();
        acquire_timer += dt;
        
        if (!paused && !show_settings && acquire_timer >= 0.05f) {
            if (AcquireData(&port, &data)) {
                frame_count++;
            }
            acquire_timer = 0;
        }
        
        if (!show_settings) {
            if (IsKeyPressed(KEY_SPACE)) {
                data.excitation_mode = (data.excitation_mode + 1) % 3;
            }
            if (IsKeyPressed(KEY_P)) paused = !paused;
            if (IsKeyPressed(KEY_S)) single_channel = !single_channel;
            if (IsKeyPressed(KEY_A)) view.auto_scale = !view.auto_scale;
            if (IsKeyPressed(KEY_R)) PlotViewReset(&view);
            if (IsKeyPressed(KEY_F)) PlotViewFitData(&view, &data, single_channel);
            if (IsKeyPressed(KEY_F1)) {
                show_settings = !show_settings;
                if (show_settings) view.dragging = false;  // Reset dragging when entering settings
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                break;  // Exit program only if not in settings
            }
        } else {
            if (IsKeyPressed(KEY_ESCAPE)) {
                // Just close settings without saving
                strcpy(port_edit, config.serial_port);
                snprintf(width_edit, sizeof(width_edit), "%d", config.window_width);
                snprintf(height_edit, sizeof(height_edit), "%d", config.window_height);
                for (int i = 0; i < 8; i++) {
                    strcpy(keybind_edits[i], config.keybinds[i]);
                }
                show_settings = false;
                active_color_picker = -1;
            }
        }
        
        if (!show_settings) {
            float wheel = GetMouseWheelMove();
            if (wheel != 0) PlotViewHandleZoom(&view, wheel);
            
            // Define settings button bounds (same as in drawing code)
            Rectangle settings_btn = {
                (float)(screen_w - 140), 
                (float)(screen_h - 90), 
                120, 40
            };
            
            Vector2 mouse_pos = GetMousePosition();
            bool clicked_on_settings_btn = CheckCollisionPointRec(mouse_pos, settings_btn);
            
            // Only start dragging if not clicking on settings button
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !view.auto_scale && !clicked_on_settings_btn) {
                view.dragging = true;
                view.drag_start = mouse_pos;
                view.drag_offset = (Vector2){view.pan_x, view.pan_y};
            }
            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                view.dragging = false;
            }
            if (view.dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                Vector2 mouse = GetMousePosition();
                Vector2 delta = {mouse.x - view.drag_start.x, mouse.y - view.drag_start.y};
                float x_range = ADC_MAX / view.zoom;
                float y_range = (ADC_MAX - 700) / view.zoom;
                view.pan_x = view.drag_offset.x + (delta.x * x_range / view.area.width);
                view.pan_y = view.drag_offset.y - (delta.y * y_range / view.area.height);
            }
        } else {
            // Reset dragging state when in settings to prevent issues when returning
            view.dragging = false;
        }
        
        BeginDrawing();
        ClearBackground(config.bg_color);
        
        if (!show_settings) {
            PlotViewDraw(&view, &data, &config, single_channel);
            
            const char* mode_names[] = {"4.7K(T)", "100K WEAK(W)", "ALT"};
            DrawText(TextFormat("I-V Characteristics - %s %s Zoom:%.2fx Frame:%d", 
                                mode_names[data.excitation_mode],
                                view.auto_scale ? "[AUTO]" : "[FIXED]",
                                view.zoom, frame_count),
                     (int)view.area.x, (int)(view.area.y - 40), 20, config.axis_color);
            
            DrawText("SPACE=mode P=pause S=single A=auto F=fit R=reset F1=settings ESC=quit",
                     20, screen_h - 40, 20, LIGHTGRAY);
            
            DrawText(connected ? "Connected" : "NOT CONNECTED",
                     screen_w - 150, 20, 20, connected ? GREEN : RED);
            
            // Settings button at bottom right
            Rectangle settings_btn = {
                (float)(screen_w - 140), 
                (float)(screen_h - 90), 
                120, 40
            };
            
            // Check if mouse is hovering
            bool settings_btn_hover = CheckCollisionPointRec(GetMousePosition(), settings_btn);
            Color settings_btn_color = settings_btn_hover ? (Color){70, 130, 180, 255} : (Color){50, 100, 150, 255};
            
            DrawRectangleRec(settings_btn, settings_btn_color);
            DrawRectangleLinesEx(settings_btn, 2, WHITE);
            DrawText("Settings", (int)(settings_btn.x + 25), (int)(settings_btn.y + 10), 20, WHITE);
            
            // Handle settings button click
            if (settings_btn_hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                show_settings = true;
                view.dragging = false;  // Reset dragging state
            }

            if (paused) {
                DrawText("PAUSED", screen_w/2 - 80, screen_h/2, 48, YELLOW);
            }
        } else {
            Rectangle panel = {100, 50, (float)(screen_w - 200), (float)(screen_h - 100)};
            GuiPanel(panel, "Settings");
            
            const char* tab_names[] = {"General", "Colors", "Key Bindings"};
            Rectangle tab_area = {panel.x + 10, panel.y + 40, panel.width - 20, 40};
            int tab_click = DrawTabs(tab_area, tab_names, 3, active_tab);
            if (tab_click >= 0) active_tab = (SettingsTab)tab_click;
            
            int content_y = (int)(panel.y + 100);
            
            if (active_tab == TAB_GENERAL) {
                GuiLabel((Rectangle){panel.x + 30, (float)content_y, 150, 25}, "Serial Port:");
                if (GuiTextBox((Rectangle){panel.x + 200, (float)content_y, 300, 30}, 
                               port_edit, 256, port_edit_mode)) {
                    port_edit_mode = !port_edit_mode;
                }
                
                // Auto-find button
                if (GuiButton((Rectangle){panel.x + 510, (float)content_y, 120, 30}, "Auto Find")) {
                    char* found_port = SerialFindCurveBug();
                    if (found_port) {
                        strcpy(port_edit, found_port);
                        free(found_port);
                    }
                }
                
                content_y += 50;
                GuiLabel((Rectangle){panel.x + 30, (float)content_y, 150, 25}, "Window Width:");
                if (GuiTextBox((Rectangle){panel.x + 200, (float)content_y, 150, 30}, 
                               width_edit, 32, width_edit_mode)) {
                    width_edit_mode = !width_edit_mode;
                }
                
                content_y += 50;
                GuiLabel((Rectangle){panel.x + 30, (float)content_y, 150, 25}, "Window Height:");
                if (GuiTextBox((Rectangle){panel.x + 200, (float)content_y, 150, 30}, 
                               height_edit, 32, height_edit_mode)) {
                    height_edit_mode = !height_edit_mode;
                }
                
            } else if (active_tab == TAB_COLORS) {
                // Preset buttons
                if (GuiButton((Rectangle){panel.x + 30, (float)content_y, 120, 30}, "Dark Mode")) {
                    ConfigSetDarkMode(&config);
                }
                if (GuiButton((Rectangle){panel.x + 160, (float)content_y, 120, 30}, "Light Mode")) {
                    ConfigSetLightMode(&config);
                }
                
                content_y += 50;
                
                // Preview window
                Rectangle preview_bounds = {
                    panel.x + panel.width - 220,
                    (float)content_y,
                    200, 200
                };
                DrawColorPreview(preview_bounds, &config);
                
                const char* color_labels[] = {
                    "Background", "DUT1 Trace", "DUT2 Trace", "DUT1 Dimmed",
                    "DUT2 Dimmed", "Grid BG", "Grid", "Crosshair",
                    "Labels", "Axis", "Border"
                };
                Color* colors[] = {
                    &config.bg_color, &config.dut1_trace, &config.dut2_trace,
                    &config.dut1_dimmed, &config.dut2_dimmed, &config.grid_bg,
                    &config.grid_color, &config.crosshair, &config.label_color,
                    &config.axis_color, &config.border_color
                };
                
                int max_items_per_col = 6;
                
                for (int i = 0; i < 11; i++) {
                    int col = i / max_items_per_col;
                    int row = i % max_items_per_col;
                    int x = (int)(panel.x + 30 + col * 280);
                    int y = content_y + row * 90;
                    
                    GuiLabel((Rectangle){(float)x, (float)y, 150, 25}, color_labels[i]);
                    
                    Rectangle picker_bounds = {(float)(x + 160), (float)(y - 10), 100, 80};
                    
                    // Check if mouse is over entire picker area (including hue bar)
                    Rectangle picker_full_bounds = {picker_bounds.x, picker_bounds.y, 
                                                     picker_bounds.width + 20, picker_bounds.height};
                    
                    if (active_color_picker == -1 || active_color_picker == i) {
                        // Activate on click
                        if (CheckCollisionPointRec(GetMousePosition(), picker_full_bounds) && 
                            IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                            active_color_picker = i;
                        }
                        
                        if (active_color_picker == i) {
                            GuiColorPicker(picker_bounds, NULL, colors[i]);
                            
                            // Deactivate if clicked outside the full bounds
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && 
                                !CheckCollisionPointRec(GetMousePosition(), picker_full_bounds)) {
                                active_color_picker = -1;
                            }
                        } else {
                            DrawRectangleRec(picker_bounds, *colors[i]);
                            DrawRectangleLinesEx(picker_bounds, 2, GRAY);
                        }
                    } else {
                        DrawRectangleRec(picker_bounds, *colors[i]);
                        DrawRectangleLinesEx(picker_bounds, 2, DARKGRAY);
                    }
                }
                
            } else if (active_tab == TAB_KEYBINDS) {
                // Valid keys info
                DrawText("Valid keys: A-Z, SPACE, ESC, F1-F12", 
                        (int)(panel.x + 30), content_y - 5, 20, DARKGRAY);
                
                const char* keybind_names[] = {
                    "Pause", "Single", "Auto", "Fit", "Reset", "Mode", "Settings", "Quit"
                };
                
                for (int i = 0; i < 8; i++) {
                    int col = i / 4;
                    int row = i % 4;
                    int x = (int)(panel.x + 30 + col * 400);
                    int y = content_y + 20 + row * 60;
                    
                    GuiLabel((Rectangle){(float)x, (float)y, 150, 25}, 
                            TextFormat("%s:", keybind_names[i]));
                    if (GuiTextBox((Rectangle){(float)(x + 160), (float)y, 150, 30}, 
                                  keybind_edits[i], 32, keybind_edit_mode[i])) {
                        keybind_edit_mode[i] = !keybind_edit_mode[i];
                    }
                }
            }
            
            if (GuiButton((Rectangle){panel.x + panel.width - 240, panel.y + panel.height - 60, 
                                     100, 40}, "Save")) {
                strcpy(config.serial_port, port_edit);
                config.window_width = atoi(width_edit);
                config.window_height = atoi(height_edit);
                
                for (int i = 0; i < 8; i++) {
                    strcpy(config.keybinds[i], keybind_edits[i]);
                }
                
                ConfigSave(&config, "curvebug.cfg");
                show_settings = false;
                
                SerialClose(&port);
                connected = SerialOpen(&port, config.serial_port, 115200);
            }
            
            if (GuiButton((Rectangle){panel.x + panel.width - 130, panel.y + panel.height - 60, 
                                     100, 40}, "Cancel")) {
                strcpy(port_edit, config.serial_port);
                snprintf(width_edit, sizeof(width_edit), "%d", config.window_width);
                snprintf(height_edit, sizeof(height_edit), "%d", config.window_height);
                for (int i = 0; i < 8; i++) {
                    strcpy(keybind_edits[i], config.keybinds[i]);
                }
                show_settings = false;
            }
        }
        
        EndDrawing();
    }
    
    SerialClose(&port);
    CloseWindow();
    
    return 0;
}