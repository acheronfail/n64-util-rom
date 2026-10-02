#ifndef DRAW_H
#define DRAW_H
#include <stdint.h>
void rect(float x, float y, float w, float h, uint32_t color);
void triangle(float x1,float y1,float x2,float y2,float x3,float y3,uint32_t color);
void label(float x, float y, int style, const char *text);
/* Centre a single button glyph using its visible font bounding box. */
void button_label(float x, float y, int style, char glyph);
#endif
