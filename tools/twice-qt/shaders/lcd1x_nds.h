#ifndef TWICE_QT_SHADERS_LCD1X_NDS_H
#define TWICE_QT_SHADERS_LCD1X_NDS_H

/*
   lcd1x_nds shader

   A slightly tweaked version of lcd3x:

   - Original lcd3x code written by Gigaherz and released into the public
   domain

   - Original 'nds_color' code written by hunterk, modified by Pokefan531 and
     released into the public domain

   Notes:

   > Omits LCD 'colour seperation' effect

   > Has 'properly' aligned scanlines

   > Includes NDS colour correction

   > Supports any NDS internal resolution setting

   Edited by jdgleaver

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.
*/

/*
 * Edited for use in twice.
 */

constexpr const char *lcd1x_nds_frag_src = R"___(
#version 330 core
out vec4 FragColor;
in vec2 uv;
uniform sampler2D texture0;

#define BRIGHTEN_SCANLINES 32.0
#define BRIGHTEN_LCD 32.0
#define PI 3.141592654
#define TARGET_GAMMA 1.91
const float INV_DISPLAY_GAMMA = 1.0 / 1.91;
#define CC_LUM 0.89
#define CC_R 0.87
#define CC_G 0.645
#define CC_B 0.73
#define CC_RG 0.10
#define CC_RB 0.10
#define CC_GR 0.255
#define CC_GB 0.17
#define CC_BR -0.125
#define CC_BG 0.255

void main()
{
	vec2 texture_size = vec2(256.0, 384.0);
	vec2 angle = 2.0 * PI * ((uv * texture_size) - 0.25);
	float yfactor = (BRIGHTEN_SCANLINES + sin(angle.y)) /
			(BRIGHTEN_SCANLINES + 1.0);
	float xfactor = (BRIGHTEN_LCD + sin(angle.x)) /
			(BRIGHTEN_LCD + 1.0);

	vec3 color = texture(texture0, uv).rgb;
	color.rgb = pow(color.rgb, vec3(TARGET_GAMMA));
	color.rgb = mat3(CC_R,  CC_RG, CC_RB,
			 CC_GR, CC_G,  CC_GB,
			 CC_BR, CC_BG, CC_B) * (color.rgb * CC_LUM);
	color.rgb = clamp(pow(color.rgb, vec3(INV_DISPLAY_GAMMA)), 0.0, 1.0);
	color.rgb = yfactor * xfactor * color.rgb;

	FragColor = vec4(color.rgb, 1.0);
} 
)___";

#endif
