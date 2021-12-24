uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

layout(location = 0) in vec3 vertex;

void main() {
    gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex,1);
}
