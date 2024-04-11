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


//==========Window==========//
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "space invaders"


void init_glfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    printf("INFO : GLFW initialize!\n");
}

GLFWwindow *create_window()
{
    GLFWwindow *window;
    GLFWmonitor **monitors = {0};
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
    printf("INFO : GLFW window has been created!\n");
    glfwMakeContextCurrent(window);
    return window;
}

void frame_buffer_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}


//==========Shader==========//
const char *read_entire_file(const char *filename)
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

    char *buffer = malloc((sizeof(char) * length) + 1);
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not malloc memory for reading a file. Please buy more RAM!");
        return NULL;
    }
    fread(buffer, sizeof(char), length, file);
    buffer[length] = '\0';
    fclose(file);

    return buffer;
}

unsigned int compile_shader(const char *vertex_filename, const char *fragment_filename) {
    int success;
    char msg[512];
    // vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char *vertex_shader_source = read_entire_file(vertex_filename);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    // check for shader compile errors
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
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
    if (!success) {
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

//==========Texture==========//
void load_image_and_bind_texture(const char *texture_filename)
{
    int width, height, n_channels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(texture_filename, &width, &height, &n_channels, STBI_rgb_alpha);
    if (data) {
        GLenum texture_format = 0;
        switch (n_channels) {
            case 3: texture_format = GL_RGB; break;
            case 4: texture_format = GL_RGBA; break;
            default: fprintf(stderr, "ERROR: Invalid number of texture image channels");
        }
        glTexImage2D(GL_TEXTURE_2D,     // target
                     0,                 // samples
                     texture_format,            // internal format
                     width, height,     // size
                     0,                 // border
                     texture_format,    // format
                     GL_UNSIGNED_BYTE,  // type
                     data               // pixels
        );
        glGenerateMipmap(GL_TEXTURE_2D);
        printf("INFO : Texture %s is read!\n", texture_filename);
    } else {
        fprintf(stderr, "ERROR: Cannot load image!\n");
    }
    stbi_image_free(data);
}


//==========Linear Algebra==========//
typedef struct {
        float data[16];
} Matrix4;

typedef struct {
    float x, y, z;
} Point;


static const Matrix4 IDENTITY_MATRIX = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
}};

const Matrix4 multiply_matrix4(Matrix4 *mat1, Matrix4 *m2)
{
    Matrix4 out = IDENTITY_MATRIX;
    for (size_t row = 0, row_offset = row * 4; row < 4; ++row, row_offset = row * 4)
        for (size_t column = 0; column < 4; ++column)
            out.data[row_offset + column] = (mat1->data[row_offset + 0] * m2->data[column + 0]) +
                (mat1->data[row_offset + 1] * m2->data[column + 4]) +
                (mat1->data[row_offset + 2] * m2->data[column + 8]) +
                (mat1->data[row_offset + 3] * m2->data[column + 12]);
    return out;
}

void translate(Matrix4 *m, Point vector)
{
        Matrix4 translation = IDENTITY_MATRIX;
        translation.data[12] = vector.x;
        translation.data[13] = vector.y;
        translation.data[14] = vector.z;
        memcpy(m->data, multiply_matrix4(m, &translation).data, sizeof(m->data));
}


//==========Object==========//
typedef struct {
    unsigned int VAO;       // Vertex array
    unsigned int VBO, TBO;  // Vertices and Texture buffers
    unsigned int IBO;       // Indices buffer
    unsigned int shader;
    unsigned int texture;
    size_t n_component;  // number of component to draw
    Point initialize_position;
    Point initialize_size;
    Point rel_position;  // relative positions (-1,1)
} Object;

Object *create_object(float *vertices, size_t n_vertices, float *texture_coords,
                      unsigned int *indices, size_t n_component, const char *texture_filename,
                      const char *vertex_shader_filename, const char *fragment_shader_filename)
{
    Object *obj = malloc(sizeof(Object));
    if (obj == NULL) {
        fprintf(stderr, "ERROR: Cannot malloc memory for a object. Please buy more RAM!");
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n_vertices * 3, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, obj->TBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n_vertices * 2, texture_coords, GL_STATIC_DRAW);

    obj->n_component = n_component;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * n_component, indices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, obj->VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, obj->TBO);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);

    // texture
    if (texture_filename) {
        glGenTextures(1, &obj->texture);
        glBindTexture(GL_TEXTURE_2D, obj->texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        load_image_and_bind_texture(texture_filename);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    /* glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); */
    glBindVertexArray(0);

    obj->rel_position = (Point){0, 0, 0};

    return obj;
}

typedef struct {
    Point position;  // center position (not edge position)
    float height, width;
} Rectangle;

Object *create_rectangle_object(Rectangle rectangle, const char *texture_filename,
                                const char *vertex_shader_filename,
                                const char *fragment_shader_filename)
{
    const Point center     = rectangle.position;
    const float rel_height = rectangle.height / 2;
    const float rel_width  = rectangle.width / 2;
    float vertices[] = {
        center.x + rel_width, center.y + rel_height, 0,  // right up
        center.x + rel_width, center.y - rel_height, 0,  // right down
        center.x - rel_width, center.y - rel_height, 0,  // left down
        center.x - rel_width, center.y + rel_height, 0,  // left up
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

    Object *obj = create_object(vertices, 4, texture_coords, indices, 6, texture_filename,
                                vertex_shader_filename, fragment_shader_filename);
    if (obj != NULL) {
        obj->initialize_position = center;
        obj->initialize_size = (Point){rectangle.width, rectangle.height, 0};
    }
    return obj;
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

void move_object(Object *obj)
{
    Matrix4 transform = IDENTITY_MATRIX;
    translate(&transform, obj->rel_position);

    glUseProgram(obj->shader);
    unsigned int transform_loc = glGetUniformLocation(obj->shader, "transform");
    glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &(transform.data[0]));
    draw_object(obj);
}


//==========Fire==========//
typedef struct {
    Object *fires[1024];
    int count;
    double last_fire_time;  // store last fire to stop spaming fire
} Fires;

Fires fires = {0};

#define FIRE_HEIGHT 0.1f
#define FIRE_WIDTH  0.02f

#define FIRE_SPAWN_DELAY 0.9f  // 0.9 second

void fire_fire(Point init_position)
{
    fires.fires[fires.count++] = create_rectangle_object(
        (Rectangle){
            .position = init_position,
            .height = FIRE_HEIGHT,
            .width = FIRE_WIDTH,
        },
        NULL,
        "resources/fire_vertex.glsl",
        "resources/fragment_no_texture.glsl");
    fires.last_fire_time = glfwGetTime();
    printf("INFO : Spawn a fire\n");
}

#define FIRE_SPEED 0.002f

void move_fires()
{
    for (size_t i = 0; i < fires.count; i++) {
        Object *obj = fires.fires[i];
        obj->rel_position = (Point){.x = 0, .y = obj->rel_position.y + FIRE_SPEED, .z = 0};
        move_object(obj);
        draw_object(obj);
    }
}

void delete_fires()
{
    for (size_t i = 0; i < fires.count; i++) {
        delete_object(fires.fires[i]);
    }
    fires.count = 0;
}


//==========Player Action==========//
#define PLAYER_SPEED 0.001f

void check_object_moving(GLFWwindow *window, Object *obj)
{
    // if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
    //     glfwSetWindowShouldClose(window, true);
    // }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (obj->rel_position.y + obj->initialize_position.y >
            (-1.0f + (obj->initialize_size.y / 2)))
            obj->rel_position.y -= PLAYER_SPEED;
    } else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (obj->rel_position.y + obj->initialize_position.y <
            (1.0f - (obj->initialize_size.y / 2)))
            obj->rel_position.y += PLAYER_SPEED;
    } else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (obj->rel_position.x + obj->initialize_position.x <
            (1.0f - (obj->initialize_size.x / 2)))
            obj->rel_position.x += PLAYER_SPEED;
    } else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (obj->rel_position.x + obj->initialize_position.x >
            (-1.0f + (obj->initialize_size.x / 2)))
            obj->rel_position.x -= PLAYER_SPEED;
    } else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        double time = glfwGetTime();
        if (fires.last_fire_time + FIRE_SPAWN_DELAY < time) {
            Point position = (Point){.x = obj->initialize_position.x + obj->rel_position.x,
                                     .y = obj->initialize_position.y + obj->rel_position.y + 0.1f,
                                     .z = obj->initialize_position.z + obj->rel_position.z};
            fire_fire(position);  // spawn based on player(obj) position
        }
    }
}


//==========Enemies==========//
#define NUMBER_OF_RED_ENEMIES_IN_ROW 8
#define RED_ENEMIES_SCALES 0.8f

Object **create_red_enemies()
{
    Object **enemies = malloc(sizeof(Object));
    const float STRIDE = 1.6f / NUMBER_OF_RED_ENEMIES_IN_ROW;
    for (int i = 0; i < (int)(NUMBER_OF_RED_ENEMIES_IN_ROW); i++) {
        float x_position = (i * STRIDE) - 1.0f + 0.3f;
        enemies[i] = create_rectangle_object(
            (Rectangle){
                .position = {x_position, 0.8f, 0.0f},
                .height = 0.16f * RED_ENEMIES_SCALES,
                .width = 0.2f * RED_ENEMIES_SCALES,
            },
            "resources/red.png", "resources/dynamic_vertex.glsl", "resources/fragment.glsl");
        printf("INFO : A red enemy was created in position (%f, %f)\n", x_position, 0.8f);
    }
    return enemies;
}

#define NUMBER_OF_GREEN_ENEMIES_IN_ROW 8
#define GREEN_ENEMIES_SCALES 0.8f

Object **create_green_enemies()
{
    Object **enemies = malloc(sizeof(Object));
    const float STRIDE = 1.6f / NUMBER_OF_GREEN_ENEMIES_IN_ROW;
    for (size_t i = 0; i < (int)(NUMBER_OF_GREEN_ENEMIES_IN_ROW); i++) {
        float x_position = (i * STRIDE) - 1.0f + 0.3f;
        enemies[i] = create_rectangle_object(
            (Rectangle){
                .position = {x_position, 0.6f, 0.0f},
                .height = 0.16f * RED_ENEMIES_SCALES,
                .width = 0.2f * RED_ENEMIES_SCALES,
            },
            "resources/green.png", "resources/dynamic_vertex.glsl", "resources/fragment.glsl");
        printf("INFO : A green enemy was created in position (%f, %f)\n", x_position, 0.6f);
    }
    return enemies;
}

#define ENEMY_SPEED 3.0

void move_enemies(Object **objects, const size_t number_of_enemies)
{
    double time = sin(glfwGetTime() * ENEMY_SPEED) * 0.2;
    for (size_t i = 0; i < number_of_enemies; i++) {
        Object *obj = objects[i];
        Matrix4 transform = IDENTITY_MATRIX;
        translate(&transform, (Point){.x = (float)time, .y = 0, .z = 0});
        glUseProgram(obj->shader);
        unsigned int transform_loc = glGetUniformLocation(obj->shader, "transform");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &(transform.data[0]));
        draw_object(obj);
    }
}

void delete_enemies(Object **objects, const size_t number_of_enemies)
{
    for (size_t i = 0; i < number_of_enemies; i++) {
        delete_object(objects[i]);
    }
}


int main()
{
    init_glfw();

    GLFWwindow *window = NULL;
    window = create_window(window);
    if (!window) return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "ERROR: Failed to initialize GLAD\n");
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, frame_buffer_callback);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Object *player = create_rectangle_object(
        (Rectangle){
            .position = {0.0f, -0.8f, 0.0f},
            .height = 0.15f,
            .width = 0.3f,
        },
        "resources/player.png",
        "resources/dynamic_vertex.glsl",
        "resources/fragment.glsl"
    );

    Object **red_enemies = create_red_enemies();
    Object **green_enemies = create_green_enemies();

    glUseProgram(player->shader);

    while (!glfwWindowShouldClose(window)) {
        check_object_moving(window, player);

        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        move_object(player);

        move_enemies(red_enemies, NUMBER_OF_RED_ENEMIES_IN_ROW);
        move_enemies(green_enemies, NUMBER_OF_GREEN_ENEMIES_IN_ROW);

        move_fires();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete_object(player);
    delete_enemies(red_enemies, NUMBER_OF_RED_ENEMIES_IN_ROW);
    delete_enemies(green_enemies, NUMBER_OF_GREEN_ENEMIES_IN_ROW);
    delete_fires();

    glfwTerminate();

    printf("INFO : Finish Program!");
    return 0;
}
