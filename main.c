#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

inline void pixels_clear(uint32_t *pixels, size_t size, uint32_t color)
{
    for (size_t i = 0; i < size; i++)
        pixels[i] = color;
}

//==========Window==========//
#define WINDOW_WIDTH  512
#define WINDOW_HEIGHT 256
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
    GLFWmonitor **monitors = { 0 };
    int count_monitors     = 0;
    bool full_screen       = false;
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
char *read_entire_file(const char *filename)
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

    char *buffer = malloc(sizeof(char) * (length + 1));
    memset(buffer, 0, sizeof(char) * (length + 1));
    if (buffer == NULL) {
        fprintf(stderr, "ERROR: Could not malloc memory for reading a file. Please buy more RAM!");
        return NULL;
    }
    size_t actual_size = fread(buffer, sizeof(char), length, file);
    fclose(file);

    return buffer;
}

unsigned int compile_shader(const char *vertex_filename, const char *fragment_filename)
{
    int success;
    char msg[512];
    // vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    char *vertex_shader_source = read_entire_file(vertex_filename);
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
    char *fragment_shader_source = read_entire_file(fragment_filename);
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

//==========Sprite==========//
typedef struct {
    uint8_t *data;
    uint32_t width, height;
    size_t x;
    size_t y;
} Sprite;

Sprite *create_new_sprite(uint32_t width, uint32_t height)
{
    uint8_t *data = (uint8_t *)malloc(width * height * sizeof(uint8_t));
    if (data == NULL) {
        fprintf(stderr, "Error: Cannot malloc memmory for sprite. Please buy more RAM!\n");
        return NULL;
    }
    Sprite *sprite = malloc(sizeof(Sprite));
    *sprite        = (Sprite){ data, width, height };
    return sprite;
}

void free_sprite(Sprite *sprite)
{
    free(sprite->data);
    sprite->data = NULL;
}

//==========Object==========//
typedef struct {
    Sprite *sprite;
    size_t x, y;
    size_t init_x, init_y;
} Object;

#define pixels(col, row) pixels[((col) * (WINDOW_WIDTH)) + (row)]

void draw_object(uint32_t *pixels, Object *obj, uint32_t color)
{
    const size_t left_up_x = obj->x - (obj->sprite->width / 2);
    const size_t left_up_y = obj->y - (obj->sprite->height / 2);
    for (size_t ys = 0; ys < obj->sprite->height; ys++) {
        for (size_t xs = 0; xs < obj->sprite->width; xs++) {
            size_t col = (ys + left_up_y);
            size_t row = xs + left_up_x;
            if ((col < WINDOW_HEIGHT) & (row < WINDOW_WIDTH) & (obj->sprite->data[xs + ((obj->sprite->height - ys - 1) * (obj->sprite->width))]))
                pixels(col, row) = color;
        }
    }
}

void delete_object(Object *obj)
{
    if (obj->sprite->data)
        free(obj->sprite->data);
    free(obj->sprite);
    free(obj);
}

//==========Player==========//
static Object PLAYER_OBJECT;

void init_player_object()
{
    PLAYER_OBJECT.sprite = create_new_sprite(11, 7);
    uint8_t temp[]       = {
        0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, // .....@.....
        0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
        0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@@@@.
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
    };
    memcpy_s(PLAYER_OBJECT.sprite->data, 11 * 7, temp, 11 * 7);
    PLAYER_OBJECT.x = PLAYER_OBJECT.init_x = WINDOW_WIDTH / 2;
    PLAYER_OBJECT.y = PLAYER_OBJECT.init_y = WINDOW_HEIGHT / 5;
}

//==========Player Action==========//
#define MAX_FIRES 20
Object *player_fires[MAX_FIRES];

void initialize_fires()
{
    for (int i = 0; i < MAX_FIRES; i++)
        player_fires[i] = NULL;
}

void moving_fires()
{
    for (int i = 0; i < MAX_FIRES; i++) {
        if (player_fires[i] != NULL) {
            player_fires[i]->y += 1;
            if (player_fires[i]->y > WINDOW_WIDTH) {
                free(player_fires[i]);
                player_fires[i] = NULL;
            }
        }
    }
}

void spawn_player_fire(Object *player, uint32_t *pixels)
{
    Object *fire          = malloc(sizeof(Object));
    fire->sprite          = create_new_sprite(1, 3);
    fire->sprite->data    = malloc(sizeof(uint8_t) * 2);
    fire->sprite->data[0] = 1;
    fire->sprite->data[1] = 1;
    fire->sprite->data[2] = 1;
    fire->sprite->x = fire->x = player->x;
    fire->sprite->y = fire->y = player->y;
    for (size_t i = 0; i < MAX_FIRES; i++) {
        if (player_fires[i] == NULL) {
            player_fires[i] = fire;
            return;
        }
    }
    fprintf(stderr, "ERROR: Could not spawn a new fire anymore\n");
}

#define PLAYER_SPEED          0.2f
#define PLAYER_FIRE_RATE_TIME 0.2f

void check_player_action(GLFWwindow *window, Object *player, uint32_t *pixels)
{
    static double last_spawn_fire = 0;
    static float changes          = 0;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (player->x + PLAYER_SPEED < WINDOW_WIDTH) {
            changes += PLAYER_SPEED;
            if (changes >= 1) {
                player->x += (size_t)(changes);
                changes -= 1;
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (player->x - PLAYER_SPEED >= 0) {
            changes -= PLAYER_SPEED;
            if (changes <= -1) {
                player->x += (size_t)(changes);
                changes += 1;
            }
        }
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        double curr_time = glfwGetTime();
        if (curr_time - last_spawn_fire > PLAYER_FIRE_RATE_TIME) {
            spawn_player_fire(player, pixels);
            last_spawn_fire = curr_time;
        }
    }
}

//==========Enemy==========//
#define ENEMY_SPEED 2

inline void moving_enemy_animation(Object *enemy, double curr_time)
{
    enemy->x = enemy->init_x + (int)(sin(curr_time * ENEMY_SPEED) * (WINDOW_WIDTH / 16));
}

#define NUMBER_OF_GREEN_ENEMIES_IN_ROW 8

Object **create_green_enemies()
{
    uint8_t temp[] = {
        0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, // ..@......@..
        0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, // ...@....@...
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, // ..@@@@@@@@..
        0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, // .@@.@@@@.@@.
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@@
        1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, // @.@@@@@@@@.@
        1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, // @.@......@.@
        0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0  // ...@@..@@...
    };
    Object **enemies = malloc(NUMBER_OF_GREEN_ENEMIES_IN_ROW * sizeof(Object));
    if (!enemies) {
        fprintf(stderr, "ERROR: Could not malloc memory for list of green enemies. Please buy more RAM!");
        return NULL;
    }
    const size_t STRIDE = (WINDOW_WIDTH * 3 / 4) / NUMBER_OF_GREEN_ENEMIES_IN_ROW;
    for (size_t i = 0; i < NUMBER_OF_GREEN_ENEMIES_IN_ROW; i++) {
        enemies[i] = malloc(sizeof(Object));
        if (!enemies[i]) {
            fprintf(stderr, "ERROR: Could not malloc memory for a green enemy. Please buy more RAM!");
            continue;
        }
        enemies[i]->sprite = create_new_sprite(12, 8);
        memcpy_s(enemies[i]->sprite->data, 12 * 8, temp, 12 * 8);
        enemies[i]->x = enemies[i]->init_x = (i * STRIDE) + (WINDOW_WIDTH / 8) + (STRIDE / 2);
        enemies[i]->y = enemies[i]->init_y = WINDOW_HEIGHT * 8 / 10;
        printf("INFO : A green enemy was created in position (%zu, %zu)\n", enemies[i]->x, enemies[i]->y);
    }
    return enemies;
}

#define NUMBER_OF_RED_ENEMIES_IN_ROW 8

Object **create_red_enemies()
{
    uint8_t temp[] = {
        0, 0, 0, 1, 1, 0, 0, 0, // ...@@...
        0, 0, 1, 1, 1, 1, 0, 0, // ..@@@@..
        0, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@.
        1, 1, 0, 1, 1, 0, 1, 1, // @@.@@.@@
        1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@
        0, 1, 0, 1, 1, 0, 1, 0, // .@.@@.@.
        1, 0, 0, 0, 0, 0, 0, 1, // @......@
        0, 1, 0, 0, 0, 0, 1, 0  // .@....@.
    };
    Object **enemies = malloc(NUMBER_OF_GREEN_ENEMIES_IN_ROW * sizeof(Object));
    if (!enemies) {
        fprintf(stderr, "ERROR: Could not malloc memory for list of red enemies. Please buy more RAM!");
        return NULL;
    }
    const size_t STRIDE = (WINDOW_WIDTH * 3 / 4) / NUMBER_OF_RED_ENEMIES_IN_ROW;
    for (size_t i = 0; i < NUMBER_OF_RED_ENEMIES_IN_ROW; i++) {
        enemies[i] = malloc(sizeof(Object));
        if (!enemies[i]) {
            fprintf(stderr, "ERROR: Could not malloc memory for a red enemy. Please buy more RAM!");
            continue;
        }
        enemies[i]->sprite = create_new_sprite(8, 8);
        memcpy_s(enemies[i]->sprite->data, 8 * 8, temp, 8 * 8);
        enemies[i]->x = enemies[i]->init_x = (i * STRIDE) + (WINDOW_WIDTH / 8) + (STRIDE / 2);
        enemies[i]->y = enemies[i]->init_y = WINDOW_HEIGHT * 7 / 10;
        printf("INFO : A red enemy was created in position (%zu, %zu)\n", enemies[i]->x, enemies[i]->y);
    }
    return enemies;
}

void delete_enemies(Object **enemies, size_t number_of_enemies)
{
    for (size_t i = 0; i < number_of_enemies; i++)
        delete_object(enemies[i]);
    free(enemies);
}

//==========Main==========//
int main()
{
    init_glfw();
    GLFWwindow *window = create_window();
    if (!window)
        return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "ERROR: Failed to initialize GLAD\n");
        return -1;
    }

    glClearColor(1, 0, 0, 1);

    uint32_t *pixels = malloc(sizeof(uint32_t) * WINDOW_HEIGHT * WINDOW_WIDTH);
    pixels_clear(pixels, WINDOW_HEIGHT * WINDOW_WIDTH, 0);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    uint32_t shader = compile_shader("resources/pixel_vertex.glsl", "resources/pixel_fragment.glsl");
    glUseProgram(shader);

    GLuint location = glGetUniformLocation(shader, "pixels");
    glUniform1i(location, 0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(vao);

    init_player_object();

    Object **green_enemies = create_green_enemies();
    Object **red_enemies   = create_red_enemies();

    initialize_fires();

    glfwSetFramebufferSizeCallback(window, frame_buffer_callback);

    while (!glfwWindowShouldClose(window)) {
        pixels_clear(pixels, WINDOW_HEIGHT * WINDOW_WIDTH, 0x181818FF);

        check_player_action(window, &PLAYER_OBJECT, pixels);
        draw_object(pixels, &PLAYER_OBJECT, 0xFFFFFFFF);

        moving_fires();
        for (size_t i = 0; i < MAX_FIRES; i++)
            if (player_fires[i] != NULL)
                draw_object(pixels, player_fires[i], 0xFFFFFFFF);

        double curr_time = glfwGetTime();
        for (size_t i = 0; i < NUMBER_OF_GREEN_ENEMIES_IN_ROW; i++) {
            moving_enemy_animation(green_enemies[i], curr_time);
            draw_object(pixels, green_enemies[i], 0x31EDEEFF);
        }
        for (size_t i = 0; i < NUMBER_OF_RED_ENEMIES_IN_ROW; i++) {
            moving_enemy_animation(red_enemies[i], curr_time);
            draw_object(pixels, red_enemies[i], 0xEB1A40FF);
        }

        glTexSubImage2D(
            GL_TEXTURE_2D,
            0, 0, 0,
            WINDOW_WIDTH, WINDOW_HEIGHT,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8,
            pixels);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    glDeleteVertexArrays(1, &vao);

    free(pixels);
    free(PLAYER_OBJECT.sprite->data);
    delete_enemies(green_enemies, NUMBER_OF_GREEN_ENEMIES_IN_ROW);

    return 0;
}