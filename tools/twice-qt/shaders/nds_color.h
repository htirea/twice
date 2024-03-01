#ifndef TWICE_QT_SHADERS_NDS_COLOR_H
#define TWICE_QT_SHADERS_NDS_COLOR_H

/*
   Shader Modified: Pokefan531
   Color Mangler
   Author: hunterk
   License: Public domain
*/

/*
 * Edited for use in twice.
 */

constexpr const char *nds_color_frag_src = R"___(
#version 330 core
out vec4 FragColor;
in vec2 uv;
uniform sampler2D texture0;

#define target_gamma 2.0
#define display_gamma 2.0

void main()
{
	float lum = 1.0;
	vec4 screen = pow(texture(texture0, uv), vec4(target_gamma)).rgba;
	screen = clamp(screen * lum, 0.0, 1.0);
	mat4 NDS_sRGB = mat4(
		0.705, 0.09, 0.1075, 0.0,  //red channel
		0.235, 0.585, 0.1725, 0.0, //green channel
		-0.075, 0.24, 0.72, 0.0,   //blue channel
		0.0,  0.0,  0.0,  1.0      //alpha channel
	);
	screen = NDS_sRGB * screen;
	FragColor = pow(screen, vec4(1.0 / display_gamma));
}
)___";

#endif
