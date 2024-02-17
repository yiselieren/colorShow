/*
 * Print color esc sequence to stdout
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "colorUtils.h"

void normal()
{
    printf("\x1b[0m");
}

void rgb_bg(double rf, double gf, double bf)
{
    uint8_t r = round(rf);
    uint8_t g = round(gf);
    uint8_t b = round(bf);
    printf("\x1b[48;2;%d;%d;%dm", r, g, b);
}

void rgb_fg(double rf, double gf, double bf)
{
    uint8_t r = round(rf);
    uint8_t g = round(gf);
    uint8_t b = round(bf);
    printf("\x1b[38;2;%d;%d;%dm", r, g, b);
}

void hsl_bg(double h, double s, double l)
{
    double r,g,b;
    HSLToRGB(h, s, l, r, g, b);
    rgb_bg(r, g, b);
}
void hsl_fg(double h, double s, double l)
{
    double r,g,b;
    HSLToRGB(h, s, l, r, g, b);
    rgb_fg(r, g, b);
}
