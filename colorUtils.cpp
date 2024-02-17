/*
 * Color utils
 *
 * HSL to RGB conversion
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

static double HueToRGB(double v1, double v2, double vH)
{
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

void HSLToRGB(double H, double S, double L, double& R, double& G, double& B)
{
	if (S == 0)
		R = G = B = (unsigned char)(L * 255);
	else
	{
		double v1, v2;
		double hue = H / 360.0;

		v2 = (L < 0.5) ? (L * (1 + S)) : ((L + S) - (L * S));
		v1 = 2 * L - v2;

		R = 255.0 * HueToRGB(v1, v2, hue + (1.0 / 3));
		G = 255.0 * HueToRGB(v1, v2, hue);
		B = 255.0 * HueToRGB(v1, v2, hue - (1.0 / 3));
	}
}

void RGBToHSL(double R, double G, double B, double& H, double& S, double& L)
{
    R /= 255.0;
    G /= 255.0;
    B /= 255.0;

    double max = std::max(std::max(R,G),B);
    double min = std::min(std::min(R,G),B);

    H = S = L = (max + min) / 2;

    if (max == min) {
        H = S = 0;
    } else {
        double d = max - min;
        S = (L > 0.5) ? d / (2 - max - min) : d / (max + min);

        if (max == R) {
            H = (G - B) / d + (G < B ? 6 : 0);
        }
        else if (max == G) {
            H = (B - R) / d + 2;
        }
        else if (max == B) {
            H = (R - G) / d + 4;
        }

        H *= 60.0;
    }
}
