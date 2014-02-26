#version 120

varying vec2 texcoord;
uniform vec4 color;
uniform sampler2D tex;

void main() {
		gl_FragColor = vec4(color.x, color.y, color.z, texture2D(tex, texcoord).a);
}
