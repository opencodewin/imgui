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
    float smoothness; \n\
    int size; \n\
    float direction_x; \n\
    float direction_y; \n\
} p; \
"

#define SHADER_MAIN \
" \n\
ivec2 squares = ivec2(p.size, p.size); \n\
vec2 direction = vec2(p.direction_x, p.direction_y); \n\
const vec2 center = vec2(0.5, 0.5); \n\
sfpvec4 transition(vec2 point) \n\
{ \n\
    vec2 v = normalize(direction); \n\
    v /= abs(v.x) + abs(v.y); \n\
    float d = v.x * center.x + v.y * center.y; \n\
    float offset = p.smoothness; \n\
    float pr = smoothstep(-offset, 0.0, v.x * point.x + v.y * point.y - (d - 0.5 + p.progress * (1. + offset))); \n\
    vec2 squarep = fract(point * vec2(squares)); \n\
    vec2 squaremin = vec2(pr / 2.0); \n\
    vec2 squaremax = vec2(1.0 - pr / 2.0); \n\
    float a = (1.0 - step(p.progress, 0.0)) * step(squaremin.x, squarep.x) * step(squaremin.y, squarep.y) * step(squarep.x, squaremax.x) * step(squarep.y, squaremax.y); \n\
    sfpvec4 rgba_to = load_rgba_src2(int(point.x * (p.w2 - 1)), int((1.f - point.y) * (p.h2 - 1)), p.w2, p.cstep2, p.in_format2, p.in_type2); \n\
    sfpvec4 rgba_from = load_rgba(int(point.x * (p.w - 1)), int((1.f - point.y) * (p.h - 1)), p.w, p.cstep, p.in_format, p.in_type); \n\
    return mix(rgba_from, rgba_to, sfp(a)); \n\
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

static const char SquaresWire_data[] = 
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
