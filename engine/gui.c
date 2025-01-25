#include "gui.h"
#include "raylib.h"
#include <string.h>
#include <stdio.h>

#define RAYGUI_MAX_CONTROLS             16      // Maximum number of standard controls
#define RAYGUI_MAX_PROPS_BASE           16      // Maximum number of standard properties
#define RAYGUI_MAX_PROPS_EXTENDED        8      // Maximum number of extended properties

#define MAX_LINE_BUFFER_SIZE    256
static unsigned int guiStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };
static bool guiStyleLoaded = false;         // Style loaded flag for lazy style initialization
static Font guiFont = { 0 };                // Gui current font (WARNING: highly coupled to raylib)
                                            //
// Check if two rectangles are equal, used to validate a slider bounds as an id
#ifndef CHECK_BOUNDS_ID
    #define CHECK_BOUNDS_ID(src, dst) ((src.x == dst.x) && (src.y == dst.y) && (src.width == dst.width) && (src.height == dst.height))
#endif

typedef enum { BORDER = 0, BASE, TEXT, OTHER } GuiPropertyElement;

void GuiLoadStyleDefault(void)
{
    // We set this variable first to avoid cyclic function calls
    // when calling GuiSetStyle() and GuiGetStyle()
    guiStyleLoaded = true;

    // Initialize default LIGHT style property values
    GuiSetStyle(DEFAULT, BORDER_COLOR_NORMAL, 0x838383ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_NORMAL, 0xc9c9c9ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, 0x686868ff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_FOCUSED, 0x5bb2d9ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_FOCUSED, 0xc9effeff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_FOCUSED, 0x6c9bbcff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_PRESSED, 0x0492c7ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_PRESSED, 0x97e8ffff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_PRESSED, 0x368bafff);
    GuiSetStyle(DEFAULT, BORDER_COLOR_DISABLED, 0xb5c1c2ff);
    GuiSetStyle(DEFAULT, BASE_COLOR_DISABLED, 0xe6e9e9ff);
    GuiSetStyle(DEFAULT, TEXT_COLOR_DISABLED, 0xaeb7b8ff);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);                       // WARNING: Some controls use other values
    GuiSetStyle(DEFAULT, TEXT_PADDING, 0);                       // WARNING: Some controls use other values
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER); // WARNING: Some controls use other values

    // Initialize control-specific property values
    // NOTE: Those properties are in default list but require specific values by control type
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(BUTTON, BORDER_WIDTH, 2);
    GuiSetStyle(SLIDER, TEXT_PADDING, 4);
    GuiSetStyle(CHECKBOX, TEXT_PADDING, 4);
    GuiSetStyle(CHECKBOX, TEXT_ALIGNMENT, TEXT_ALIGN_RIGHT);
    GuiSetStyle(TEXTBOX, TEXT_PADDING, 4);
    GuiSetStyle(TEXTBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(VALUEBOX, TEXT_PADDING, 0);
    GuiSetStyle(VALUEBOX, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(SPINNER, TEXT_PADDING, 0);
    GuiSetStyle(SPINNER, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiSetStyle(STATUSBAR, TEXT_PADDING, 8);
    GuiSetStyle(STATUSBAR, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);

    // Initialize extended property values
    // NOTE: By default, extended property values are initialized to 0
    GuiSetStyle(DEFAULT, TEXT_SIZE, 10);                // DEFAULT, shared by all controls
    GuiSetStyle(DEFAULT, TEXT_SPACING, 1);              // DEFAULT, shared by all controls
    GuiSetStyle(DEFAULT, LINE_COLOR, 0x90abb5ff);       // DEFAULT specific property
    GuiSetStyle(DEFAULT, BACKGROUND_COLOR, 0xf5f5f5ff); // DEFAULT specific property
    GuiSetStyle(TOGGLE, GROUP_PADDING, 2);
    GuiSetStyle(SLIDER, SLIDER_WIDTH, 16);
    GuiSetStyle(SLIDER, SLIDER_PADDING, 1);
    GuiSetStyle(PROGRESSBAR, PROGRESS_PADDING, 1);
    GuiSetStyle(CHECKBOX, CHECK_PADDING, 1);
    GuiSetStyle(COMBOBOX, COMBO_BUTTON_WIDTH, 32);
    GuiSetStyle(COMBOBOX, COMBO_BUTTON_SPACING, 2);
    GuiSetStyle(DROPDOWNBOX, ARROW_PADDING, 16);
    GuiSetStyle(DROPDOWNBOX, DROPDOWN_ITEMS_SPACING, 2);
    GuiSetStyle(TEXTBOX, TEXT_LINES_SPACING, 4);
    GuiSetStyle(TEXTBOX, TEXT_INNER_PADDING, 4);
    GuiSetStyle(SPINNER, SPIN_BUTTON_WIDTH, 24);
    GuiSetStyle(SPINNER, SPIN_BUTTON_SPACING, 2);
    GuiSetStyle(SCROLLBAR, BORDER_WIDTH, 0);
    GuiSetStyle(SCROLLBAR, ARROWS_VISIBLE, 0);
    GuiSetStyle(SCROLLBAR, ARROWS_SIZE, 6);
    GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_PADDING, 0);
    GuiSetStyle(SCROLLBAR, SCROLL_SLIDER_SIZE, 16);
    GuiSetStyle(SCROLLBAR, SCROLL_PADDING, 0);
    GuiSetStyle(SCROLLBAR, SCROLL_SPEED, 12);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_HEIGHT, 28);
    GuiSetStyle(LISTVIEW, LIST_ITEMS_SPACING, 2);
    GuiSetStyle(LISTVIEW, SCROLLBAR_WIDTH, 12);
    GuiSetStyle(LISTVIEW, SCROLLBAR_SIDE, SCROLLBAR_RIGHT_SIDE);
    GuiSetStyle(COLORPICKER, COLOR_SELECTOR_SIZE, 8);
    GuiSetStyle(COLORPICKER, HUEBAR_WIDTH, 16);
    GuiSetStyle(COLORPICKER, HUEBAR_PADDING, 8);
    GuiSetStyle(COLORPICKER, HUEBAR_SELECTOR_HEIGHT, 8);
    GuiSetStyle(COLORPICKER, HUEBAR_SELECTOR_OVERFLOW, 2);

    if (guiFont.texture.id != GetFontDefault().texture.id)
    {
        // Unload previous font texture
        UnloadTexture(guiFont.texture);

        // Setup default raylib font
        guiFont = GetFontDefault();

        // Setup default raylib font rectangle
        Rectangle whiteChar = { 41, 46, 2, 8 };
        SetShapesTexture(guiFont.texture, whiteChar);
    }
}

void GuiSetStyle(int control, int property, int value)
{
    if (!guiStyleLoaded) GuiLoadStyleDefault();
    guiStyle[control*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + property] = value;

    // Default properties are propagated to all controls
    if ((control == 0) && (property < RAYGUI_MAX_PROPS_BASE))
    {
        for (int i = 1; i < RAYGUI_MAX_CONTROLS; i++) guiStyle[i*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + property] = value;
    }
}

int GuiGetStyle(int control, int property)
{
    if (!guiStyleLoaded) GuiLoadStyleDefault();
    return guiStyle[control*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED) + property];
}

static int GetTextWidth(const char* text)
{
    #if !defined(ICON_TEXT_PADDING)
        #define ICON_TEXT_PADDING   4
    #endif

    Vector2 textSize = { 0 };
    int textIconOffset = 0;

    if ((text != NULL) && (text[0] != '\0'))
    {
        if (text[0] == '#')
        {
            for (int i = 1; (text[i] != '\0') && (i < 5); i++)
            {
                if (text[i] == '#')
                {
                    textIconOffset = i;
                    break;
                }
            }
        }

        text += textIconOffset;

        // Make sure guiFont is set, GuiGetStyle() initializes it lazynessly
        float fontSize = (float)GuiGetStyle(DEFAULT, TEXT_SIZE);

        // Custom MeasureText() implementation
        if ((guiFont.texture.id > 0) && (text != NULL))
        {
            // Get size in bytes of text, considering end of line and line break
            int size = 0;
            for (int i = 0; i < MAX_LINE_BUFFER_SIZE; i++)
            {
                if ((text[i] != '\0') && (text[i] != '\n')) size++;
                else break;
            }

            float scaleFactor = fontSize/(float)guiFont.baseSize;
            textSize.y = (float)guiFont.baseSize*scaleFactor;
            float glyphWidth = 0.0f;

            for (int i = 0, codepointSize = 0; i < size; i += codepointSize)
            {
                int codepoint = GetCodepointNext(&text[i], &codepointSize);
                int codepointIndex = GetGlyphIndex(guiFont, codepoint);

                if (guiFont.glyphs[codepointIndex].advanceX == 0) glyphWidth = ((float)guiFont.recs[codepointIndex].width*scaleFactor + (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                else glyphWidth = ((float)guiFont.glyphs[codepointIndex].advanceX*scaleFactor + GuiGetStyle(DEFAULT, TEXT_SPACING));

                textSize.x += glyphWidth;
            }
        }

    }

    return (int)textSize.x;
}
const char** GetTextLines(const char* text, int* count)
{
    #define RAYGUI_MAX_TEXT_LINES   128

    static const char *lines[RAYGUI_MAX_TEXT_LINES] = { 0 };
    for (int i = 0; i < RAYGUI_MAX_TEXT_LINES; i++) lines[i] = NULL;    // Init NULL pointers to substrings

    int textSize = (int)strlen(text);

    lines[0] = text;
    //int len = 0;
    *count = 1;
    //int lineSize = 0;   // Stores current line size, not returned

    for (int i = 0, k = 0; (i < textSize) && (*count < RAYGUI_MAX_TEXT_LINES); i++)
    {
        if (text[i] == '\n')
        {
            //lineSize = len;
            k++;
            lines[k] = &text[i + 1];     // WARNING: next value is valid?
            //len = 0;
            *count += 1;
        }
        //else len++;
    }

    //lines[*count - 1].size = len;

    return lines;
}

static void GuiDrawRectangle(Rectangle rec, int borderWidth, Color borderColor, Color color)
{
    if (color.a > 0)
    {
        // Draw rectangle filled with color
        DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, (int)rec.height, color);
    }

    if (borderWidth > 0)
    {
        // Draw rectangle border lines with color
        DrawRectangle((int)rec.x, (int)rec.y, (int)rec.width, borderWidth, borderColor);
        DrawRectangle((int)rec.x, (int)rec.y + borderWidth, borderWidth, (int)rec.height - 2*borderWidth, borderColor);
        DrawRectangle((int)rec.x + (int)rec.width - borderWidth, (int)rec.y + borderWidth, borderWidth, (int)rec.height - 2*borderWidth, borderColor);
        DrawRectangle((int)rec.x, (int)rec.y + (int)rec.height - borderWidth, (int)rec.width, borderWidth, borderColor);
    }
}

static void GuiDrawText(const char* text, Rectangle bounds, int alignment, Color tint)
{
    #define TEXT_VALIGN_PIXEL_OFFSET(h)  ((int)h%2)     // Vertical alignment for pixel perfect

    #if !defined(ICON_TEXT_PADDING)
        #define ICON_TEXT_PADDING   4
    #endif

    // We process the text lines one by one
    if ((text != NULL) && (text[0] != '\0'))
    {
        // Get text lines ('\n' delimiter) to process lines individually
        // NOTE: We can't use GuiTextSplit() because it can be already use before calling
        // GuiDrawText() and static buffer would be overriden :(
        int lineCount = 0;
        const char **lines = GetTextLines(text, &lineCount);

        //Rectangle textBounds = GetTextBounds(LABEL, bounds);
        float totalHeight = (float)(lineCount*GuiGetStyle(DEFAULT, TEXT_SIZE) + (lineCount - 1)*GuiGetStyle(DEFAULT, TEXT_SIZE)/2);

        float posOffsetY = 0;

        for (int i = 0; i < lineCount; i++)
        {
            // Get text position depending on alignment and iconId
            //---------------------------------------------------------------------------------
            Vector2 position = { bounds.x, bounds.y };

            // TODO: We get text size after icon has been processed
            // WARNING: GetTextWidth() also processes text icon to get width! -> Really needed?
            int textSizeX = GetTextWidth(lines[i]);

            // Check guiTextAlign global variables
            switch (alignment)
            {
                case TEXT_ALIGN_LEFT:
                {
                    position.x = bounds.x;
                    position.y = bounds.y + posOffsetY + bounds.height/2 - totalHeight/2 + TEXT_VALIGN_PIXEL_OFFSET(bounds.height);
                } break;
                case TEXT_ALIGN_CENTER:
                {
                    position.x = bounds.x +  bounds.width/2 - textSizeX/2;
                    position.y = bounds.y + posOffsetY + bounds.height/2 - totalHeight/2 + TEXT_VALIGN_PIXEL_OFFSET(bounds.height);
                } break;
                case TEXT_ALIGN_RIGHT:
                {
                    position.x = bounds.x + bounds.width - textSizeX;
                    position.y = bounds.y + posOffsetY + bounds.height/2 - totalHeight/2 + TEXT_VALIGN_PIXEL_OFFSET(bounds.height);
                } break;
                default: break;
            }

            // NOTE: Make sure we get pixel-perfect coordinates,
            // In case of decimals we got weird text positioning
            position.x = (float)((int)position.x);
            position.y = (float)((int)position.y);
            //---------------------------------------------------------------------------------

            // Draw text (with icon if available)
            //---------------------------------------------------------------------------------
            //DrawTextEx(guiFont, text, position, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), (float)GuiGetStyle(DEFAULT, TEXT_SPACING), tint);

            // Get size in bytes of text, 
            // considering end of line and line break
            int size = 0;
            for (int c = 0; (lines[i][c] != '\0') && (lines[i][c] != '\n'); c++, size++){ }
            float scaleFactor = (float)GuiGetStyle(DEFAULT, TEXT_SIZE)/guiFont.baseSize;

            int textOffsetY = 0;
            float textOffsetX = 0.0f;
            for (int c = 0, codepointSize = 0; c < size; c += codepointSize)
            {
                int codepoint = GetCodepointNext(&lines[i][c], &codepointSize);
                int index = GetGlyphIndex(guiFont, codepoint);

                // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
                // but we need to draw all of the bad bytes using the '?' symbol moving one byte
                if (codepoint == 0x3f) codepointSize = 1;

                if (codepoint == '\n') break;   // WARNING: Lines are already processed manually, no need to keep drawing after this codepoint
                else
                {
                    if ((codepoint != ' ') && (codepoint != '\t'))
                    {
                        // TODO: Draw only required text glyphs fitting the bounds.width, '...' can be appended at the end of the text
                        if (textOffsetX < bounds.width)
                        {
                            DrawTextCodepoint(guiFont, codepoint, (Vector2){ position.x + textOffsetX, position.y + textOffsetY }, (float)GuiGetStyle(DEFAULT, TEXT_SIZE), tint);
                        }
                    }

                    if (guiFont.glyphs[index].advanceX == 0) textOffsetX += ((float)guiFont.recs[index].width*scaleFactor + (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                    else textOffsetX += ((float)guiFont.glyphs[index].advanceX*scaleFactor + (float)GuiGetStyle(DEFAULT, TEXT_SPACING));
                }
            }

            posOffsetY += (float)GuiGetStyle(DEFAULT, TEXT_SIZE)*1.5f;    // TODO: GuiGetStyle(DEFAULT, TEXT_LINE_SPACING)?
            //---------------------------------------------------------------------------------
        }
    }
}

static Rectangle GetTextBounds(int control, Rectangle bounds)
{
    Rectangle textBounds = bounds;

    textBounds.x = bounds.x + GuiGetStyle(control, BORDER_WIDTH);
    textBounds.y = bounds.y + GuiGetStyle(control, BORDER_WIDTH);
    textBounds.width = bounds.width - 2*GuiGetStyle(control, BORDER_WIDTH) - 2*GuiGetStyle(control, TEXT_PADDING);
    textBounds.height = bounds.height - 2*GuiGetStyle(control, BORDER_WIDTH);

    // Consider TEXT_PADDING properly, depends on control type and TEXT_ALIGNMENT
    switch (control)
    {
        case COMBOBOX: textBounds.width -= (GuiGetStyle(control, COMBO_BUTTON_WIDTH) + GuiGetStyle(control, COMBO_BUTTON_SPACING)); break;
        //case VALUEBOX: break;   // NOTE: ValueBox text value always centered, text padding applies to label
        default:
        {
            if (GuiGetStyle(control, TEXT_ALIGNMENT) == TEXT_ALIGN_RIGHT) textBounds.x -= GuiGetStyle(control, TEXT_PADDING);
            else textBounds.x += GuiGetStyle(control, TEXT_PADDING);
            textBounds.width -= 2 * GuiGetStyle(control, TEXT_PADDING);
        }
        break;
    }

    // TODO: Special cases (no label): COMBOBOX, DROPDOWNBOX, LISTVIEW (scrollbar?)
    // More special cases (label on side): CHECKBOX, SLIDER, VALUEBOX, SPINNER

    return textBounds;
}
//------- End of raygui section---------//


void UI_button(const UIComp_t* comp, const char* text)
{
    GuiDrawRectangle(comp->bbox, GuiGetStyle(BUTTON, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(BUTTON, BORDER + (comp->state*3))), comp->alpha), Fade(GetColor(GuiGetStyle(BUTTON, BASE + (comp->state*3))), comp->alpha));
    GuiDrawText(text, GetTextBounds(BUTTON, comp->bbox), GuiGetStyle(BUTTON, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(BUTTON, TEXT + (comp->state*3))), comp->alpha));
    
}

void hover_text(const UIComp_t* comp, Font font, const char* text, Vector2 pos, int font_size, int spacing, Color colour) {
    if (comp->state == STATE_FOCUSED) {
        DrawTextEx(font, text, pos, font_size, spacing, Fade(colour, 0.1));
        pos.y -= font_size >> 2;
    }
    DrawTextEx(font, text, pos, font_size, spacing, colour);
}


// Slider control with pro parameters
// NOTE: Other GuiSlider*() controls use this one
int GuiSliderPro(const UIComp_t* comp,  const char *textLeft, const char *textRight, float *value, float minValue, float maxValue, int sliderWidth)
{
    Rectangle bounds = comp->bbox;
    int result = 0;
    GuiState state = comp->state;

    float temp = (maxValue - minValue)/2.0f;
    if (value == NULL) value = &temp;

    int sliderValue = (int)(((*value - minValue)/(maxValue - minValue))*(bounds.width - 2*GuiGetStyle(SLIDER, BORDER_WIDTH)));

    Rectangle slider = { bounds.x, bounds.y + GuiGetStyle(SLIDER, BORDER_WIDTH) + GuiGetStyle(SLIDER, SLIDER_PADDING),
                         0, bounds.height - 2*GuiGetStyle(SLIDER, BORDER_WIDTH) - 2*GuiGetStyle(SLIDER, SLIDER_PADDING) };

    if (sliderWidth > 0)        // Slider
    {
        slider.x += (sliderValue - (sliderWidth >> 1));
        slider.width = (float)sliderWidth;
    }
    else if (sliderWidth == 0)  // SliderBar
    {
        slider.x += GuiGetStyle(SLIDER, BORDER_WIDTH);
        slider.width = (float)sliderValue;
    }

    // Update control
    //--------------------------------------------------------------------
    //if ((state != STATE_DISABLED) && !guiLocked)
    if (state != STATE_DISABLED)
    {
        Vector2 mousePoint = GetMousePosition();

        if (comp->pressed) // Keep dragging outside of bounds
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                //if (CHECK_BOUNDS_ID(bounds, guiSliderActive))
                {
                    state = STATE_PRESSED;

                    // Get equivalent value and slider position from mousePosition.x
                    *value = ((maxValue - minValue)*(mousePoint.x - (float)(bounds.x + sliderWidth/2)))/(float)(bounds.width - sliderWidth) + minValue;
                }
            }
            //else
            //{
            //    guiSliderDragging = false;
            //    guiSliderActive = RAYGUI_CLITERAL(Rectangle){ 0, 0, 0, 0 };
            //}
        }
        else if (CheckCollisionPointRec(mousePoint, bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                state = STATE_PRESSED;
                //guiSliderDragging = true;
                //guiSliderActive = bounds; // Store bounds as an identifier when dragging starts

                if (!CheckCollisionPointRec(mousePoint, slider))
                {
                    // Get equivalent value and slider position from mousePosition.x
                    *value = ((maxValue - minValue)*(mousePoint.x - (float)(bounds.x + (sliderWidth >> 1) )))/(float)(bounds.width - sliderWidth) + minValue;

                    if (sliderWidth > 0) slider.x = mousePoint.x - slider.width/2;      // Slider
                    else if (sliderWidth == 0) slider.width = (float)sliderValue;       // SliderBar
                }
            }
            else state = STATE_FOCUSED;
        }

        if (*value > maxValue) *value = maxValue;
        else if (*value < minValue) *value = minValue;
    }

    // Bar limits check
    if (sliderWidth > 0)        // Slider
    {
        if (slider.x <= (bounds.x + GuiGetStyle(SLIDER, BORDER_WIDTH))) slider.x = bounds.x + GuiGetStyle(SLIDER, BORDER_WIDTH);
        else if ((slider.x + slider.width) >= (bounds.x + bounds.width)) slider.x = bounds.x + bounds.width - slider.width - GuiGetStyle(SLIDER, BORDER_WIDTH);
    }
    else if (sliderWidth == 0)  // SliderBar
    {
        if (slider.width > bounds.width) slider.width = bounds.width - 2*GuiGetStyle(SLIDER, BORDER_WIDTH);
    }
    //--------------------------------------------------------------------

    // Draw control
    //--------------------------------------------------------------------
    GuiDrawRectangle(bounds, GuiGetStyle(SLIDER, BORDER_WIDTH), GetColor(GuiGetStyle(SLIDER, BORDER + (state*3))), GetColor(GuiGetStyle(SLIDER, (state != STATE_DISABLED)?  BASE_COLOR_NORMAL : BASE_COLOR_DISABLED)));

    // Draw slider internal bar (depends on state)
    if (state == STATE_NORMAL) GuiDrawRectangle(slider, 0, BLANK, GetColor(GuiGetStyle(SLIDER, BASE_COLOR_PRESSED)));
    else if (state == STATE_FOCUSED) GuiDrawRectangle(slider, 0, BLANK, GetColor(GuiGetStyle(SLIDER, TEXT_COLOR_FOCUSED)));
    else if (state == STATE_PRESSED) GuiDrawRectangle(slider, 0, BLANK, GetColor(GuiGetStyle(SLIDER, TEXT_COLOR_PRESSED)));

    // Draw left/right text if provided
    if (textLeft != NULL)
    {
        Rectangle textBounds = { 0 };
        textBounds.width = (float)GetTextWidth(textLeft);
        textBounds.height = (float)GuiGetStyle(DEFAULT, TEXT_SIZE);
        textBounds.x = bounds.x - textBounds.width - GuiGetStyle(SLIDER, TEXT_PADDING);
        textBounds.y = bounds.y + bounds.height/2 - (GuiGetStyle(DEFAULT, TEXT_SIZE) >> 1);

        GuiDrawText(textLeft, textBounds, TEXT_ALIGN_RIGHT, GetColor(GuiGetStyle(SLIDER, TEXT + (state*3))));
    }

    if (textRight != NULL)
    {
        Rectangle textBounds = { 0 };
        textBounds.width = (float)GetTextWidth(textRight);
        textBounds.height = (float)GuiGetStyle(DEFAULT, TEXT_SIZE);
        textBounds.x = bounds.x + bounds.width + GuiGetStyle(SLIDER, TEXT_PADDING);
        textBounds.y = bounds.y + bounds.height/2 - (GuiGetStyle(DEFAULT, TEXT_SIZE) >> 1);

        GuiDrawText(textRight, textBounds, TEXT_ALIGN_LEFT, GetColor(GuiGetStyle(SLIDER, TEXT + (state*3))));
    }
    //--------------------------------------------------------------------

    return result;
}
//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
float GuiVerticalSliderPro(const UIComp_t* comp, const char *textTop, const char *textBottom, float* in_value, float minValue, float maxValue, int sliderHeight)
{
    float value = *in_value;
    GuiState state = comp->state;
    Rectangle bounds = comp->bbox;

    int sliderValue = (int)(((value - minValue)/(maxValue - minValue)) * (bounds.height - 2 * GuiGetStyle(SLIDER, BORDER_WIDTH)));

    Rectangle slider = {
        bounds.x + GuiGetStyle(SLIDER, BORDER_WIDTH) + GuiGetStyle(SLIDER, SLIDER_PADDING),
        bounds.y + bounds.height - sliderValue,
        bounds.width - 2*GuiGetStyle(SLIDER, BORDER_WIDTH) - 2*GuiGetStyle(SLIDER, SLIDER_PADDING),
        0.0f,
    };

    if (sliderHeight > 0)        // Slider
    {
        slider.y -= sliderHeight >> 1;
        slider.height = (float)sliderHeight;
    }
    else if (sliderHeight == 0)  // SliderBar
    {
        slider.y -= GuiGetStyle(SLIDER, BORDER_WIDTH);
        slider.height = (float)sliderValue;
    }
    // Update control
    //--------------------------------------------------------------------
    //if ((state != STATE_DISABLED) && !guiLocked)
    if (state != STATE_DISABLED)
    {
        Vector2 mousePoint = GetMousePosition();

        if (CheckCollisionPointRec(mousePoint, bounds))
        {
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
            {
                state = STATE_PRESSED;

                // Get equivalent value and slider position from mousePoint.x
                float normalizedValue = (bounds.y + bounds.height - mousePoint.y - (float)(sliderHeight >> 1)) / (bounds.height - (float)sliderHeight);
                value = (maxValue - minValue) * normalizedValue + minValue;

                if (sliderHeight > 0) slider.y = mousePoint.y - slider.height / 2;  // Slider
                else if (sliderHeight == 0)                                          // SliderBar
                {
                    slider.y = mousePoint.y;
                    slider.height = bounds.y + bounds.height - slider.y - GuiGetStyle(SLIDER, BORDER_WIDTH);
                }
            }
            else state = STATE_FOCUSED;
        }

        if (value > maxValue) value = maxValue;
        else if (value < minValue) value = minValue;
    }


    // Bar limits check
    if (sliderHeight > 0)        // Slider
    {
        if (slider.y < (bounds.y + GuiGetStyle(SLIDER, BORDER_WIDTH))) slider.y = bounds.y + GuiGetStyle(SLIDER, BORDER_WIDTH);
        else if ((slider.y + slider.height) >= (bounds.y + bounds.height)) slider.y = bounds.y + bounds.height - slider.height - GuiGetStyle(SLIDER, BORDER_WIDTH);
    }
    else if (sliderHeight == 0)  // SliderBar
    {
        if (slider.y < (bounds.y + GuiGetStyle(SLIDER, BORDER_WIDTH)))
        {
            slider.y = bounds.y + GuiGetStyle(SLIDER, BORDER_WIDTH);
            slider.height = bounds.height - 2*GuiGetStyle(SLIDER, BORDER_WIDTH);
        }
    }

    //--------------------------------------------------------------------
    // Draw control
    //--------------------------------------------------------------------
    GuiDrawRectangle(bounds, GuiGetStyle(SLIDER, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(SLIDER, BORDER + (state*3))), comp->alpha), Fade(GetColor(GuiGetStyle(SLIDER, (state != STATE_DISABLED)?  BASE_COLOR_NORMAL : BASE_COLOR_DISABLED)), comp->alpha));

    // Draw slider internal bar (depends on state)
    if ((state == STATE_NORMAL) || (state == STATE_PRESSED)) GuiDrawRectangle(slider, 0, BLANK, Fade(GetColor(GuiGetStyle(SLIDER, BASE_COLOR_PRESSED)), comp->alpha));
    else if (state == STATE_FOCUSED) GuiDrawRectangle(slider, 0, BLANK, Fade(GetColor(GuiGetStyle(SLIDER, TEXT_COLOR_FOCUSED)), comp->alpha));

    // Draw top/bottom text if provided
    if (textTop != NULL)
    {
        Rectangle textBounds = { 0 };
        textBounds.width = (float)GetTextWidth(textTop);
        textBounds.height = (float)GuiGetStyle(DEFAULT, TEXT_SIZE);
        textBounds.x = bounds.x + bounds.width/2 - textBounds.width/2;
        textBounds.y = bounds.y - textBounds.height - GuiGetStyle(SLIDER, TEXT_PADDING);

        GuiDrawText(textTop, textBounds, TEXT_ALIGN_RIGHT, Fade(GetColor(GuiGetStyle(SLIDER, TEXT + (state*3))), comp->alpha));
    }

    if (textBottom != NULL)
    {
        Rectangle textBounds = { 0 };
        textBounds.width = (float)GetTextWidth(textBottom);
        textBounds.height = (float)GuiGetStyle(DEFAULT, TEXT_SIZE);
        textBounds.x = bounds.x + bounds.width/2 - textBounds.width/2;
        textBounds.y = bounds.y + bounds.height + GuiGetStyle(SLIDER, TEXT_PADDING);

        GuiDrawText(textBottom, textBounds, TEXT_ALIGN_LEFT, Fade(GetColor(GuiGetStyle(SLIDER, TEXT + (state*3))), comp->alpha));
    }
    //--------------------------------------------------------------------

    *in_value = value;
    return value;
}

float UI_vert_slider(const UIComp_t* comp, const char *textTop, const char *textBottom, float* value, float minValue, float maxValue)
{
    return GuiVerticalSliderPro(comp, textTop, textBottom, value, minValue, maxValue, GuiGetStyle(SLIDER, SLIDER_WIDTH));
}



// Slider control extended, returns selected value and has text
float UI_slider(const UIComp_t* comp, const char *textLeft, const char *textRight, float *value, float minValue, float maxValue)
{
    return GuiSliderPro(comp, textLeft, textRight, value, minValue, maxValue, GuiGetStyle(SLIDER, SLIDER_WIDTH));
}

void vert_scrollarea_init(VertScrollArea_t* scroll_area, Rectangle display_area, Vector2 canvas_dims)
{
    scroll_area->comp.font = GetFontDefault(); 
    scroll_area->canvas = LoadRenderTexture(canvas_dims.x, canvas_dims.y);
    scroll_area->scroll_pos = canvas_dims.y - display_area.height;
    scroll_area->scroll_bounds = (Vector2){
        0, scroll_area->scroll_pos
    };

    vert_scrollarea_set_item_dims(scroll_area, 12, 3);
    scroll_area->curr_selection = 0;

    scroll_area->display_area = display_area;
    scroll_area->scroll_bar.alpha = 1;
    scroll_area->scroll_bar.bbox = (Rectangle){
        display_area.width + display_area.x, display_area.y,
        0.1f * display_area.width, display_area.height,
    };
    scroll_area->scroll_bar.state = STATE_NORMAL;

    scroll_area->comp.alpha = 1;
    scroll_area->comp.bbox = display_area;
    scroll_area->comp.bbox.width *= 1.1f;
    scroll_area->scroll_bar.state = STATE_NORMAL;
        
}

void vert_scrollarea_set_item_dims(VertScrollArea_t* scroll_area, unsigned int item_height, unsigned int item_padding)
{
    scroll_area->item_height = item_height;
    scroll_area->item_padding = item_padding;
    scroll_area->max_items = (scroll_area->canvas.texture.height - scroll_area->item_padding) / (scroll_area->item_height + scroll_area->item_padding);
}

bool vert_scrollarea_n_items(VertScrollArea_t* scroll_area, unsigned int n_items) {
    if (n_items >= scroll_area->max_items) return false;

    // Due to OpenGL convention where y is -1 downwards,
    // The scroll bar is set to decreases the scroll_pos when going down
    // This value is used as the offset when rendering, which results in correctly
    // give the illusion of 'scrolling down'
    // So, adjust the minimum bound which is the height of the items that are not shown
    scroll_area->n_items = n_items;
    scroll_area->scroll_bounds.x =
        (scroll_area->max_items - n_items)* (scroll_area->item_height + scroll_area->item_padding) + scroll_area->item_padding;
    if (scroll_area->scroll_bounds.x > scroll_area->scroll_bounds.y) {
        scroll_area->scroll_bounds.x = scroll_area->scroll_bounds.y;
    }
    return true;
}

void vert_scrollarea_insert_item(VertScrollArea_t* scroll_area, char* str, unsigned int item_idx)
{
    if (item_idx >= scroll_area->max_items) return;

    DrawTextEx(
        scroll_area->comp.font,
        str,
        (Vector2){0, scroll_area->item_padding + (scroll_area->item_height + scroll_area->item_padding) * item_idx}
        , scroll_area->item_height, 0, BLACK);
}

unsigned int vert_scrollarea_set_pos(VertScrollArea_t* scroll_area, Vector2 pos)
{
    float x = pos.x - scroll_area->display_area.x;
    float y = pos.y - scroll_area->display_area.y;

    if (x >= scroll_area->display_area.width || x < 0) return scroll_area->max_items;
    if (y >= scroll_area->display_area.height || y < 0) return scroll_area->max_items;

    // How much have scroll down
    float canvas_offset = (scroll_area->scroll_bounds.y - scroll_area->scroll_pos);

    scroll_area->curr_selection = (y + canvas_offset - scroll_area->item_padding) / (scroll_area->item_height + scroll_area->item_padding);

    return scroll_area->curr_selection;
}

void vert_scrollarea_refocus(VertScrollArea_t* scroll_area)
{
    float canvas_offset = (scroll_area->scroll_bounds.y - scroll_area->scroll_pos);
    // Reverse from item selection to y in display area
    float selection_y = scroll_area->curr_selection * (scroll_area->item_height + scroll_area->item_padding)  + scroll_area->item_padding - canvas_offset;

    // Auto adjust scroll based on selection
    if (selection_y < 0)
    {
        scroll_area->scroll_pos -= selection_y;
    }
    else if (selection_y + scroll_area->item_height + scroll_area->item_padding > scroll_area->display_area.height)
    {
        scroll_area->scroll_pos -= selection_y + scroll_area->item_height + scroll_area->item_padding - scroll_area->display_area.height;
    }

}


void vert_scrollarea_render(VertScrollArea_t* scroll_area)
{
    BeginScissorMode(scroll_area->comp.bbox.x, scroll_area->comp.bbox.y, scroll_area->comp.bbox.width, scroll_area->comp.bbox.height);
    UI_vert_slider(&scroll_area->scroll_bar,
        NULL,
        NULL,
        &scroll_area->scroll_pos,
        scroll_area->scroll_bounds.x,
        scroll_area->scroll_bounds.y
    );
    Rectangle draw_rec = (Rectangle){
        0, scroll_area->scroll_pos,
        scroll_area->display_area.width,
        scroll_area->display_area.height
    };

    float canvas_offset = (scroll_area->scroll_bounds.y - scroll_area->scroll_pos);
    float selection_y = scroll_area->curr_selection * (scroll_area->item_height + scroll_area->item_padding)  + scroll_area->item_padding - canvas_offset;

    DrawRectangle(
        scroll_area->display_area.x,
        scroll_area->display_area.y + selection_y,
        scroll_area->canvas.texture.width, scroll_area->item_height,
        Fade(BLUE, 0.7)
    );

    Vector2 draw_pos = {scroll_area->display_area.x, scroll_area->display_area.y};
    draw_rec.height *= -1;
    DrawTextureRec(
        scroll_area->canvas.texture,
        draw_rec,
        draw_pos,
        WHITE
    );
    EndScissorMode();
}

void vert_scrollarea_free(VertScrollArea_t* scroll_area)
{
    UnloadRenderTexture(scroll_area->canvas);
}


void init_UI(void)
{
    GuiLoadStyleDefault();
}
