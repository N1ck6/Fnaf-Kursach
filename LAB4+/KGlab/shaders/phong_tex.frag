uniform sampler2D tex;
uniform vec3 light_pos_v;
uniform vec3 Ia;
uniform vec3 Id;
uniform vec3 Is;
uniform vec3 ma;
uniform vec3 md;
uniform vec3 ms;
uniform float sh;

varying vec3 Position;
varying vec3 Normal;
varying vec2 texCoord;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(light_pos_v - Position);
    vec3 V = normalize(-Position);
    vec3 R = reflect(-L, N);

    vec3 color_amb = Ia * ma;
    vec3 color_dif = Id * md * max(dot(L, N), 0.0);
    vec3 color_spec = Is * ms * pow(max(dot(R, V), 0.0), sh);

    vec4 texColor = texture2D(tex, texCoord);
    vec3 finalColor = texColor.rgb * (color_amb + color_dif) + color_spec;

    gl_FragColor = vec4(finalColor, texColor.a);
}
