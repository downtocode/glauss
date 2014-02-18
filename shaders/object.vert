uniform mat4 modelviewProjection;
attribute vec4 pos;

void main() {
		gl_Position = modelviewProjection * pos;
}
