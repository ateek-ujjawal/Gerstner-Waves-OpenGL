#version 410

// Tessellation evaluation shader
// After tessellation control shader is done and then passed new vertices onto the TES,
// we displace the vertices by the gerstner wave function(also including the normals).
// I also bilinearly interpolated the position, normals and texture coordinates according to the
// weights given by gl_TessCoord, which is the tessellated coordinate.

// We use quads with a fractional even spacing tessellation to smoothen the edges
layout(quads, fractional_even_spacing) in;

uniform mat4 u_ViewMatrix;
uniform mat4 u_Projection; // We'll use a perspective projection
uniform mat4 u_ModelMatrix;

in VertexData {
    vec3 v_vertexPosition;
    vec3 v_vertexNormals;
} te_in[];

out VertexData {
    vec3 v_vertexPosition;
    vec3 v_vertexNormals;
} te_out;

uniform uint num_of_waves = 0;
uniform float time;
uniform struct GerstnerWave {
    vec2 direction;
    float amplitude;
    float steepness;
    float frequency;
    float speed;
} gerstner_waves[5];

vec3 gerstner_wave_normal(vec3 position, float time) {
    vec3 wave_normal = vec3(0.0, 1.0, 0.0);

    for (uint i = 0; i < num_of_waves; ++i) {
        float psi = dot(position.xz, gerstner_waves[i].direction) * gerstner_waves[i].frequency +
                    time * gerstner_waves[i].speed,
              alpha = gerstner_waves[i].amplitude * gerstner_waves[i].frequency * sin(psi);

        wave_normal.y -= gerstner_waves[i].steepness * alpha;

        float x = gerstner_waves[i].direction.x,
              y = gerstner_waves[i].direction.y,
              omega = gerstner_waves[i].amplitude * gerstner_waves[i].frequency * cos(psi);

        wave_normal.x -= x * omega;
        wave_normal.z -= y * omega;
    } 
    
    return wave_normal;
}

vec3 gerstner_wave_position(vec2 position, float time) {
    vec3 wave_position = vec3(position.x, 0, position.y);

    for (uint i = 0; i < num_of_waves; ++i) {
        float theta = dot(position, gerstner_waves[i].direction) * gerstner_waves[i].frequency
                      + time * gerstner_waves[i].speed,
              height = gerstner_waves[i].amplitude * sin(theta);

        wave_position.y += height;

        float maximum_width = gerstner_waves[i].steepness *
                              gerstner_waves[i].amplitude,
              width = maximum_width * cos(theta),
              x = gerstner_waves[i].direction.x,
              y = gerstner_waves[i].direction.y;

        wave_position.x += x * width;
        wave_position.z += y * width;
    } 
    
    return wave_position;
}

vec3 gerstner_wave(vec2 position, float time, inout vec3 normal) {
    vec3 wave_position = gerstner_wave_position(position, time);
    normal = gerstner_wave_normal(wave_position, time);
    return wave_position;
}

void main() {
    // Interpolate positions, normals using gl_TessCoord as the weights
    vec3 x_up_position_mix = mix(te_in[0].v_vertexPosition, te_in[3].v_vertexPosition, gl_TessCoord.x);
    vec3 x_down_position_mix = mix(te_in[1].v_vertexPosition, te_in[2].v_vertexPosition, gl_TessCoord.x);
    te_out.v_vertexPosition = mix(x_down_position_mix, x_up_position_mix, gl_TessCoord.y);

    vec3 x_up_normal_mix = mix(te_in[0].v_vertexNormals, te_in[3].v_vertexNormals, gl_TessCoord.x);
    vec3 x_down_normal_mix = mix(te_in[1].v_vertexNormals, te_in[2].v_vertexNormals, gl_TessCoord.x);
    te_out.v_vertexNormals = mix(x_down_normal_mix, x_up_normal_mix, gl_TessCoord.y);

    // Displace the tessellated geometry in the direction of the normal by us-
    // ing a sum of Gerstner waves.
    te_out.v_vertexPosition = gerstner_wave(te_out.v_vertexPosition.xz, time,  te_out.v_vertexNormals);
    vec4 world_position = vec4(te_out.v_vertexPosition, 1);

    gl_Position = u_Projection * u_ViewMatrix * world_position;
}