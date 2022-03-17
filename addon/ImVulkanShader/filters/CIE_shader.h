#pragma once
#include <imvk_mat_shader.h>

#define SHADER_PARAM \
" \n\
layout (push_constant) uniform parameter \n\
{ \n\
    int w; \n\
    int h; \n\
    int cstep; \n\
    int format; \n\
    int type; \n\
\n\
    int outw; \n\
    int outh; \n\
    int outcstep; \n\
\n\
    int cie; \n\
    float intensity; \n\
\n\
} p; \
"

#define SHADER_CIE \
" \n\
#define XYY     0 \n\
#define UCS     1 \n\
#define LUV     2 \n\
sfpmat3 matrix_mat_r2x = { \n\
    {convert_matrix_r2x[0], convert_matrix_r2x[3], convert_matrix_r2x[6]}, \n\
    {convert_matrix_r2x[1], convert_matrix_r2x[4], convert_matrix_r2x[7]}, \n\
    {convert_matrix_r2x[2], convert_matrix_r2x[5], convert_matrix_r2x[8]}, \n\
}; \
sfpvec3 rgb_to_xyz(sfpvec4 rgba) \n\
{ \n\
    sfpvec3 rgb = rgba.rgb; \n\
    sfpvec3 xyz = rgb * matrix_mat_r2x; \n\
    sfp sum = xyz.x + xyz.y + xyz.z; \n\
    if (sum == 0) \n\
        sum = sfp(1.f); \n\
    xyz.x = xyz.x / sum; \n\
    xyz.y = xyz.y / sum; \n\
    return xyz; \n\
} \n\
sfpvec2 xy_to_upvp(sfpvec2 xy) \n\
{ \n\
    sfpvec2 upvp = {sfp(0.f), sfp(0.f)}; \n\
    upvp.x = 4*xy.x / (- 2*xy.x + 12*xy.y + 3); \n\
    upvp.y = 9*xy.y / (- 2*xy.x + 12*xy.y + 3); \n\
    return upvp; \n\
} \n\
sfpvec2 xy_to_uv(sfpvec2 xy) \n\
{ \n\
    sfpvec2 uv = {sfp(0.f), sfp(0.f)}; \n\
    uv.x = 4*xy.x / (- 2*xy.x + 12*xy.y + 3); \n\
    uv.y = 6*xy.y / (- 2*xy.x + 12*xy.y + 3); \n\
    return uv; \n\
} \n\
void cie(int x, int y) \n\
{ \n\
    ivec2 ixy = {0, 0}; \n\
    sfpvec2 fxy = {sfp(0.f), sfp(0.f)}; \n\
    sfpvec4 rgba = load_rgba(x, y, p.w, p.cstep, p.format, p.type); \n\
    sfpvec3 xyz = rgb_to_xyz(rgba); \n\
    if (p.cie == LUV) \n\
    { \n\
        fxy = xy_to_upvp(sfpvec2(xyz.x, xyz.y)); \n\
    }\n\
    else if (p.cie == UCS) \n\
    { \n\
        fxy = xy_to_uv(sfpvec2(xyz.x, xyz.y)); \n\
    } \n\
    else \n\
    { \n\
        fxy = sfpvec2(xyz.x, xyz.y); \n\
    } \n\
    ixy.x = int((p.outw - 1) * fxy.x); \n\
    ixy.y = (p.outh - 1) - int(((p.outh - 1) * fxy.y)); \n\
    if (ixy.x >= 0 && ixy.x < p.outw && ixy.y >= 0 && ixy.y < p.outh) \n\
    { \n\
        int offset = ixy.y * p.outw + ixy.x; \n\
        atomicAdd(alpha_blob_data[offset], int(p.intensity * 255)); \n\
    } \n\
} \
"

#define SHADER_CIE_MAIN \
" \n\
void main() \n\
{ \n\
    int gx = int(gl_GlobalInvocationID.x); \n\
    int gy = int(gl_GlobalInvocationID.y); \n\
    if (gx >= p.w || gy >= p.h) \n\
        return; \n\
\n\
    cie(gx, gy); \n\
} \
"

static const char CIE_data[] = 
SHADER_HEADER
SHADER_PARAM
SHADER_SRC_DATA
R"(
layout (binding = 4) buffer alpha_blob { int alpha_blob_data[]; };
layout (binding = 5) readonly buffer matrix_r2x { float convert_matrix_r2x[]; };
)"
SHADER_LOAD_RGBA
SHADER_CIE
SHADER_CIE_MAIN
;

// merge shader
#define SHADER_MERGE_PARAM \
" \n\
layout (push_constant) uniform parameter \n\
{ \n\
    int w; \n\
    int h; \n\
    int cstep; \n\
    int in_format; \n\
    int in_type; \n\
    \n\
    int out_w; \n\
    int out_h; \n\
    int out_cstep; \n\
    int out_format; \n\
    int out_type; \n\
    \n\
    int show_color; \n\
} p; \
"

#define SHADER_MERGE \
" \n\
void merge(int x, int y) \n\
{ \n\
    sfpvec4 rgba = load_rgba(x, y, p.w, p.cstep, p.in_format, p.in_type); \n\
    int offset = y * p.w + x; \n\
    int alpha = alpha_blob_data[offset]; \n\
    if (p.show_color == 1) \n\
    { \n\
        if (alpha > 0) \n\
        { \n\
            rgba *= sfp(1.f - clamp(alpha / 255.f, 0.f, 1.f)); \n\
            rgba.a = sfp(1.f); \n\
            store_rgba(rgba, x, y, p.out_w, p.out_cstep, p.out_format, p.out_type); \n\
        } else { \n\
            rgba.a = sfp(1.f); \n\
            store_rgba(rgba, x, y, p.out_w, p.out_cstep, p.out_format, p.out_type); \n\
        } \n\
    } \n\
    else \n\
    { \n\
        if (rgba.a <= sfp(0.51f) && rgba.a >= sfp(0.49f)) \n\
        { \n\
            rgba.a = sfp(1.f); \n\
        } \n\
        else if (alpha == 0) \n\
        { \n\
            rgba.r = sfp(0.f); \n\
            rgba.g = sfp(0.f); \n\
            rgba.b = sfp(0.f); \n\
            rgba.a = sfp(1.f); \n\
        } \n\
        else \n\
            rgba.a = sfp(1.f); \n\
        store_rgba(rgba, x, y, p.out_w, p.out_cstep, p.out_format, p.out_type); \n\
    } \n\
} \
"

#define SHADER_MERGE_MAIN \
" \n\
void main() \n\
{ \n\
    int gx = int(gl_GlobalInvocationID.x); \n\
    int gy = int(gl_GlobalInvocationID.y); \n\
    if (gx >= p.out_w || gy >= p.out_h) \n\
        return; \n\
\n\
    merge(gx, gy); \n\
} \
"

static const char CIE_merge_data[] = 
SHADER_HEADER
SHADER_MERGE_PARAM
SHADER_INPUT_OUTPUT_DATA
R"(
layout (binding = 8) readonly buffer alpha_blob { int alpha_blob_data[]; };
)"
SHADER_LOAD_RGBA
SHADER_STORE_RGBA
SHADER_MERGE
SHADER_MERGE_MAIN
;

// set shader
#define SHADER_SET_PARAM \
" \n\
layout (push_constant) uniform parameter \n\
{ \n\
    int w; \n\
    int h; \n\
    int cstep; \n\
    \n\
    int format; \n\
    int show_color; \n\
} p; \
"

#define SHADER_SET_MAIN \
" \n\
void main() \n\
{ \n\
    int gx = int(gl_GlobalInvocationID.x); \n\
    int gy = int(gl_GlobalInvocationID.y); \n\
    if (gx >= p.w || gy >= p.h) \n\
        return; \n\
    int offset = gy * p.w + gx; \n\
    alpha_blob_data[offset] = 0; \n\
} \
"

static const char CIE_set_data[] = 
SHADER_HEADER
R"(
layout (binding = 0) writeonly buffer alpha_blob { int alpha_blob_data[]; };
)"
SHADER_SET_PARAM
SHADER_SET_MAIN
;
