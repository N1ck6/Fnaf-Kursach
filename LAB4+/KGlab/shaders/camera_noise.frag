uniform sampler2D texStatic;
uniform sampler2D texCamera;
uniform float u_time;

varying vec2 texCoord;

void main()
{
    float factor = clamp(
        pow(cos(u_time + 0.8), 50.0)
        + 0.1 * cos(u_time * 30.0 + 1.0)
        + 0.05 * cos(u_time * 50.0 + 2.0),
        0.0, 1.0);

    vec3 staticColor = texture2D(texStatic, texCoord).rgb;
    vec3 noiseColor = texture2D(texCamera, texCoord).rgb;

    gl_FragColor = vec4(staticColor * (1.0 - factor) + noiseColor * factor, 1.0);
}
