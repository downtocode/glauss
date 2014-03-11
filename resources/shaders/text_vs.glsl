#version 100

precision highp float;

attribute vec4 coord;
varying vec2 texcoord;

void main(void) {
	gl_Position = vec4(coord.xy, coord.z, 1);
	texcoord = coord.zw;
}
