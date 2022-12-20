uniform vec3 light_pos;
uniform vec3 camera_pos;
uniform vec3 player_pos;
uniform mat4 iViewMatrix;

varying vec2 texCoord; 
varying vec3 vertNormal;
varying vec3 Position;

varying vec3 light_pos_iv;
varying vec3 camera_pos_iv;
varying vec3 player_pos_iv;

void main(void) 
{     
	   
	gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;
	Position = (gl_ModelViewMatrix * gl_Vertex).xyz;
	
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;   
	texCoord = gl_TexCoord[0].xy;

	light_pos_iv = (iViewMatrix*vec4(light_pos,1.0)).xyz;
    camera_pos_iv = vec3(0,0,0);
	player_pos_iv = (iViewMatrix*vec4(player_pos,1.0)).xyz;

	vertNormal = normalize(gl_NormalMatrix*gl_Normal);
}