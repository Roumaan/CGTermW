uniform sampler2D texture;
uniform sampler2D normalMap;
uniform bool enableTexture;
uniform bool enableNormalMap;
uniform bool enableLight;
uniform bool enableFog;

uniform float Ia;
uniform float Id;
uniform float Is;
uniform vec3 ma;
uniform vec3 md;
uniform vec4 ms;

varying vec3 vertNormal;
varying vec3 Position;
varying vec2 texCoord;

varying vec3 light_pos_iv;
varying vec3 camera_pos_iv;
varying vec3 player_pos_iv;

float getFogFactor(float d)
{
    const float FogMax = 30.0;
    const float FogMin = 10.0;

    if (d>=FogMax) return 1.0;
    if (d<=FogMin) return 0.0;

    return 1.0 - (FogMax - d) / (FogMax - FogMin);
}

void main(void)
{
	gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);

	if (enableLight)
	{
		vec3 normal;
		if (enableNormalMap)
			normal = normalize(gl_NormalMatrix*(texture2D(normalMap, texCoord).rgb*2.0-1.0));
		else
			normal = vertNormal;

		vec3 color_amb = Ia*ma;
	
		vec3 light_vector = normalize(light_pos_iv - Position);
		vec3 color_dif = Id*md*dot(light_vector, normal);
	
		vec3 view_vector = normalize(camera_pos_iv - Position);
		vec3 r = reflect(light_vector, normal);
		vec3 color_spec = Is*ms.xyz*pow(max(0.0,dot(-r,view_vector)),ms.w);
	
		vec3 light = color_amb + color_dif + color_spec;

		gl_FragColor *= vec4(light, 1.0);
	}
	
	if (enableTexture)
		gl_FragColor *= vec4(texture2D(texture, texCoord).rgb, 1.0);
	
	if (!enableLight && !enableTexture)
		gl_FragColor = vec4(Id*ma, 1.0);

	if (enableFog)
	{
		float d = distance(player_pos_iv, Position);
		float alpha = getFogFactor(d);
		gl_FragColor = mix(gl_FragColor, vec4(0.05,0.05,0.05,1.0), alpha);
	}
}