/* Generated from text_vs.glsl */
"#version 100\012"
"\012"
"precision highp float;\012"
"\012"
"attribute vec4 coord;\012"
"varying vec2 texcoord;\012"
"\012"
"void main(void) {\012"
"\011gl_Position = vec4(coord.xy, coord.z, 1);\012"
"\011texcoord = coord.zw;\012"
"}\012"