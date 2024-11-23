#version 410

// Tessellation control shader: we decide how much we should tessellate a quad geometry.
// All tessellation in OpenGL is done according to an "abstract patch", which is a quad,
// in this case. The outer tessellation level decides how many edges to spawn in each of
// the edges of the quad, and the inner tessellation level say how many inner primitives
// to spawn (in this case, quads). These settings are passed over to the primitive tess.
// unit that will generate more geometry for us. In the case of our shader, tessellation
// level varies inversely proportional to the distance of the chosen vertex and the eye.

layout(vertices = 4) out;

in PipelineData {
    vec3 v_vertexPosition;
    vec3 v_vertexColors;
    vec3 v_vertexNormals;
} tc_in[];

out PipelineData {
    vec3 v_vertexPosition;
    vec3 v_vertexColors;
    vec3 v_vertexNormals;
} tc_out[];

void main() {
    // Just forward the vertex attributes through the GL pipeline.
    tc_out[gl_InvocationID].v_vertexPosition = tc_in[gl_InvocationID].v_vertexPosition;
    tc_out[gl_InvocationID].v_vertexColors = tc_in[gl_InvocationID].v_vertexColors;
    tc_out[gl_InvocationID].v_vertexNormals = tc_in[gl_InvocationID].v_vertexNormals;

    float base_contribution = 4.0;
    float distance_contribution = 12.0;
    float max_contribution = base_contribution + distance_contribution;
    float half_contribution = 0.5*max_contribution; // For outer edges.
//    // We assume that the farthest we'll ever look is 24 units away, and closest is
//    // 0 units away. We then apply a Hermite blending function to the distance, for
//    // a smoother result, this gives the level of detail for the tessellation level.
//    float distance_to_eye = distance(tc_in[gl_InvocationID].position, eye_position);
//    float tessel_lod = 1.0 - smoothstep(0.0, 24.0, distance_to_eye); // 1 --> close.
//    float tessellation_level = distance_contribution*tessel_lod + base_contribution;

    gl_TessLevelInner[0] = base_contribution;
    gl_TessLevelInner[1] = base_contribution;
    gl_TessLevelOuter[0] = base_contribution; 
    gl_TessLevelOuter[1] = base_contribution;
    gl_TessLevelOuter[2] = base_contribution; 
    gl_TessLevelOuter[3] = base_contribution;
}