#version 410

//uniform sampler2D tex;
uniform samplerCube skybox;
uniform vec3 cameraPos;

in PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} fs_in;

out vec4 color;

// Entry point of program
void main()
{
    // Refractive index of water
    float ratio = 1.00 / 1.33;
    vec3 I = normalize(fs_in.v_vertexPosition - cameraPos);
    //vec3 R = reflect(I, normalize(fs_in.v_vertexNormals));
    vec3 R = refract(I, normalize(fs_in.v_vertexNormals), ratio);
    //vec3 diffuseColor =  0.5 * texture(tex, fs_in.v_texCoords).rgb;
    // if (R.y <= 0.0f) {
    //     R.y = -R.y;
    // }
	color = vec4(texture(skybox, R).rgb, 1.0f);
}
