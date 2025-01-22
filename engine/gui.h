#ifndef __GUI_H
#define __GUI_H
#include "raylib.h"
#include "raygui.h"

// Generic Component
typedef struct UIComp {
    Rectangle bbox;
    GuiState state;
    float alpha;
    bool pressed;
    Font font;
} UIComp_t;

typedef struct VertScrollArea {
    UIComp_t comp;  // Display area + scrollbar
    Rectangle display_area; // Display dimension
    
    RenderTexture2D canvas; // Complete canvas
    
    unsigned int n_items;
    unsigned int max_items;
    unsigned int curr_selection;
    unsigned int item_height;
    unsigned int item_padding;

    float scroll_pos;
    Vector2 scroll_bounds;
    
    UIComp_t scroll_bar;

} VertScrollArea_t;

void init_UI(void);
void vert_scrollarea_init(VertScrollArea_t* scroll_area, Rectangle display_area, Vector2 canvas_dims);
void vert_scrollarea_set_item_dims(VertScrollArea_t* scroll_area, unsigned int item_height, unsigned int item_padding);
bool vert_scrollarea_n_items(VertScrollArea_t* scroll_area, unsigned int n_items);
void vert_scrollarea_insert_item(VertScrollArea_t* scroll_area, char* str, unsigned int item_idx);
unsigned int vert_scrollarea_set_pos(VertScrollArea_t* scroll_area, Vector2 pos);
void vert_scrollarea_refocus(VertScrollArea_t* scroll_area);
void vert_scrollarea_render(VertScrollArea_t* scroll_area);
void vert_scrollarea_free(VertScrollArea_t* scroll_area);

static inline void ScrollAreaRenderBegin(VertScrollArea_t* scroll)
{
    BeginTextureMode(scroll->canvas);
}
#define ScrollAreaRenderEnd() EndTextureMode()

void UI_button(const UIComp_t* bbox, const char* text);
float UI_slider(const UIComp_t* comp, const char *textLeft, const char *textRight, float *value, float minValue, float maxValue);
float UI_vert_slider(const UIComp_t* comp, const char *textTop, const char *textBottom, float* value, float minValue, float maxValue);
void hover_text(const UIComp_t* comp, Font font, const char* text, Vector2 pos, int font_size, int spacing, Color colour);
#endif
