#version 100

precision lowp float;

uniform vec4 objcolor;

void main() {
	gl_FragColor = objcolor;
}
