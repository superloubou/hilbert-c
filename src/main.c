#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

float scaleFactor = 1.0f;
float translateVector[] = {0.0f, 0.0f};

int dragging = 0;
double lastX, lastY;

char *loadShaderFromFile(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open shader file: %s\n", path);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *) malloc(length + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    fread(buffer, 1, length, file);
    buffer[length] = '\0';

    fclose(file);
    return buffer;
}

unsigned int compileShader(const char *source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compilation error: %s\n", infoLog);
        exit(EXIT_FAILURE);
    }
    return shader;
}

unsigned int createShaderProgram(const char *vertexSource, const char *fragmentSource) {
    unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        fprintf(stderr, "Shader program linking error: %s\n", infoLog);
        exit(EXIT_FAILURE);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) { glViewport(0, 0, width, height); }

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, 1);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    scaleFactor *= (float) pow(1.1, yoffset);
    scaleFactor = fmaxf(0.25, fminf(scaleFactor, 200.0));
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = 1;
            glfwGetCursorPos(window, &lastX, &lastY);
        } else if (action == GLFW_RELEASE) {
            dragging = 0;
        }
    }
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
    if (dragging) {
        float dx = (float) (x - lastX);
        float dy = (float) (y - lastY);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        dx = dx * 2.0f / width;
        dy = -dy * 2.0f / height;

        translateVector[0] += dx / scaleFactor;
        translateVector[1] += dy / scaleFactor;

        lastX = x;
        lastY = y;
    }
}

float *hilbertCurve(int order) {

    float base[] = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.5f, -0.5f};

    int num_vertices = pow(4, order + 1);
    int num_floats = num_vertices * 2;

    float *vertices = (float *) malloc(num_floats * sizeof(float));
    memcpy(vertices, base, 8 * sizeof(float));

    for (int current_order = 1; current_order <= order; current_order++) {
        int prev_num_floats = pow(4, current_order) * 2;
        int new_num_floats = prev_num_floats * 4;

        for (int i = 1; i < 4; i++) {
            memcpy(&vertices[i * prev_num_floats], vertices, prev_num_floats * sizeof(float));
        }

        for (int i = 0; i < new_num_floats; i += 2) {

            int quarter = i / (new_num_floats / 4);

            float *px = &vertices[i];
            float *py = &vertices[i + 1];

            if (quarter == 0) {
                float temp = *px;
                *px = *py - 1.0;
                *py = temp - 1.0;
            }
            if (quarter == 1) {
                *px -= 1.0;
                *py += 1.0;
            }
            if (quarter == 2) {
                *px += 1.0;
                *py += 1.0;
            }
            if (quarter == 3) {
                float temp = *px;
                *px = -(*py) + 1.0;
                *py = -temp - 1.0;
            }

            *px *= 0.5;
            *py *= 0.5;
        }
    }

    return vertices;
}

float *generate_triangle_strip(float *vertices, int num_floats, float width) {
    int num_new_floats = num_floats * 2;
    float *new_vertices = (float *) malloc(num_new_floats * sizeof(float));

    for (int i = 0; i < num_floats; i += 2) {
        float a[2] = {vertices[i - 2], vertices[i - 1]};
        float b[2] = {vertices[i], vertices[i + 1]};
        float c[2] = {vertices[i + 2], vertices[i + 3]};

        float ba_diff[2] = {0.0f, 0.0f};
        float cb_diff[2] = {0.0f, 0.0f};

        if (i > 0) {
            a[0] = vertices[i - 2];
            a[1] = vertices[i - 1];
            ba_diff[0] = b[0] - a[0];
            ba_diff[1] = b[1] - a[1];
        }

        if (i < num_floats - 2) {
            c[0] = vertices[i + 2];
            c[1] = vertices[i + 3];
            cb_diff[0] = c[0] - b[0];
            cb_diff[1] = c[1] - b[1];
        }

        if (i == 0) {
            vertices[i] -= (cb_diff[0] > 0.0f) * width;
            vertices[i + 1] -= (cb_diff[1] > 0.0f) * width;
        }

        if (i == num_floats - 2) {
            vertices[num_floats - 2] += (ba_diff[0] > 0.0f) * width;
            vertices[num_floats - 1] -= (ba_diff[1] < 0.0f) * width;
        }

        int any_positive_y = 0, any_negative_y = 0;
        int any_positive_x = 0, any_negative_x = 0;

        any_positive_y = (ba_diff[1] > 0.0f) || (cb_diff[1] > 0.0f);
        any_negative_y = (ba_diff[1] < 0.0f) || (cb_diff[1] < 0.0f);

        any_positive_x = (ba_diff[0] > 0.0f) || (cb_diff[0] > 0.0f);
        any_negative_x = (ba_diff[0] < 0.0f) || (cb_diff[0] < 0.0f);

        new_vertices[i * 2 + 0] = vertices[i] + (-width * any_positive_y + width * any_negative_y);
        new_vertices[i * 2 + 1] = vertices[i + 1] + (width * any_positive_x - width * any_negative_x);

        new_vertices[i * 2 + 2] = vertices[i] + (width * any_positive_y - width * any_negative_y);
        new_vertices[i * 2 + 3] = vertices[i + 1] + (-width * any_positive_x + width * any_negative_x);
    }

    return new_vertices;
}

int main() {

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Hilbert Curve", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    char *vertexShaderSource = loadShaderFromFile("./shaders/shader.vs");

    char *fragmentShaderSource = loadShaderFromFile("./shaders/shader.fs");

    unsigned int shaderProgram = createShaderProgram(vertexShaderSource, fragmentShaderSource);

    free(vertexShaderSource);
    free(fragmentShaderSource);

    int order = 3;
    int num_vertices = pow(4, order + 1) * 2;

    float *vertices = hilbertCurve(order);

    float *triangle_strip_vertices = generate_triangle_strip(vertices, num_vertices, 0.25 / pow(2, order));

    glUseProgram(shaderProgram);

    glUniform1i(glGetUniformLocation(shaderProgram, "uNumSegments"), (num_vertices / 2) - 1);

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(float) * 2, triangle_strip_vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(0);

    free(vertices);
    free(triangle_strip_vertices);

    while (!glfwWindowShouldClose(window)) {

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniform1f(glGetUniformLocation(shaderProgram, "uScale"), scaleFactor);
        glUniform2f(glGetUniformLocation(shaderProgram, "uTranslate"), translateVector[0],
                    translateVector[1]);

        int width, height;
        glfwGetWindowSize(window, &width, &height);
        glUniform2f(glGetUniformLocation(shaderProgram, "u_resolution"), (float) width, (float) height);

        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vertices);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}