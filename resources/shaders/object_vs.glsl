#version 130

precision highp float;

uniform mat4 translMat;
uniform mat4 rotationMat;
uniform mat4 scalingMat;
uniform mat4 perspectiveMat;
attribute vec4 pos;
uniform vec4 objcolor;
uniform float radius;

void main() {
	gl_Position = pos*translMat*rotationMat*scalingMat*perspectiveMat;
	gl_PointSize = radius;
}
