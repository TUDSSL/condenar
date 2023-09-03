#include "Kernel.h"
#include "Metrics.h"
#include "Helpers/Misc.h"
#include "Kernel_Draw.h"
#include "am_util_stdio.h"

#ifdef KERNEL_ENABLE_METRICS

#define METRIC_MAX_COUNT 8
#define METRIC_STRING_SIZE 32
#define METRICS_UPDATE_INTERVAL_SECONDS 1

struct MetricStorage{
    char name[32];
    int value;
    bool used;
    int lastValue;
    bool shouldUpdate; //Whtever the value should be updated on the next update tick. false if the metric value is manually set, true if a rate approach is used
} metrics[16];

int nextTimestampToUpdateMetrics = 0;

void IncreaseMetricsInternal(const char *name, int value){
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(metrics[i].used && strcmp(metrics[i].name, name, METRIC_STRING_SIZE) == 0){
            metrics[i].value += value;
            metrics[i].shouldUpdate = true;
            return;
        }
    }
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(!metrics[i].used){
            metrics[i].used = true;
            strncpy(metrics[i].name, name, METRIC_STRING_SIZE);
            metrics[i].name[METRIC_STRING_SIZE - 1] = '\0'; //Ensure null termination
            metrics[i].value = value;
            metrics[i].shouldUpdate = true;
            return;
        }
    }
    LOG_W("Failed to increase metrics. Metrics full!");
}

#endif

void SetMetricDirect(const char *name, int value){
#ifdef KERNEL_ENABLE_METRICS
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(metrics[i].used && strcmp(metrics[i].name, name, METRIC_STRING_SIZE) == 0){
            metrics[i].value = value;
            metrics[i].shouldUpdate = false;
            return;
        }
    }
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(!metrics[i].used){
            metrics[i].used = true;
            strncpy(metrics[i].name, name, METRIC_STRING_SIZE);
            metrics[i].name[METRIC_STRING_SIZE - 1] = '\0'; //Ensure null termination
            metrics[i].value = value;
            metrics[i].shouldUpdate = false;
            return;
        }
    }
    LOG_W("Failed to set metric. Metrics full!");
#endif
}

void UpdateMetricsLastCountIfNeccessary(){
#ifdef KERNEL_ENABLE_METRICS
    if(k_GetTimeMS() < nextTimestampToUpdateMetrics){
        return;
    }
    nextTimestampToUpdateMetrics = k_GetTimeMS() + (1000*METRICS_UPDATE_INTERVAL_SECONDS);
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(metrics[i].used){
            metrics[i].lastValue = metrics[i].value / METRICS_UPDATE_INTERVAL_SECONDS;
            metrics[i].value = 0;
        }
    }
#endif
}

void k_draw_Metrics(){
    
#ifdef KERNEL_ENABLE_METRICS
    UpdateMetricsLastCountIfNeccessary();
    
    int y = 12;
    for(int i = 0; i < METRIC_MAX_COUNT; i++){
        if(metrics[i].used){
            //Measure text size
            char buffer[48];
            int written = am_util_stdio_snprintf(buffer, 48, "%s: %d", metrics[i].name, metrics[i].lastValue);
            buffer[written] = '\0';
            int textWidth = strlen(buffer) * 8;
            
            k_draw_FillRectangle(0, y, textWidth + 2, 8 + 2, K_COLOR_WHITE);
            //Fill background rectangle
            k_draw_DrawTextLeftAligned(1, y+1, 1, buffer);
            y += 8 + 2;
        }
    }
#endif
}

