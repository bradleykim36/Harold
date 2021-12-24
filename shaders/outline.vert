uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;
uniform float thickness;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 leftNormal;
layout(location = 3) in vec3 rightNormal;

void main() {
    vec3 p = (modelViewMatrix * vec4(vertex,1)).xyz;
    vec3 e = -p;
    vec3 nl = (normalMatrix * vec4(leftNormal,0)).xyz;
    vec3 nr = (normalMatrix * vec4(rightNormal,0)).xyz;
    vec3 v = vertex;
    if (dot(e,nl) * dot(e,nr) < 0.0) {
       v += thickness*normal;
    }
    gl_Position = projectionMatrix * modelViewMatrix * vec4(v,1);
}
