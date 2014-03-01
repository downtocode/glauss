#version 100

precision highp float;
 
varying vec2 texcoord;
uniform mat4 modelviewProjection;
attribute vec4 pos;

void main() {
		gl_Position = vec4(pos.xy, 0, 1)*modelviewProjection;
		texcoord = pos.zw;
}
