/* Generated from object_vs.glsl */
"#version 100\012"
"\012"
"precision highp float;\012"
"\012"
"uniform mat4 translMat;\012"
"uniform mat4 rotationMat;\012"
"uniform mat4 scalingMat;\012"
"uniform mat4 perspectiveMat;\012"
"attribute vec4 pos;\012"
"\012"
"void main() {\012"
"\011gl_Position = pos*translMat*rotationMat*scalingMat*perspectiveMat;\012"
"\011gl_PointSize = 1.0;\012"
"}\012"