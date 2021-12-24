in vec3 Position;
in vec3 Normal;

out vec4 color;

uniform vec3 lightPosition;
uniform vec4 Ia, Id, Is;

uniform vec4 ka, kd, ks;
uniform float s;

uniform sampler2D diffuseRamp;
uniform sampler2D specularRamp;


void main() {
    vec3 l = normalize(lightPosition.xyz - Position);
    vec3 n = normalize(Normal);
    vec3 e = normalize(-Position);
    vec3 h = normalize(e + l);
    float diffuse = (dot(l,n) + 1)/2;
    float specular = pow(max(dot(n,h),0), s);
    color = ka*Ia
          + kd*Id*texture(diffuseRamp, vec2(diffuse,0))
          + ks*Is*texture(specularRamp, vec2(specular,0));
    color.a = 1;
}
