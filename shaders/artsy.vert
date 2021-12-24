uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;

out vec3 Position;
out vec3 Normal;

void main() {
    Position = (modelViewMatrix * vec4(vertex,1)).xyz;
    Normal = normalize((normalMatrix * vec4(normal,0)).xyz);
    gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex,1);
}
