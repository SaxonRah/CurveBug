#ifndef PLOTTER_H
#define PLOTTER_H

#include <raylib.h>
#include <stdbool.h>

#define ADC_MAX 2800
#define ADC_ORIGIN 2048
#define MAX_SAMPLES 336

typedef struct {
    float voltage[MAX_SAMPLES];
    float current[MAX_SAMPLES];
    int count;
} ChannelData;

typedef struct {
    ChannelData ch1_std;
    ChannelData ch2_std;
    ChannelData ch1_weak;
    ChannelData ch2_weak;
    
    ChannelData* ch1_active;
    ChannelData* ch2_active;
    
    bool last_was_weak;
    int excitation_mode; // 0=4.7K, 1=100K, 2=ALT
    bool alt_use_weak;
} CurveData;

typedef struct {
    Rectangle area;
    bool auto_scale;
    float zoom;
    float pan_x;
    float pan_y;
    bool dragging;
    Vector2 drag_start;
    Vector2 drag_offset;
} PlotView;

void PlotViewInit(PlotView* view, Rectangle area);
void PlotViewDraw(PlotView* view, CurveData* data, Config* config, bool single_channel);
void PlotViewHandlePan(PlotView* view, Vector2 mouse_pos);
void PlotViewHandleZoom(PlotView* view, float wheel);
void PlotViewFitData(PlotView* view, CurveData* data, bool single_channel);
void PlotViewReset(PlotView* view);

#endif