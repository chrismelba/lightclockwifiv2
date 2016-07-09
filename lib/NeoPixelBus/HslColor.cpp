/*--------------------------------------------------------------------
NeoPixel is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
--------------------------------------------------------------------*/

#include "RgbColor.h"
#include "HslColor.h"


HslColor::HslColor(RgbColor color)
{
    //Serial.print("RGB to HSL : ");
    //Serial.print(color.R);
    //Serial.print(", ");
    //Serial.print(color.G);
    //Serial.print(", ");
    //Serial.print(color.B);
    //Serial.print(" => ");

    // convert colors to float between (0.0 - 1.0)
    float r = color.R / 255.0f;
    float g = color.G / 255.0f;
    float b = color.B / 255.0f;

    float max = (r > g && r > b) ? r : (g > b) ? g : b;
    float min = (r < g && r < b) ? r : (g < b) ? g : b;

    float h, s, l;
    l = (max + min) / 2.0f;

    if (max == min) 
    {
        h = s = 0.0f;
    }
    else 
    {
        float d = max - min;
        s = (l > 0.5f) ? d / (2.0f - max - min) : d / (max + min);

        if (r > g && r > b)
        {
            h = (g - b) / d + (g < b ? 6.0f : 0.0f);
        }
        else if (g > b)
        {
            h = (b - r) / d + 2.0f;
        }
        else
        {
            h = (r - g) / d + 4.0f;
        }
        h /= 6.0f;
    }

    //Serial.print(h);
    //Serial.print(", ");
    //Serial.print(s);
    //Serial.print(", ");
    //Serial.print(l);
    //Serial.print(" = ");

#ifdef HSL_FLOAT
    H = h;
    S = s;
    L = l;

#else
    // convert 0.0-1.0 values to 0-255
    H = (uint8_t)(h * 255);
    S = (uint8_t)(s * 255);
    L = (uint8_t)(l * 255);
#endif


    //Serial.print(H);
    //Serial.print(", ");
    //Serial.print(S);
    //Serial.print(", ");
    //Serial.print(L);
    //Serial.println();
}

HslColor HslColor::LinearBlend(HslColor left, HslColor right, float progress)
{   
    float hue;
    float sat;
    float lum;
    float d = right.H-left.H;
    float temp;

    sat = left.S + progress*(right.S - left.S);
    lum = left.L + progress*(right.L - left.L);

    if (left.S==0||right.S==0) // special case, one of the colours is white
    {
        if (left.S==0)
        {
            hue = right.H;
        } else {
            hue = left.H;
        }
    } else {
        if (left.H > right.H)
        {
            temp = right.H;
            right.H = left.H;
            left.H = temp;
            d = -d;
            progress = 1-progress;
        }
        if (d>0.5)
        {
            left.H += 1;
            hue = (left.H+progress*(right.H-left.H));
            if (hue > 1)
            {
                hue -= 1;
            }
        }
        else {
        hue = left.H+progress*d;
        }
    }
    

    return HslColor(hue, sat, lum);
}