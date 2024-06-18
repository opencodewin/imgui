#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>

#define MAX_LIGHTS 5

typedef glm::mat3 mat3;
typedef glm::mat4 mat4; 
typedef glm::vec3 vec3; 
typedef glm::vec4 vec4; 

namespace gl {
class Mesh{
public:
    Mesh(const std::string path);
    ~Mesh() { destroy_buffers(); };

    std::vector <vec3> objectVertices;
    std::vector <vec3> objectNormals;
    std::vector <unsigned int> objectIndices;    

    int render_mode = 0;
    float sx = 1.f, sy = 1.f; 
    float tx = 0.f, ty = 0.f; 
    float fovy = 90.0f;
    float zFar = 99.0f;
    float zNear = 0.1f;
    float amount = 5.f;
    vec3 eye = {0.0, 0.0, 5.0};  
    vec3 up = {0.0, 1.0, 0.0};  
    vec3 center = {0.0, 0.0, 0.0};  

private:
    float view_width = 800, view_height = 600;
    GLuint vertex_array, vertex_buffer, normal_buffer, index_buffer;
    std::string object_path; 
    GLuint vertexshader, fragmentshader, shaderprogram;
    mat4 projection, modelview;
    GLuint projectionPos, modelviewPos;
    GLuint lightcol; 
    GLuint lightpos; 
    GLuint numusedcol; 
    GLuint enablelighting; 
    GLuint ambientcol; 
    GLuint diffusecol; 
    GLuint specularcol; 
    GLuint emissioncol; 
    GLuint shininesscol; 

    void generate_buffers();
    void destroy_buffers();
    void parse_and_bind();
    inline void bind(){glBindVertexArray(vertex_array);}
public:
    void set_view_size(float width, float height);
    void set_angle(float angleX, float angleY);
    void display(float& ambient_slider, float& diffuse_slider, float& specular_slider, float& shininess_slider, bool custom_color, float& light_position, float& light_color);
};
}

namespace gl {
class Transform{
public:
	static void left(float degrees, vec3& eye, vec3& up);
	static void up(float degrees, vec3& eye, vec3& up);
    static mat3 rotate(const float degrees, const vec3& axis);
    static mat4 scale(const float &sx, const float &sy, const float &sz); 
    static mat4 translate(const float &tx, const float &ty, const float &tz);
    static vec3 upvector(const vec3 &up, const vec3 &zvec); 
};
}
