#include "gui.h"
#include <string.h>

#define RAYGUI_MAX_CONTROLS             16      // Maximum number of standard controls
#define RAYGUI_MAX_PROPS_BASE           16      // Maximum number of standard properties
#define RAYGUI_MAX_PROPS_EXTENDED        8      // Maximum number of extended properties

#define MAX_LINE_BUFFER_SIZE    256
static unsigned int guiStyle[RAYGUI_MAX_CONTROLS*(RAYGUI_MAX_PROPS_BASE + RAYGUI_MAX_PROPS_EXTENDED)] = { 0 };
static bool guiStyleLoaded = false;         // Style loaded flag for lazy style initialization
static Font guiFont = { 0 };                // Gui current font (WARNING: highly coupled to raylib)

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

static int GetTextWidth(const char *text)
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
const char **GetTextLines(const char *text, int *count)
{
    #define RAYGUI_MAX_TEXT_LINES   128

    static const char *lines[RAYGUI_MAX_TEXT_LINES] = { 0 };
    for (int i = 0; i < RAYGUI_MAX_TEXT_LINES; i++) lines[i] = NULL;    // Init NULL pointers to substrings

    int textSize = (int)strlen(text);

    lines[0] = text;
    int len = 0;
    *count = 1;
    int lineSize = 0;   // Stores current line size, not returned

    for (int i = 0, k = 0; (i < textSize) && (*count < RAYGUI_MAX_TEXT_LINES); i++)
    {
        if (text[i] == '\n')
        {
            lineSize = len;
            k++;
            lines[k] = &text[i + 1];     // WARNING: next value is valid?
            len = 0;
            *count += 1;
        }
        else len++;
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

static void GuiDrawText(const char *text, Rectangle bounds, int alignment, Color tint)
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


void UI_button(const UIComp_t *comp, const char *text)
{
    GuiDrawRectangle(comp->bbox, GuiGetStyle(BUTTON, BORDER_WIDTH), Fade(GetColor(GuiGetStyle(BUTTON, BORDER + (comp->state*3))), comp->alpha), Fade(GetColor(GuiGetStyle(BUTTON, BASE + (comp->state*3))), comp->alpha));
    GuiDrawText(text, GetTextBounds(BUTTON, comp->bbox), GuiGetStyle(BUTTON, TEXT_ALIGNMENT), Fade(GetColor(GuiGetStyle(BUTTON, TEXT + (comp->state*3))), comp->alpha));
    
}

void init_UI(void)
{
    GuiLoadStyleDefault();
}
