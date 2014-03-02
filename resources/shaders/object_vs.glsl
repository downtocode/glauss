#version 100

precision highp float;

uniform mat4 modelviewProjection;
attribute vec4 pos;

void main() {
	gl_Position = pos*modelviewProjection;
}
