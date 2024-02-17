/*
 * Print color esc sequence to stdout
 *
 */
#ifndef COLORPRINT_H
#define COLORPRINT_H

void normal();
void rgb_bg(double red, double green, double blue);
void rgb_fg(double red, double green, double blue);
void hsl_bg(double hue, double saturation, double luminosity);
void hsl_fg(double hue, double saturation, double luminosity);

#endif
