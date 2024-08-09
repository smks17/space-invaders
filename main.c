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

//==========Animation==========//
typedef struct
{
    bool loop;
    Sprite **frames;
    size_t number_of_frames;
    double frame_duration;
    double start_time; // start animation
    double time;       // time spent in animation playing from frame 0
} Anim;

//==========Object==========//
typedef struct {
    Sprite *curr_sprite;
    long double x, y;
    size_t init_x, init_y;
    uint32_t color;
    Anim **animations;
    size_t number_of_animations;
} Object;

#define pixels(col, row) pixels[((col) * (WINDOW_WIDTH)) + (row)]

void draw_object(uint32_t *pixels, Object *obj)
{
    const size_t left_up_x = obj->x - (obj->curr_sprite->width / 2);
    const size_t left_up_y = obj->y - (obj->curr_sprite->height / 2);
    for (size_t ys = 0; ys < obj->curr_sprite->height; ys++) {
        for (size_t xs = 0; xs < obj->curr_sprite->width; xs++) {
            size_t col = (ys + left_up_y);
            size_t row = xs + left_up_x;
            if ((col < WINDOW_HEIGHT) & (row < WINDOW_WIDTH) & (obj->curr_sprite->data[xs + ((obj->curr_sprite->height - ys - 1) * (obj->curr_sprite->width))]))
                pixels(col, row) = obj->color;
        }
    }
}

void delete_object(Object *obj)
{
    free(obj->curr_sprite);
    free(obj);
}

bool check_collision(Object *obj, Object **fires, size_t number_of_fires)
{
    size_t up_obj    = obj->y + (obj->curr_sprite->height / 2);
    size_t down_obj  = obj->y - (obj->curr_sprite->height / 2);
    size_t right_obj = obj->x + (obj->curr_sprite->width / 2);
    size_t left_obj  = obj->x - (obj->curr_sprite->width / 2);
    for (size_t i = 0; i < number_of_fires; i++) {
        if (fires[i] == NULL)
            continue;
        size_t up_fire    = fires[i]->y + (fires[i]->curr_sprite->height / 2);
        size_t down_fire  = fires[i]->y - (fires[i]->curr_sprite->height / 2);
        size_t left_fire  = fires[i]->x - (fires[i]->curr_sprite->width / 2);
        size_t right_fire = fires[i]->x + (fires[i]->curr_sprite->width / 2);

        // TODO: also check curr_sprite data
        if (((up_fire < up_obj && up_fire > down_obj) || (down_fire < up_obj && down_fire > down_obj)) && ((left_fire > right_obj && left_fire < left_obj) || (right_fire < right_obj && right_fire > left_obj))) {
            delete_object(fires[i]);
            fires[i] = NULL;
            return true;
        }
    }
    return false;
}

void play_object_animation(Object *obj, Anim *anim)
{
    if ((anim->number_of_frames * anim->frame_duration) < anim->time) {
        if (!anim->loop)
            return;
        else if (anim->loop) {
            anim->time       = 0.0;
            anim->start_time = glfwGetTime();
        }
    }
    size_t index     = (size_t)(anim->time / anim->frame_duration);
    obj->curr_sprite = anim->frames[index];
    anim->time       = (glfwGetTime() - anim->start_time);
}

//==========Player==========//
#define PLAYER_SPRITE_WIDTH  11
#define PLAYER_SPRITE_HEIGHT 7

const static uint8_t player_sprite_data[] = {
    0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, // .....@.....
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
    0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, // ....@@@....
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@@@@.
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@
};

static Object PLAYER_OBJECT;

void init_player_object()
{
    PLAYER_OBJECT.curr_sprite       = create_new_sprite(PLAYER_SPRITE_WIDTH, PLAYER_SPRITE_HEIGHT);
    PLAYER_OBJECT.curr_sprite->data = player_sprite_data;
    PLAYER_OBJECT.x = PLAYER_OBJECT.init_x = WINDOW_WIDTH / 2;
    PLAYER_OBJECT.y = PLAYER_OBJECT.init_y = WINDOW_HEIGHT / 5;
    PLAYER_OBJECT.color                    = 0xFFFFFFFF;
    PLAYER_OBJECT.animations               = NULL;
    PLAYER_OBJECT.number_of_animations     = 0;
}

//==========Player Action==========//
#define MAX_PLAYER_FIRES 20
Object *player_fires[MAX_PLAYER_FIRES];

#define MAX_ENEMY_FIRES 50
Object *enemy_fires[MAX_ENEMY_FIRES];

void initialize_fires()
{
    for (int i = 0; i < MAX_PLAYER_FIRES; i++)
        player_fires[i] = NULL;
    for (int i = 0; i < MAX_ENEMY_FIRES; i++)
        enemy_fires[i] = NULL;
}

#define PLAYER_FIRE_SPEED  0.3
#define PLAYER_ENEMY_SPEED 0.1

void moving_fires()
{
    for (int i = 0; i < MAX_PLAYER_FIRES; i++) {
        if (player_fires[i] != NULL) {
            player_fires[i]->y += PLAYER_FIRE_SPEED;
            if (player_fires[i]->y >= WINDOW_HEIGHT) {
                free(player_fires[i]);
                player_fires[i] = NULL;
            }
        }
    }
    for (int i = 0; i < MAX_ENEMY_FIRES; i++) {
        if (enemy_fires[i] != NULL) {
            enemy_fires[i]->y -= PLAYER_ENEMY_SPEED;
            if (enemy_fires[i]->y <= 0) {
                free(enemy_fires[i]);
                enemy_fires[i] = NULL;
            }
        }
    }
}

void spawn_player_fire(Object *player, uint32_t *pixels)
{
    Object *fire          = malloc(sizeof(Object));
    fire->curr_sprite          = create_new_sprite(1, 3);
    fire->curr_sprite->data    = malloc(sizeof(uint8_t) * 3);
    fire->curr_sprite->data[0] = 1;
    fire->curr_sprite->data[1] = 1;
    fire->curr_sprite->data[2] = 1;
    fire->curr_sprite->x = fire->x = player->x;
    fire->curr_sprite->y = fire->y = player->y;
    fire->color = player->color;
    for (size_t i = 0; i < MAX_PLAYER_FIRES; i++) {
        if (player_fires[i] == NULL) {
            player_fires[i] = fire;
            return;
        }
    }
    fprintf(stderr, "ERROR: Could not spawn a new fire anymore\n");
}

#define PLAYER_SPEED          0.2f
#define PLAYER_FIRE_RATE_TIME 0.4f

void check_player_action(GLFWwindow *window, Object *player, uint32_t *pixels)
{
    static double last_spawn_fire = 0;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (player->x + PLAYER_SPEED < WINDOW_WIDTH)
            player->x += PLAYER_SPEED;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (player->x - PLAYER_SPEED >= 0)
            player->x -= PLAYER_SPEED;
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
void check_to_spawn_enemy_fires(Object **enemies, size_t number_of_emmies)
{
    if ((rand() % 10000) == 0) {
        Object *fire          = malloc(sizeof(Object));
        fire->curr_sprite          = create_new_sprite(1, 3);
        fire->curr_sprite->data    = malloc(sizeof(uint8_t) * 2);
        fire->curr_sprite->data[0] = 1;
        fire->curr_sprite->data[1] = 1;
        fire->curr_sprite->data[2] = 1;

        int index = 0;
        int count = 0;
        do {
            index = rand() % number_of_emmies;
            count++;
        } while (enemies[index] == NULL || (count < (number_of_emmies * 2)));

        if (enemies[index] == NULL)
            return;

        Object *enemy   = enemies[index];
        fire->curr_sprite->x = fire->x = enemy->x;
        fire->curr_sprite->y = fire->y = enemy->y;
        fire->color     = enemy->color;
        for (size_t i = 0; i < MAX_ENEMY_FIRES; i++) {
            if (enemy_fires[i] == NULL) {
                enemy_fires[i] = fire;
                return;
            }
        }
        fprintf(stderr, "ERROR: Could not spawn a new fire anymore\n");
    }
}

#define ENEMY_SPEED 2

inline void moving_enemy_animation(Object *enemy, double curr_time)
{
    enemy->x = enemy->init_x + (int)(sin(curr_time * ENEMY_SPEED) * (WINDOW_WIDTH / 16));
}

#define NUMBER_OF_GREEN_ENEMIES_IN_ROW 8
#define GREEN_ENEMY_WIDTH              12
#define GREEN_ENEMY_HEIGHT             8
#define GREEN_ENEMY_ANIMATION_FRAMES   2
#define GREEN_ENEMY_FRAME_DURATION     0.2

const static uint8_t green_enemy_frames[GREEN_ENEMY_ANIMATION_FRAMES][GREEN_ENEMY_WIDTH * GREEN_ENEMY_HEIGHT] = {
    {
        0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, // ..@......@..
        0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, // ...@....@...
        0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, // ..@@@@@@@@..
        0, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, // .@@.@@@@.@@.
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@@
        1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, // @.@@@@@@@@.@
        1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, // @.@......@.@
        0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0  // ...@@..@@...
    },
    {
        0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, // ..@......@..
        1, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 1, // @..@....@..@
        1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, // @.@@@@@@@@.@
        1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, // @@@.@@@@.@@@
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@@@@@
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@@@@@.
        0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, // ..@......@..
        0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0  // .@........@.
    }
};

Object **create_green_enemies()
{
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
        enemies[i]->x = enemies[i]->init_x = (i * STRIDE) + (WINDOW_WIDTH / 8) + (STRIDE / 2);
        enemies[i]->y = enemies[i]->init_y = WINDOW_HEIGHT * 8 / 10;
        enemies[i]->color                  = 0x31EDEEFF;

        enemies[i]->number_of_animations  = 1;
        enemies[i]->animations            = malloc(1 * sizeof(Anim));
        enemies[i]->animations[0]         = malloc(sizeof(Anim));
        enemies[i]->animations[0]->frames = malloc(GREEN_ENEMY_ANIMATION_FRAMES * sizeof(Sprite));
        for (size_t j = 0; j < GREEN_ENEMY_ANIMATION_FRAMES; j++) {
            enemies[i]->animations[0]->frames[j]       = create_new_sprite(GREEN_ENEMY_WIDTH, GREEN_ENEMY_HEIGHT);
            enemies[i]->animations[0]->frames[j]->data = green_enemy_frames[j];
        }
        enemies[i]->animations[0]->loop             = true;
        enemies[i]->animations[0]->frame_duration   = GREEN_ENEMY_FRAME_DURATION;
        enemies[i]->animations[0]->number_of_frames = GREEN_ENEMY_ANIMATION_FRAMES;
        enemies[i]->animations[0]->time             = 0;
        enemies[i]->animations[0]->start_time       = 0;
        enemies[i]->curr_sprite                     = enemies[i]->animations[0]->frames[0];
        printf("INFO : A green enemy was created in position (%zu, %zu)\n", (size_t)enemies[i]->x, (size_t)enemies[i]->y);
    }
    return enemies;
}

#define NUMBER_OF_RED_ENEMIES_IN_ROW 8
#define RED_ENEMY_WIDTH              8
#define RED_ENEMY_HEIGHT             8
#define RED_ENEMY_ANIMATION_FRAMES   2
#define RED_ENEMY_FRAME_DURATION     0.2

const static uint8_t red_enemy_frames[RED_ENEMY_ANIMATION_FRAMES][RED_ENEMY_WIDTH * RED_ENEMY_HEIGHT] = {
    {
        0, 0, 0, 1, 1, 0, 0, 0, // ...@@...
        0, 0, 1, 1, 1, 1, 0, 0, // ..@@@@..
        0, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@.
        1, 1, 0, 1, 1, 0, 1, 1, // @@.@@.@@
        1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@
        0, 1, 0, 1, 1, 0, 1, 0, // .@.@@.@.
        1, 0, 0, 0, 0, 0, 0, 1, // @......@
        0, 1, 0, 0, 0, 0, 1, 0  // .@....@.
    },
    {
        0, 0, 0, 1, 1, 0, 0, 0, // ...@@...
        0, 0, 1, 1, 1, 1, 0, 0, // ..@@@@..
        0, 1, 1, 1, 1, 1, 1, 0, // .@@@@@@.
        1, 1, 0, 1, 1, 0, 1, 1, // @@.@@.@@
        1, 1, 1, 1, 1, 1, 1, 1, // @@@@@@@@
        0, 0, 1, 0, 0, 1, 0, 0, // ..@..@..
        0, 1, 0, 1, 1, 0, 1, 0, // .@.@@.@.
        1, 0, 1, 0, 0, 1, 0, 1  // @.@..@.@
    }
};

Object **create_red_enemies()
{
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
        enemies[i]->x = enemies[i]->init_x = (i * STRIDE) + (WINDOW_WIDTH / 8) + (STRIDE / 2);
        enemies[i]->y = enemies[i]->init_y = WINDOW_HEIGHT * 7 / 10;
        enemies[i]->color                  = 0xEB1A40FF;

        enemies[i]->number_of_animations  = 1;
        enemies[i]->animations            = malloc(1 * sizeof(Anim));
        enemies[i]->animations[0]         = malloc(sizeof(Anim));
        enemies[i]->animations[0]->frames = malloc(RED_ENEMY_ANIMATION_FRAMES * sizeof(Sprite));
        for (size_t j = 0; j < RED_ENEMY_ANIMATION_FRAMES; j++) {
            enemies[i]->animations[0]->frames[j]       = create_new_sprite(RED_ENEMY_WIDTH, RED_ENEMY_HEIGHT);
            enemies[i]->animations[0]->frames[j]->data = red_enemy_frames[j];
        }
        enemies[i]->animations[0]->loop             = true;
        enemies[i]->animations[0]->frame_duration   = RED_ENEMY_FRAME_DURATION;
        enemies[i]->animations[0]->number_of_frames = RED_ENEMY_ANIMATION_FRAMES;
        enemies[i]->animations[0]->time             = 0;
        enemies[i]->animations[0]->start_time       = 0;
        enemies[i]->curr_sprite                     = enemies[i]->animations[0]->frames[0];
        printf("INFO : A red enemy was created in position (%zu, %zu)\n", (size_t)enemies[i]->x, (size_t)enemies[i]->y);
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
        draw_object(pixels, &PLAYER_OBJECT);

        moving_fires();
        for (size_t i = 0; i < MAX_PLAYER_FIRES; i++)
            if (player_fires[i] != NULL)
                draw_object(pixels, player_fires[i]);
        for (size_t i = 0; i < MAX_ENEMY_FIRES; i++)
            if (enemy_fires[i] != NULL)
                draw_object(pixels, enemy_fires[i]);

        double curr_time = glfwGetTime();
        for (size_t i = 0; i < NUMBER_OF_GREEN_ENEMIES_IN_ROW; i++) {
            if (green_enemies[i] != NULL) {
                play_object_animation(green_enemies[i], green_enemies[i]->animations[0]);
                moving_enemy_animation(green_enemies[i], curr_time);
                if (check_collision(green_enemies[i], player_fires, MAX_PLAYER_FIRES)) {
                    delete_object(green_enemies[i]);
                    green_enemies[i] = NULL;
                    continue;
                }
                draw_object(pixels, green_enemies[i]);
            }
        }
        for (size_t i = 0; i < NUMBER_OF_RED_ENEMIES_IN_ROW; i++) {
            if (red_enemies[i] != NULL) {
                play_object_animation(red_enemies[i], red_enemies[i]->animations[0]);
                moving_enemy_animation(red_enemies[i], curr_time);
                if (check_collision(red_enemies[i], player_fires, MAX_PLAYER_FIRES)) {
                    delete_object(red_enemies[i]);
                    red_enemies[i] = NULL;
                    continue;
                }
                draw_object(pixels, red_enemies[i]);
            }
        }
        check_to_spawn_enemy_fires(red_enemies, NUMBER_OF_RED_ENEMIES_IN_ROW);
        check_to_spawn_enemy_fires(green_enemies, NUMBER_OF_GREEN_ENEMIES_IN_ROW);

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
    delete_enemies(green_enemies, NUMBER_OF_GREEN_ENEMIES_IN_ROW);

    return 0;
}