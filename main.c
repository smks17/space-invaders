#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <glad/glad.h>
#include <glfw/glfw3.h>


#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600
#define WINDOW_TITLE  "space invaders"


void init_glfw() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    printf("INFO : glfw initilize!\n");
}

GLFWwindow* create_window() {
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


const char *vertex_shader_source = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 transform;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = transform * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragment_shader_source = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

unsigned int compile_shader() {
    int success;
    char msg[512];
    // vertex shader
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
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


typedef struct {
        float data[16];
} Matrix4;

static const Matrix4 IDENTITY_MATRIX = {{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
}};

const Matrix4 multiply_matrix4(Matrix4* mat1, Matrix4* m2) {
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

void translate(Matrix4* m, float x, float y, float z) {
        Matrix4 translation = IDENTITY_MATRIX;
        translation.data[12] = x;
        translation.data[13] = y;
        translation.data[14] = z;
        memcpy(m->data, multiply_matrix4(m, &translation).data, sizeof(m->data));
}


float vertex = 0, horizon = 0;

#define SPEED 0.001f

void process_input(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    else if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        if (vertex < 1.0f) vertex += SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        if (vertex > -1.0f) vertex -= SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        if (horizon > -1.0f) horizon -= SPEED;
    }
    else if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        if (horizon < 1.0f) horizon += SPEED;
    }
}


int main () {
    init_glfw();

    GLFWwindow* window = NULL;
    window = create_window(window);
    if (!window) return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    unsigned int shader_program = compile_shader();

    float vertices[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
        0.5f ,  0.5f, 0.0f,
        0.5f , -0.5f, 0.0f,
    };

    unsigned int indices[] = {
        0, 1, 2,
        1, 2, 3
    };

    unsigned int VAO;
    unsigned int vertices_buffer;
    unsigned int EBO;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &vertices_buffer);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, vertices_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    /* glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); */
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window)) {
        process_input(window);

        glClearColor(0.18f, 0.18f, 0.18f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        Matrix4 transform = IDENTITY_MATRIX;
        translate(&transform, horizon, vertex, 0.0f);

        glUseProgram(shader_program);
        unsigned int transform_loc = glGetUniformLocation(shader_program, "transform");
        glUniformMatrix4fv(transform_loc, 1, GL_FALSE, &(transform.data[0]));

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &vertices_buffer);
    glDeleteBuffers(1, &EBO);

    glDeleteProgram(shader_program);

    glfwTerminate();

    printf("Finish Program!");
    return 0;
}
