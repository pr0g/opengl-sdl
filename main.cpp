#include <cstdint>
#include <iostream>

#include <SDL.h>
#include <glad/gl.h>

#include <SDL_opengl.h>

const char* const g_vertex_shader_source =
  R"(#version 330 core
layout (location = 0) in vec3 aPos;
void main()
{
  gl_Position = vec4(aPos, 1.0);
})";
const char* const g_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;
void main()
{
  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
})";

int main(int argc, char** argv)
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  const int width = 1024;
  const int height = 768;
  const float aspect = float(width) / float(height);
  SDL_Window* window = SDL_CreateWindow(
    argv[0], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

  if (window == nullptr) {
    printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    return 1;
  }

  const SDL_GLContext context = SDL_GL_CreateContext(window);
  const int version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
  printf(
    "OpenGL version %d.%d\n", GLAD_VERSION_MAJOR(version),
    GLAD_VERSION_MINOR(version));

  constexpr int info_log_size = 512;
  char info_log[info_log_size];
  info_log[0] = '\0';

  uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &g_vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  int vertex_shader_success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_success);
  if (!vertex_shader_success) {
    glGetShaderInfoLog(vertex_shader, info_log_size, NULL, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << info_log << '\0';
  }

  uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &g_fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  int fragment_shader_success;
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &fragment_shader_success);
  if (!fragment_shader_success) {
    glGetShaderInfoLog(fragment_shader, info_log_size, NULL, info_log);
    std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
              << info_log << '\0';
  }

  uint32_t shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  glLinkProgram(shader_program);
  int shader_program_success;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &shader_program_success);
  if (!shader_program_success) {
    glGetProgramInfoLog(shader_program, info_log_size, NULL, info_log);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << info_log << '\0';
  }

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  float vertices[] = {
    0.5f,  0.5f,  0.0f, // top right
    0.5f,  -0.5f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, // bottom left
    -0.5f, 0.5f,  0.0f // top left
  };

  uint32_t indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3 // second triangle
  };

  uint32_t vbo, vao, ebo;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(
    GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  glViewport(0, 0, width, height);

  for (bool quit = false; !quit;) {
    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SDL_GL_SwapWindow(window);
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteProgram(shader_program);

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
