#version 100

precision highp float;

varying vec2 texcoord;
uniform vec4 colors;
uniform sampler2D tex;

void main() {
		gl_FragColor = vec4(colors.x, colors.y, colors.z, texture2D(tex, texcoord).a);
}
