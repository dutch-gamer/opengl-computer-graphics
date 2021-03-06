#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <string>
#include <math.h>
#include <cstdlib>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "objloader.h"
#include "texture.h"


#include "glsl.h"

using namespace std;
using namespace glm;


//--------------------------------------------------------------------------------
// Consts
//--------------------------------------------------------------------------------

const int WIDTH = 800, HEIGHT = 600;

const char* Pfragshader_name = "Pfragmentshader.frag";
const char* Pvertexshader_name = "Pvertexshader.vert";

const char* Ofragshader_name = "Ofragmentshader.frag";
const char* Overtexshader_name = "Overtexshader.vert";


vec3 light_position = vec3(4, 4, 4),
    ambient_color = vec3(0.25, 0.25, .25),
    diffuse_color = vec3(.75, .75, .75);

unsigned const int DELTA_TIME = 10;


//--------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------

// ID's
GLuint P_program_id, O_program_id;
//GLuint vao;

// Matrices
mat4 view, projection;


//--------------------------------------------------------------------------------
// Mesh variables
//--------------------------------------------------------------------------------

//------------------------------------------------------------
//
//           7----------6
//          /|         /|
//         / |        / |
//        /  4-------/--5               y
//       /  /       /  /                |
//      3----------2  /                 ----x
//      | /        | /                 /
//      |/         |/                  z
//      0----------1
//------------------------------------------------------------

struct primitive_object
{
    GLuint vao;

    vector<GLfloat> vertices;
    vector<GLfloat> normals;
    vector<GLfloat> colors;
    vector<GLushort> elements;
    mat4 model;
    mat4 mv;
    GLuint uniform_mvp;
    GLchar type;
    primitive_object() {
        vector<GLfloat> fl;
        vector<GLushort> sh;
        vector<vec3> v3;
        vertices = fl;
        normals = fl;
        colors = fl;
        elements = sh;
        model = mat4();
        mv = mat4();
        uniform_mvp = NULL;
        type = GL_TRIANGLES;
    }
    primitive_object(vector<GLfloat> v,
        vector<GLfloat> n,
        vector<GLfloat> c,
        vector<GLushort> e,
        GLchar t) {
        vertices = v;
        normals = n;
        colors = c;
        elements = e;
        model = mat4();
        uniform_mvp = NULL;
        type = t;
    }
};

vector<primitive_object> primitive_objects;

struct textured_object
{
    GLuint vao;

    vector<vec3> vertices;
    vector<vec3> normals;
    vector<vec2> uvs;
    mat4 model;
    mat4 mv;
    GLuint uniform_mv;
    GLuint texture_id;
    textured_object() {
        vector<vec3> v;
        vector<vec3> n;
        vector<vec2> u;
        vertices = v;
        normals = n;
        uvs = u;
        model = mat4();
        texture_id = NULL;
    }
    textured_object(vector<vec3> v,
        vector<vec2> u,
        vector<vec3> n,
        GLuint id) {
        vertices = v;
        normals = n;
        uvs = u;
        texture_id = id;
        model = mat4();
    }
};

vector<textured_object> textured_objects;

vec3 playerPosition = vec3(2.0, 2.0, -10.0);
vec3 lookVector = vec3();
vec2 th_ph = vec2(0, 0);

//--------------------------------------------------------------------------------
// Keyboard handling
//--------------------------------------------------------------------------------

void keyboardHandler(unsigned char key, int a, int b)
{
    if (key == 27)
        glutExit();

    //movement
    if (key == 119)    //W
        playerPosition = playerPosition + vec3(sin(th_ph.x), 0, cos(th_ph.x));
    if (key == 97)    //A
        playerPosition = playerPosition + vec3(cos(th_ph.x), 0, -sin(th_ph.x));
    if (key == 115)    //S
        playerPosition = playerPosition + vec3(-sin(th_ph.x), 0, -cos(th_ph.x));
    if (key == 100)    //D
        playerPosition = playerPosition + vec3(-cos(th_ph.x), 0, sin(th_ph.x));
    if (key == 113)    //Q
        playerPosition = playerPosition + vec3(0, -1, 0);
    if (key == 101)    //E
        playerPosition = playerPosition + vec3(0, 1, 0);

    //looking
    if (key == 105)    //I
        th_ph.y += radians(2.0f);
    if (key == 106)    //J
        th_ph.x += radians(2.0f);
    if (key == 107)    //K
        th_ph.y -= radians(2.0f);
    if (key == 108)    //L
        th_ph.x -= radians(2.0f);

    //if (key == 109) // M
        //useMouse = true;

    //overflow protection
    th_ph.x -= radians(360.0f) * (th_ph.x >= radians(360.0f));
    th_ph.x += radians(360.0f) * (th_ph.x < 0);
    if (th_ph.y >= radians(90.0f))
        th_ph.y = radians(89.0f);
    else if (th_ph.y <= -radians(90.0f))
        th_ph.y = -radians(89.0f);
    //convert theta and phi into look at coord
    lookVector.y = sin(th_ph.y) + playerPosition.y;
    lookVector.x = cos(th_ph.y) * sin(th_ph.x) + playerPosition.x;
    lookVector.z = cos(th_ph.y) * cos(th_ph.x) + playerPosition.z;

    //remake cam
    view = lookAt(
        playerPosition,  // eye
        lookVector,  // center
        vec3(0.0, 1.0, 0.0));  // up
}


//--------------------------------------------------------------------------------
// Rendering
//--------------------------------------------------------------------------------

void Render()
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Attach to program_id
    glUseProgram(P_program_id);

    for (unsigned int i = 0; i < primitive_objects.size(); i++)
    {
        primitive_object* obj = &primitive_objects[i];
        // Do transformation
        (*obj).model = rotate((*obj).model, 0.01f, vec3(0.0f, 1.0f, 0.0f));
        (*obj).mv = view * (*obj).model;

        // Send mvp
        glUniformMatrix4fv((*obj).uniform_mvp, 1, GL_FALSE, value_ptr((*obj).mv));

        // Send vao
        glBindVertexArray((*obj).vao);
        glDrawElements(GL_TRIANGLES, (*obj).elements.size(),
            GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

    glUseProgram(O_program_id);

    for (unsigned int i = 0; i < textured_objects.size(); i++) {
        textured_object *obj = &textured_objects[i];
        (*obj).mv = view * (*obj).model;

        // Send mv
        glUniformMatrix4fv((*obj).uniform_mv, 1, GL_FALSE, value_ptr((*obj).mv));

        // Send vao
        glBindVertexArray((*obj).vao);
        glDrawArrays(GL_TRIANGLES, 0, (*obj).vertices.size());
        glBindVertexArray(0);
    }

    glutSwapBuffers();
}


//------------------------------------------------------------
// void Render(int n)
// Render method that is called by the timer function
//------------------------------------------------------------

void Render(int n)
{
    Render();
    glutTimerFunc(DELTA_TIME, Render, 0);
}


//------------------------------------------------------------
// void InitGlutGlew(int argc, char **argv)
// Initializes Glut and Glew
//------------------------------------------------------------

void InitGlutGlew(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Hello OpenGL");
    glutDisplayFunc(Render);
    glutKeyboardFunc(keyboardHandler);
    glutTimerFunc(DELTA_TIME, Render, 0);

    glewInit();
}


//------------------------------------------------------------
// void InitShaders()
// Initializes the fragmentshader and vertexshader
//------------------------------------------------------------

void InitShaders()
{
    //  PRIMITIVE
    char* Pvertexshader = glsl::readFile(Pvertexshader_name);
    GLuint Pvsh_id = glsl::makeVertexShader(Pvertexshader);

    char* Pfragshader = glsl::readFile(Pfragshader_name);
    GLuint Pfsh_id = glsl::makeFragmentShader(Pfragshader);

    P_program_id = glsl::makeShaderProgram(Pvsh_id, Pfsh_id);

    ///////////////////////////////////////////////////////

    //  LOADED
    char* Overtexshader = glsl::readFile(Overtexshader_name);
    GLuint Ovsh_id = glsl::makeVertexShader(Overtexshader);

    char* Ofragshader = glsl::readFile(Ofragshader_name);
    GLuint Ofsh_id = glsl::makeFragmentShader(Ofragshader);

    O_program_id = glsl::makeShaderProgram(Ovsh_id, Ofsh_id);
}


//------------------------------------------------------------
// void InitMatrices()
//------------------------------------------------------------

void InitMatrices()
{
    lookVector.y = sin(th_ph.y) + playerPosition.y;
    lookVector.x = cos(th_ph.y) * sin(th_ph.x) + playerPosition.x;
    lookVector.z = cos(th_ph.y) * cos(th_ph.x) + playerPosition.z;

    view = lookAt(
        playerPosition,  // eye
        lookVector,  // center
        vec3(0.0, 1.0, 0.0));  // up
    projection = perspective(
        radians(45.0f),
        1.0f * WIDTH / HEIGHT, 0.1f,
        20.0f);
    for (unsigned int i = 0; i < primitive_objects.size(); i++){
        primitive_objects[i].mv = view * primitive_objects[i].model;
    }
    for (unsigned int i = 0; i < textured_objects.size(); i++) {
        textured_objects[i].mv = view * textured_objects[i].model;
    }
    
}

primitive_object merge_prim(primitive_object p1, primitive_object p2) {
    GLushort M = 0;
    for (unsigned int i = 0; i < p1.elements.size(); i++) {
        if (p1.elements[i] < M)
            M = p1.elements[i];
    }
    vector<GLfloat> vec;
    vector<GLfloat> norm;
    vector<GLfloat> col;
    vector<GLushort> el;
    vec.insert(p1.vertices.end(), p2.vertices.begin(), p2.vertices.end());
    norm.insert(p1.normals.end(), p2.normals.begin(), p2.normals.end());
    col.insert(p1.colors.end(), p2.colors.begin(), p2.colors.end());
    for (GLushort& x : p2.elements) {
        x += M;
    }
    el.insert(p1.elements.end(), p2.elements.begin(), p2.elements.end());
    return primitive_object();
}

textured_object make_OBJ(const char* text, const char* obj) {
    vector<vec3> tmp_v, tmp_n;
    vector<vec2> tmp_u;
    bool res = loadOBJ(obj, tmp_v, tmp_u, tmp_n);
    textured_object out(tmp_v, tmp_u, tmp_n, loadBMP(text));
    return (out);

}

void InitObjects()
{
    //make primitive cube
    primitive_object cube_obj;

    {
        vector<GLfloat> cube_vec = {
            // front
            -1.0, 1.0, -1.0,   //0 //front bottom left
            -1.0, -1.0, -1.0,  //1  top
            1.0, -1.0, -1.0,   //2  right
            1.0, 1.0, -1.0,    //3 bottom right

            -1.0, 1.0, 1.0,    //4 //back bottom left
            -1.0, -1.0, 1.0,   //5  top
            1.0, -1.0, 1.0,    //6  right
            1.0, 1.0, 1.0      //7 bottom right
        };
        vector<GLfloat> cube_norm;
        float N = sqrtf(1.0f / 3.0f);
        for (GLfloat var : cube_vec) {
            cube_norm.push_back((var / abs(var)) * N);
        }
        vector<GLfloat> cube_col = {
            // front colors
            1.0, 1.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            1.0, 1.0, 1.0,
            // back colors
            0.0, 1.0, 1.0,
            1.0, 0.0, 1.0,
            1.0, 0.0, 0.0,
            1.0, 1.0, 0.0
        };
        vector<GLushort> cube_el = {
            0,1,3,  3,1,2,    // front
            4,5,0,  0,5,1,    // left
            7,6,4,  4,6,5,    // back
            3,2,7,  7,2,6,    // right
            1,5,2,  2,5,6,    // top
            3,7,0,  0,7,4     // bottom
        };
        GLchar cube_ch = GL_TRIANGLES;
        cube_obj = primitive_object(cube_vec, cube_norm, cube_col, cube_el, cube_ch);
    }

    //make primitive skybox
    primitive_object skybox_obj;

    {
        vector<GLfloat> skybox_vec = {
            // front
            -1.0, 1.0, -1.0,   //0 //front bottom left
            -1.0, -1.0, -1.0,  //1  top
            1.0, -1.0, -1.0,   //2  right
            1.0, 1.0, -1.0,    //3 bottom right

            -1.0, 1.0, 1.0,    //4 //back bottom left
            -1.0, -1.0, 1.0,   //5  top
            1.0, -1.0, 1.0,    //6  right
            1.0, 1.0, 1.0      //7 bottom right
        };
        vector<GLfloat> skybox_norm;
        float N = sqrtf(1.0f / 3.0f);
        for (GLfloat var : skybox_vec) {
            skybox_norm.push_back((var / abs(var)) * -N);
        }
        vector<GLfloat> skybox_col = {
            // front colors
            1.0, 1.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            1.0, 1.0, 1.0,
            // back colors
            0.0, 1.0, 1.0,
            1.0, 0.0, 1.0,
            1.0, 0.0, 0.0,
            1.0, 1.0, 0.0
        };
        vector<GLushort> skybox_el = {
            0,1,3,  3,1,2,    // front
            4,5,0,  0,5,1,    // left
            7,6,4,  4,6,5,    // back
            3,2,7,  7,2,6,    // right
            1,5,2,  2,5,6,    // top
            3,7,0,  0,7,4     // bottom
        };
        GLchar skybox_ch = GL_TRIANGLES;
        skybox_obj = primitive_object(skybox_vec, skybox_norm, skybox_col, skybox_el, skybox_ch);
    }

    //make primitive plane
    primitive_object plane_obj;

    {
        vector<GLfloat> plane_vec = {
            -1,0,-1,
            -1,0,1,
            1,0,1,
            1,0,-1
        };
        vector<GLfloat> plane_norm{
            0,1,0,
            0,1,0,
            0,1,0,
            0,1,0
        };
        vector<GLfloat> plane_col = {
            1.0, 1.0, 0.0,//yellow
            0.0, 1.0, 0.0,//green
            0.0, 0.0, 1.0,//blue
            1.0, 1.0, 1.0//white
        };
        vector<GLushort> plane_el = {
            0,1,3,
            3,1,2
        };
        GLchar plane_ch = GL_TRIANGLES;
        plane_obj = primitive_object(plane_vec, plane_norm, plane_col, plane_el, plane_ch);
        plane_obj.model = translate(mat4(), vec3(0, 0, -3));
    }

    //make primitive circle
    int resolution = 32;
    float radius = 1.5;
    primitive_object circle_obj;

    {
        vector<GLfloat> circle_vec;
        vector<GLfloat> circle_norm;
        vector<GLfloat> circle_col;
        for (unsigned int i = 0; i < 3; i++) {
            circle_vec.push_back(0);
            circle_norm.push_back(i==1);
        }
        for (int i = 0; i < resolution; i++){
            float rad = (float)i / (float)resolution * (float)M_PI * 2;
            circle_vec.push_back(sinf(rad)*radius);  //x
            circle_vec.push_back(0);                 //y
            circle_vec.push_back(cosf(rad)*radius);  //z

            circle_norm.push_back(0);
            circle_norm.push_back(1);
            circle_norm.push_back(0);
        }

        circle_col = circle_vec;
        circle_col[0] = 1;
        circle_col[1] = 1;
        circle_col[2] = 1;
        vector<GLushort> circle_el;
        for (int i = 1; i < resolution+1; i++) {
            circle_el.push_back(0);
            circle_el.push_back(i);
            if (i != resolution) {
                circle_el.push_back(i + 1);
            } else {
                circle_el.push_back(1);
            }
        }

        GLchar circle_ch = GL_TRIANGLES;
        circle_obj = primitive_object(circle_vec, circle_norm, circle_col, circle_el, circle_ch);
        circle_obj.model = translate(mat4(), vec3(2, 0, 0));
    }

    //make primitive cone
    float height = 5.5f;
    primitive_object cone_obj;

    {
        vector<GLfloat> cone_vec;
        vector<GLfloat> cone_norm;
        vector<GLfloat> cone_col;
        for (unsigned int i = 0; i < 3; i++) {
            cone_vec.push_back(0);
            cone_norm.push_back((i == 1)?-1.0f:0.0f);
        }
        for (int i = 0; i < resolution; i++) {
            float rad = (float)i / (float)resolution * (float)M_PI * 2;
            cone_vec.push_back(sinf(rad) * radius);  //x
            cone_vec.push_back(0);                   //y
            cone_vec.push_back(cosf(rad) * radius);  //z

            vec3 temp(sinf(rad)* radius, 1, cosf(rad)* radius);
            temp = normalize(temp);
            cone_norm.push_back(temp.x);
            cone_norm.push_back(temp.y);
            cone_norm.push_back(temp.z);
        }

        cone_vec.push_back(0);      //top xyz
        cone_vec.push_back(height);
        cone_vec.push_back(0);

        cone_norm.push_back(0);     //top norm
        cone_norm.push_back(1);
        cone_norm.push_back(0);

        cone_col = cone_vec;

        for (unsigned int i = 0; i < 3; i++) {
            cone_col[i] = 1;            //botom color
            cone_col.push_back(i==1);   //top color
        }

        vector<GLushort> cone_el;
        for (int i = 1; i < resolution + 1; i++) {
            cone_el.push_back(0);
            cone_el.push_back(i);
            if (i != resolution) {
                cone_el.push_back(i + 1);
            }
            else {
                cone_el.push_back(1);
            }
        }
        for (int i = 1; i < resolution + 1; i++) {
            cone_el.push_back(cone_vec.size()/3-1);
            cone_el.push_back(i);
            if (i != resolution) {
                cone_el.push_back(i + 1);
            }
            else {
                cone_el.push_back(1);
            }
        }

        GLchar cone_ch = GL_TRIANGLES;
        cone_obj  = primitive_object(cone_vec, cone_norm, cone_col, cone_el, cone_ch);
        cone_obj.model = translate(mat4(), vec3(2, 1, 0));
    }

    //make primitive cilinder
    primitive_object cilinder_obj;

    {
        vector<GLfloat> cilinder_vec;
        vector<GLfloat> cilinder_norm;
        vector<GLfloat> cilinder_col;
        
        int topCStart = 0;

        for (int j = 0; j < 2; j++){
            for (unsigned int i = 0; i < 3; i++) {
                cilinder_vec.push_back((j==1 && i==1)?height:0);
                cilinder_norm.push_back((i == 1) ? 1-(2*j) : 0);
            }

            for (int i = 0; i < resolution; i++) {
                float rad = (float)i / (float)resolution * (float)M_PI * 2;
                cilinder_vec.push_back(sinf(rad) * radius);     //x
                cilinder_vec.push_back((j == 1) ? height : 0);  //y
                cilinder_vec.push_back(cosf(rad) * radius);     //z

                vec3 temp(sinf(rad)* radius, -1+2*j, cosf(rad)* radius);
                temp = normalize(temp);
                cilinder_norm.push_back(temp.x);
                cilinder_norm.push_back(temp.y);
                cilinder_norm.push_back(temp.z);
            }
            if(topCStart ==0)
                topCStart = (int)cilinder_vec.size()/3;
        }

        cilinder_col = cilinder_vec;
        for (unsigned int i = 0; i < 3; i++)
        {
            cilinder_col[i] = 1;
            cilinder_col[topCStart*3+i] = 1;
        }
        
        vector<GLushort> cilinder_el;
        for (int i = 1; i < resolution + 1; i++) {  //botom
            cilinder_el.push_back(0);
            cilinder_el.push_back(i);
            if (i != resolution) {
                cilinder_el.push_back(i + 1);
            }
            else {
                cilinder_el.push_back(1);
            }
        }
        
        for (int i = topCStart; i < resolution + topCStart+1; i++) {  //top
            cilinder_el.push_back(topCStart);
            cilinder_el.push_back(i);
            if (i != resolution + topCStart) {
                cilinder_el.push_back(i + 1);
            }
            else {
                cilinder_el.push_back(1+topCStart);
            }
        }

        for (int i = 1; i < resolution+1; i++) {
            
            cilinder_el.push_back(topCStart+i);
            cilinder_el.push_back(i);
            if (i+1 > resolution) {
                cilinder_el.push_back(1);
            }
            else {
                cilinder_el.push_back(1 + i);
            }
            
            cilinder_el.push_back((i!=resolution)? topCStart+1+i:topCStart+1);
            cilinder_el.push_back(topCStart + i);
            cilinder_el.push_back((i != resolution) ? i+1 :1);
        }

        GLchar cilinder_ch = GL_TRIANGLES;
        cilinder_obj = primitive_object(cilinder_vec, cilinder_norm, cilinder_col, cilinder_el, cilinder_ch);
        cilinder_obj.model = translate(mat4(), vec3(2, 0, 0));
    }

    //primitive_objects.push_back(skybox_obj);
    //primitive_objects.push_back(cube_obj);
    //primitive_objects.push_back(plane_obj);
    //primitive_objects.push_back(circle_obj);
    //primitive_objects.push_back(cone_obj);
    //primitive_objects.push_back(cilinder_obj);
    
    ////////////////////////////////////////////////////
    
    textured_object box = make_OBJ("Textures/uvtemplate.bmp", "objects/box.obj");
    box.model = translate(mat4(), vec3(0, 1, 0));
    textured_objects.push_back(box);
}


//------------------------------------------------------------
// void InitBuffers()
// Allocates and fills buffers
//------------------------------------------------------------

void InitBuffers()
{
    //text obj
    for (unsigned int i = 0; i < textured_objects.size(); i++) {
        GLuint position_id, normal_id;
        GLuint vbo_vertices, vbo_normals, vbo_uvs;

        textured_object* obj = &textured_objects[i];

        glGenBuffers(1, &vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER,
            (*obj).vertices.size() * sizeof(vec3), &((*obj).vertices[0]),
            GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for normals
        glGenBuffers(1, &vbo_normals);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
        glBufferData(GL_ARRAY_BUFFER,
            (*obj).normals.size() * sizeof(vec3),
            &(*obj).normals[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for uvs
        glGenBuffers(1, &vbo_uvs);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
        glBufferData(GL_ARRAY_BUFFER, (*obj).uvs.size() * sizeof(vec2),
            &(*obj).uvs[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);


        // Get vertex attributes
        position_id = glGetAttribLocation(O_program_id, "position");
        normal_id = glGetAttribLocation(O_program_id, "normal");
        GLuint uv_id = glGetAttribLocation(O_program_id, "uv");


        // Allocate memory for vao
        glGenVertexArrays(1, &(*obj).vao);

        // Bind to vao
        glBindVertexArray((*obj).vao);

        // Bind vertices to vao
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //normal
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normals);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //textures
        glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs);
        glVertexAttribPointer(uv_id, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(uv_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);


        // Stop bind to vao
        glBindVertexArray(0);


        // Make uniform vars
        //uniform_mvp = glGetUniformLocation(program_id, "mvp");

        textured_objects[i].uniform_mv = glGetUniformLocation(O_program_id, "mv");
        GLuint uniform_proj = glGetUniformLocation(O_program_id, "projection");
        GLuint uniform_light_pos = glGetUniformLocation(O_program_id, "light_pos");
        GLuint uniform_material_ambient = glGetUniformLocation(O_program_id,
            "mat_ambient");
        GLuint uniform_material_diffuse = glGetUniformLocation(O_program_id,
            "mat_diffuse");




        // Define model
        (*obj).mv = view * (*obj).model;

        // Send mv
        glUseProgram(O_program_id);
        glUniformMatrix4fv((*obj).uniform_mv, 1, GL_FALSE, value_ptr((*obj).mv));
        glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, value_ptr(projection));
        glUniform3fv(uniform_light_pos, 1, value_ptr(light_position));
        glUniform3fv(uniform_material_ambient, 1, value_ptr(ambient_color));
        glUniform3fv(uniform_material_diffuse, 1, value_ptr(diffuse_color));
    }
    //prim
    for (unsigned int i = 0; i < primitive_objects.size(); i++)
    {
        GLuint position_id, color_id, normal_id;
        GLuint vbo_vertices, vbo_colors, vbo_normal;
        GLuint ibo_elements;

        primitive_object *obj = &primitive_objects[i];

        // vbo for vertices
        glGenBuffers(1, &vbo_vertices);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glBufferData(GL_ARRAY_BUFFER, (*obj).vertices.size() * sizeof(GLfloat), &(*obj).vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for colors
        glGenBuffers(1, &vbo_colors);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glBufferData(GL_ARRAY_BUFFER, (*obj).colors.size() * sizeof(GLfloat), &(*obj).colors[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for colors
        glGenBuffers(1, &vbo_normal);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glBufferData(GL_ARRAY_BUFFER, (*obj).normals.size() * sizeof(vec3), &(*obj).normals[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // vbo for elements
        glGenBuffers(1, &ibo_elements);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (*obj).elements.size() * sizeof(GLshort),
            &(*obj).elements[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // Get vertex attributes
        position_id = glGetAttribLocation(P_program_id, "position");
        color_id = glGetAttribLocation(P_program_id, "color");
        normal_id = glGetAttribLocation(P_program_id, "normal");

        // Allocate memory for vao
        glGenVertexArrays(1, &(*obj).vao);

        // Bind to vao
        glBindVertexArray((*obj).vao);

        // Bind vertices to vao
        glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices);
        glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(position_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Bind colors to vao
        glBindBuffer(GL_ARRAY_BUFFER, vbo_colors);
        glVertexAttribPointer(color_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(color_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Bind normals to vao
        glBindBuffer(GL_ARRAY_BUFFER, vbo_normal);
        glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(normal_id);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Bind elements to vao
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_elements);

        // Stop bind to vao
        glBindVertexArray(0);

        // Make uniform vars
        (*obj).uniform_mvp = glGetUniformLocation(P_program_id, "mv");
        GLuint uniform_proj = glGetUniformLocation(P_program_id, "projection");
        GLuint uniform_light_pos = glGetUniformLocation(P_program_id, "light_pos");
        GLuint uniform_material_ambient = glGetUniformLocation(P_program_id,
            "mat_ambient");
        GLuint uniform_material_diffuse = glGetUniformLocation(P_program_id,
            "mat_diffuse");

        // Define model
        (*obj).mv = view * (*obj).model;

        // Send mv
        glUseProgram(P_program_id);
        glUniformMatrix4fv((*obj).uniform_mvp, 1, GL_FALSE, value_ptr((*obj).mv));
        glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, value_ptr(projection));
        glUniform3fv(uniform_light_pos, 1, value_ptr(light_position));
        glUniform3fv(uniform_material_ambient, 1, value_ptr(ambient_color));
        glUniform3fv(uniform_material_diffuse, 1, value_ptr(diffuse_color));
    }
}


int main(int argc, char** argv)
{
    InitGlutGlew(argc, argv);
    InitShaders();
    InitMatrices();
    InitObjects();
    InitBuffers();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_HIDE);

    // Main loop
    glutMainLoop();

    return 0;
}
