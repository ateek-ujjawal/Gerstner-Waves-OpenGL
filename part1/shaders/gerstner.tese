#version 410

// Tessellation evaluation shader: after tessellation has been done, new vertices have
// been generated. The tess. evaluation shaders purpose is to decide on where to place
// these new vertices. In our case, all of the vertex properties (position and normal)
// are interpolated bilinearly (for a quad) to find the new vertex properties. And, we
// also displace the new vertex according to the Gerstner wave, to produce a realistic
// ocean surface animation (varying by using the vertices X-Z coordinate and the time).
// We also do world --> projection transformation here, since we couldn't do it before.

layout(quads, fractional_even_spacing) in;

uniform mat4 u_ViewMatrix;
uniform mat4 u_Projection; // We'll use a perspective projection

in PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} te_in[];

out PipelineData {
    vec3 v_vertexPosition;
    vec2 v_texCoords;
    vec3 v_vertexNormals;
} te_out;

uniform uint gerstner_waves_length = 0;
uniform float time;
uniform struct GerstnerWave {
    vec2 direction;
    float amplitude;
    float steepness;
    float frequency;
    float speed;
} gerstner_waves[1];

vec3 gerstner_wave_normal(vec3 position, float time) {
    vec3 wave_normal = vec3(0.0, 1.0, 0.0);
    for (uint i = 0; i < gerstner_waves_length; ++i) {
        float proj = dot(position.xz, gerstner_waves[i].direction),
              phase = time * gerstner_waves[i].speed,
              psi = proj * gerstner_waves[i].frequency + phase,
              Af = gerstner_waves[i].amplitude *
                   gerstner_waves[i].frequency,
              alpha = Af * sin(psi);

        wave_normal.y -= gerstner_waves[i].steepness * alpha;

        float x = gerstner_waves[i].direction.x,
              y = gerstner_waves[i].direction.y,
              omega = Af * cos(psi);

        wave_normal.x -= x * omega;
        wave_normal.z -= y * omega;
    } return wave_normal;
}

vec3 gerstner_wave_position(vec2 position, float time) {
    vec3 wave_position = vec3(position.x, 0, position.y);
    for (uint i = 0; i < gerstner_waves_length; ++i) {
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
    } return wave_position;
}

vec3 gerstner_wave(vec2 position, float time, inout vec3 normal) {
    vec3 wave_position = gerstner_wave_position(position, time);
    //normal = gerstner_wave_normal(wave_position, time);
    return wave_position; // Accumulated Gerstner Wave.
}

void main() {
    // Below we just interpolate the position, normal, and texture coordinates by
    // using the weights of the new vertices produced from the tessellation unit.
    vec3 x_up_position_mix = mix(te_in[0].v_vertexPosition, te_in[3].v_vertexPosition, gl_TessCoord.x);
    vec3 x_down_position_mix = mix(te_in[1].v_vertexPosition, te_in[2].v_vertexPosition, gl_TessCoord.x);
    te_out.v_vertexPosition = mix(x_down_position_mix, x_up_position_mix, gl_TessCoord.y);

    vec2 x_up_texture_mix = mix(te_in[0].v_texCoords, te_in[3].v_texCoords, gl_TessCoord.x);
    vec2 x_down_texture_mix = mix(te_in[1].v_texCoords, te_in[2].v_texCoords, gl_TessCoord.x);
    te_out.v_texCoords = mix(x_down_texture_mix, x_up_texture_mix, gl_TessCoord.y);

    vec3 x_up_normal_mix = mix(te_in[0].v_vertexNormals, te_in[3].v_vertexNormals, gl_TessCoord.x);
    vec3 x_down_normal_mix = mix(te_in[1].v_vertexNormals, te_in[2].v_vertexNormals, gl_TessCoord.x);
    te_out.v_vertexNormals = mix(x_down_normal_mix, x_up_normal_mix, gl_TessCoord.y);
    //float distance_from_center = distance(look_at_point.xz,te_out.position.xz)/24;

    // Displace the tessellated geometry in the direction of the normal by us-
    // ing a sum of Gerstner waves. A effect of this wave is that it will move
    // vertices closer to the waves' crest (the tallest peak) depending on the
    // steepness parameter. The height of the wave is done by the sum of sines
    // method. The parameters can be changed in the shader or by the uniforms.
    te_out.v_vertexPosition = gerstner_wave(te_out.v_vertexPosition.xz, time,  te_out.v_vertexNormals);

    // vec3 normal; // Below we add a bit more detail by adding 3D Simplex Noise.
    // te_out.position.y += 0.40*snoise3d(0.2*te_out.position+0.40*time, normal) * perturb;
    // te_out.position.y += 0.20*snoise3d(0.4*te_out.position+0.20*time, normal) * perturb;
    // te_out.position.y += 0.10*snoise3d(0.8*te_out.position+0.10*time, normal) * perturb;
    // te_out.position.y += 0.05*snoise3d(1.6*te_out.position+0.05*time, normal) * perturb;
    // te_out.position.y += 0.02*snoise3d(3.2*te_out.position+0.02*time, normal) * perturb;

    gl_Position = u_Projection * u_ViewMatrix * vec4(te_out.v_vertexPosition, 1.0);
}