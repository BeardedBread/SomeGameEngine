#ifndef __GUI_H
#define __GUI_H
#include "raylib.h"
#include "raygui.h"

typedef struct UIComp {
    Rectangle bbox;
    GuiState state;
    float alpha;
    bool pressed;
} UIComp_t;

void init_UI(void);
void UI_button(const UIComp_t* bbox, const char* text);
float UI_slider(const UIComp_t* comp, const char *textLeft, const char *textRight, float *value, float minValue, float maxValue);
float UI_vert_slider(const UIComp_t* comp, const char *textTop, const char *textBottom, float* value, float minValue, float maxValue);
#endif
