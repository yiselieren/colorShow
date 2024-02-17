/*
 * Color utils
 *
 * HSL to RGB conversion
 *
 */
#ifndef COLORUTILS_H
#define COLORUTILS_H

// HSL <-> RGB conversion
void HSLToRGB(double H, double S, double L, double& R, double& G, double& B);
void RGBToHSL(double R, double G, double B, double& H, double& S, double& L);

#endif
