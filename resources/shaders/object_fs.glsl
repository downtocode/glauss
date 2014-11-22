#version 130

precision highp float;

uniform vec4 objcolor;

void main() {
	gl_FragColor = objcolor;
}
