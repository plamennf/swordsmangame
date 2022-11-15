cbuffer Global_Parameters : register(b0) {
    row_major float4x4 transform;
    row_major float4x4 proj_matrix;
    row_major float4x4 view_matrix;
}
