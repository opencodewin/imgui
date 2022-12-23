#pragma once
#include <imvk_mat_shader.h>

#define SHADER_PARAM \
" \n\
layout (push_constant) uniform parameter \n\
{ \n\
    int w; \n\
    int h; \n\
    int cstep; \n\
    int in_format; \n\
    int in_type; \n\
\n\
    int w2; \n\
    int h2; \n\
    int cstep2; \n\
    int in_format2; \n\
    int in_type2; \n\
\n\
    int out_w; \n\
    int out_h; \n\
    int out_cstep; \n\
    int out_format; \n\
    int out_type; \n\
\n\
    float progress; \n\
    float colorSeparation; \n\
} p; \
"

#define SHADER_MAIN \
" \n\
sfpvec4 transition(vec2 uv) \n\
{ \n\
    float y = 0.5 + (uv.y - 0.5) / (1.0 - p.progress); \n\
    if (y < 0.0 || y > 1.0) \n\
    { \n\
        return load_rgba_src2(int(uv.x * (p.w2 - 1)), int((1.f - uv.y) * (p.h2 - 1)), p.w2, p.cstep2, p.in_format2, p.in_type2); \n\
    } \n\
    else \n\
    { \n\
        vec2 fp = vec2(uv.x, y); \n\
        vec2 off = p.progress * vec2(0.0, p.colorSeparation); \n\
        sfpvec4 c = load_rgba(int(fp.x * (p.w - 1)), int((1.f - fp.y) * (p.h - 1)), p.w, p.cstep, p.in_format, p.in_type); \n\
        vec2 fn = clamp(fp - off, vec2(0.f, 0.f), vec2(1.f, 1.f)); \n\
        sfpvec4 cn = load_rgba(int(fn.x * (p.w - 1)), int((1.f - fn.y) * (p.h - 1)), p.w, p.cstep, p.in_format, p.in_type); \n\
        vec2 fc = clamp(fp + off, vec2(0.f, 0.f), vec2(1.f, 1.f)); \n\
        sfpvec4 cp = load_rgba(int(fc.x * (p.w - 1)), int((1.f - fc.y) * (p.h - 1)), p.w, p.cstep, p.in_format, p.in_type); \n\
        return sfpvec4(cn.r, c.g, cp.b, c.a); \n\
    } \n\
} \n\
\n\
void main() \n\
{ \n\
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy); \n\
    if (uv.x >= p.out_w || uv.y >= p.out_h) \n\
        return; \n\
    vec2 point = vec2(float(uv.x) / float(p.out_w - 1), 1.f - float(uv.y) / float(p.out_h - 1)); \n\
    sfpvec4 result = transition(point); \n\
    store_rgba(result, uv.x, uv.y, p.out_w, p.out_cstep, p.out_format, p.out_type); \n\
} \
"

static const char Squeeze_data[] = 
SHADER_HEADER
SHADER_PARAM
SHADER_INPUT_OUTPUT_DATA
R"(
layout (binding =  8) readonly buffer src2_int8       { uint8_t   src2_data_int8[]; };
layout (binding =  9) readonly buffer src2_int16      { uint16_t  src2_data_int16[]; };
layout (binding = 10) readonly buffer src2_float16    { float16_t src2_data_float16[]; };
layout (binding = 11) readonly buffer src2_float32    { float     src2_data_float32[]; };
)"
SHADER_LOAD_RGBA
SHADER_LOAD_RGBA_NAME(src2)
SHADER_STORE_RGBA
SHADER_MAIN
;
