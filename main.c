#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "space invaders"


void init_glfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    printf("INFO : glfw initilize!\n");
}

GLFWwindow* create_window()
{
    GLFWwindow* window;
    GLFWmonitor** monitors = {0};
    int count_monitors = 0;
    bool full_screen = false;
    if (full_screen) {
        monitors = glfwGetMonitors(&count_monitors);
        if (!monitors) {
            fprintf(stderr, "ERROR: Could not found any monitor!\n");
            return NULL;
        }
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, monitors[0], NULL);
    } else {
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    }
    if (!window) {
        fprintf(stderr, "ERROR: Could not create window!\n");
        return NULL;
    }
    printf("INFO : glfw window has been created!\n");
    glfwMakeContextCurrent(window);
    return window;
}


const char *read_entire_file(const char* filename)
{
    FILE *file;
    fopen_s(&file, filename, "r");
    if (file == NULL) {
        fprintf(stderr, "ERROR: Could not open file %s\n", filename);
        return "";
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = malloc((sizeof(char) * length) + 1);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not malloc memory for reading a file. Please buy more RAM!");
        return NULL;
    }
    fread(buffer, sizeof(char), length, file);
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}


unsigned int compile_shader(const char *vertex_filename, const char *fragment_filename)
{
    int success;
    char msg[512];
    // vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char *vertex_shader_source = read_entire_file(vertex_filename);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    // check for shader compile errors
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader, 512, NULL, msg);
        fprintf(stderr, "ERROR: Vertex shader compilation failed: %s\n", msg);
    }
    // fragment shader
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_shader_source = read_entire_file(fragment_filename);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);
    // check for shader compile errors
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragment_shader, 512, NULL, msg);
        fprintf(stderr, "ERROR: Fragment shader compilation failed: %s\n", msg);
    }
    // link shaders
    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    // check for linking errors
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, msg);
        fprintf(stderr, "ERROR: Shader linking failed: %s\n", msg);
    }
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    printf("INFO : Shader compilation is done!\n");
    return shader_program;
}


void load_image_and_bind_texture(const char * texture_filename)
{
    int width, height, n_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char * data = stbi_load(texture_filename, &width, &height, &n_channels, STBI_rgb_alpha);
    if(data){
        if (n_channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        else if (n_channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        printf("INFO : texture %s is read!\n", texture_filename);
    }
    else {
        fprintf(stderr, "ERROR! can not load image!\n");
    }
    stbi_image_free(data);
}


typedef struct {
        float data[16];
} Matrix4;

static const Matrix4 IDENTITY_MATRIX = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
}};

const Matrix4 multiply_matrix4(Matrix4* mat1, Matrix4* m2)
{
    Matrix4 out = IDENTITY_MATRIX;
    for (size_t row = 0, row_offset = row * 4; row < 4; ++row, row_offset = row * 4)
        for (size_t column = 0; column < 4; ++column)
            out.data[row_offset + column] =
                (mat1->data[row_offset + 0] * m2->data[column + 0]) +
                (mat1->data[row_offset + 1] * m2->data[column + 4]) +
                (mat1->data[row_offset + 2] * m2->data[column + 8]) +
                (mat1->data[row_offset + 3] * m2->data[column + 12]);
    return out;
}

void translate(Matrix4* m, float x, float y, float z)
{
        Matrix4 translation = IDENTITY_MATRIX;
        translation.data[12] = x;
        translation.data[13] = y;
        translation.data[14] = z;
        memcpy(m->data, multiply_matrix4(m, &translation).data, sizeof(m->data));
}

#define SPEED 0.001f

typedef struct {
    unsigned int VAO;       // Vertex array
    unsigned int VBO, TBO;  // Vertices and Texture buffers
    unsigned int IBO;       // Indices buffer
    unsigned int shader;
    unsigned int texture;
    size_t n_component;  // number of component to draw

    float rel_position_x, rel_position_y;  // relativ positions (-1,1)
} Object;


Object* create_object(float *vertices, size_t n_vertices, float *texture_coords, unsigned int *indices,
                      size_t n_component, const char *texture_filename,
                      const char *vertex_shader_filename, const char *fragment_shader_filename)
{
    Object *obj = malloc(sizeof(Object));
    if (obj == NULL) {
        fprintf(stderr, "ERROR: Could not malloc memory for a object. Please buy more RAM!");
        return NULL;
    }

    // shader
   obj->shader = compile_shader(vertex_shader_filename, fragment_shader_filename);

    glGenVertexArrays(1, &obj->VAO);
    glGenBuffers(1, &obj->VBO);
    glGenBuffers(1, &obj->TBO);
    glGenBuffers(1, &obj->IBO);

    glBindVertexArray(obj->VAO);
    glBindBuffer(GL_ARRAY_BUFFER, obj->VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*n_vertices*3, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, obj->TBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*n_vertices*2, texture_coords, GL_STATIC_DRAW);

    obj->n_component = n_component;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int)*n_component, indices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, obj->VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, obj->TBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    //texture
    glGenTextures(1, &obj->texture);
    glBindTexture(GL_TEXTURE_2D, obj->texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    load_image_and_bind_texture(texture_filename);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    /* glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); */
    glBindVertexArray(0);

    obj->rel_position_x = 0;
    obj->rel_position_y = 0;

    return obj;
}


typedef struct {
    float x, y, z;
} Point;


typedef struct {
    Point position;  // center position (not edge position)
    float height, width;
} Rectangle;


Object* create_rectangle_object(Rectangle rectangle, const char *texture_filename,
                      const char *vertex_shader_filename, const char *fragment_shader_filename)
{
    const Point center     = rectangle.position;
    const float rel_height = rectangle.height / 2;
    const float rel_width  = rectangle.width  / 2;
    float vertices[] = {
        center.x+rel_width, center.y+rel_height, 0,  // right up
        center.x+rel_width, center.y-rel_height, 0,  // right down
        center.x-rel_width, center.y-rel_height, 0,  // left down
        center.x-rel_width, center.y+rel_height, 0,  // left up
    };

    float texture_coords[] = {
        1.0f, 1.0f,
        1.0f, 0.0f,
        0.0f, 0.0f,
        0.0f, 1.0f,
    };
    unsigned int indices[] = {
        0, 1, 2,
        0, 2, 3
    };

    return create_object(vertices, 4, texture_coords, indices, 6,
                         texture_filename, vertex_shader_filename, fragment_shader_filename);
}


void delete_object(Object *obj)
{
    glDeleteVertexArrays(1, &obj->VAO);
    glDeleteBuffers(1, &obj->VBO);
    glDeleteBuffers(1, &obj->TBO);
    glDeleteBuffers(1, &obj->IBO);
    glDeleteProgram(obj->shader);
    free(obj);
}


void draw_object(Object *obj)
{
    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE);
    glBindTexture(GL_TEXTURE_2D, obj->texture);

    glBindVertexArray(obj->VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)(obj->n_component), GL_UNSIGNED_INT, 0);
}


void check_object_moving(GLFWwindow* window, Object *obj)
{
    // if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
    //     glfwSetWindowShouldClose(window, true);
    // }
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (obj->rel_position_x < 1.0f) obj->rel_position_x += SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (obj->rel_position_x > -1.0f) obj->rel_position_x -= SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (obj->rel_position_y > -1.0f) obj->rel_position_y -= SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (obj->rel_position_y < 1.0f) obj->rel_position_y += SPEED;
    }
}


int main ()
{
    init_glfw();

    GLFWwindow* window = NULL;
    window = create_window(window);
    if (!window) return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    Object *player = create_rectangle_object((Rectangle) {
        .position = {0.0f, -0.8f, 0.0f},
        .height = 0.15f,
        .width = 0.3f,
        },
        "resources/player.png", "resources/dynamic_vertex.glsl", "resources/fragment.glsl");

    Object *enemy = create_rectangle_object((Rectangle) {
        .position = {0.0f, 0.8f, 0.0f},
        .height = 0.16f,
        .width = 0.2f,
        },
        "resources/red.png", "resources/static_vertex.glsl", "resources/fragment.glsl");

    glUseProgram(player->shader);

    while (!glfwWindowShouldClose(window)) {
        check_object_moving(window, player);

        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Matrix4 transform = IDENTITY_MATRIX;
        translate(&transform, player->rel_position_y, player->rel_position_x, 0.0f);

        glUseProgram(player->shader);
        unsigned int transform_loc = glGetUniformLocation(player->shader, "transform");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &(transform.data[0]));
        draw_object(player);

        glUseProgram(enemy->shader);
        draw_object(enemy);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete_object(player);
    delete_object(enemy);
    glfwTerminate();

    printf("Finish Program!");
    return 0;
}
