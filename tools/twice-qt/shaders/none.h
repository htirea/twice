#ifndef TWICE_QT_SHADERS_NONE_H
#define TWICE_QT_SHADERS_NONE_H

constexpr const char *vtx_shader_src = R"___(
#version 330 core
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv_in;
uniform mat4 proj_mtx;
out vec2 uv;

void main()
{
	gl_Position = proj_mtx * vec4(pos, 1.0);
	uv = uv_in;
}
)___";

constexpr const char *fragment_shader_src = R"___(
#version 330 core
out vec4 FragColor;
in vec2 uv;
uniform sampler2D texture0;

void main()
{
	FragColor = texture(texture0, uv);
}
)___";

#endif
