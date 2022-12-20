uniform vec3 gradColor;
uniform bool sinGrad;
varying vec2 texCoord; 

float getFogFactor(float d)
{
    const float FogMax = 20.0;
    const float FogMin = 10.0;

    if (d>=FogMax) return 1.0;
    if (d<=FogMin) return 0.0;

    return 1.0 - (FogMax - d) / (FogMax - FogMin);
}

void main()
{
    float d;
    if (sinGrad)
        d = sin(texCoord.x*2*radians(180.0))/8.0+0.85 - texCoord.y;
    else
        d = 1-texCoord.y;
    gl_FragColor = vec4(gradColor, d);

}