#include <cstdint>
#include <iostream>

#include <SDL.h>
#include <glad/gl.h>

#include <SDL_opengl.h>

#include <as-camera-input/as-camera-input.hpp>
#include <as/as-view.hpp>

const char* const g_vertex_shader_source =
  R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 mvp;
void main()
{
  gl_Position = mvp * vec4(aPos, 1.0);
})";
const char* const g_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;
uniform vec4 color;
void main()
{
  FragColor = color;
})";

const char* const g_screen_vertex_shader_source =
  R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
    TexCoords = aTexCoords;
})";

const char* const g_screen_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    FragColor = texture(screenTexture, TexCoords);
})";

namespace asc
{

Handedness handedness()
{
  return Handedness::Right;
}

} // namespace asc

uint32_t create_shader(
  const char* vertex_shader_source, const char* fragment_shader_source)
{
  constexpr int info_log_size = 512;
  char info_log[info_log_size];
  info_log[0] = '\0';

  uint32_t vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  int vertex_shader_success;
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &vertex_shader_success);
  if (!vertex_shader_success) {
    glGetShaderInfoLog(vertex_shader, info_log_size, NULL, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << info_log << '\0';
  }

  uint32_t fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
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

  return shader_program;
}

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
  if (version == 0) {
    printf("Failed to initialize OpenGL context\n");
    return 1;
  }

  printf(
    "OpenGL version %d.%d\n", GLAD_VERSION_MAJOR(version),
    GLAD_VERSION_MINOR(version));

  const uint32_t main_shader_program =
    create_shader(g_vertex_shader_source, g_fragment_shader_source);
  const uint32_t screen_shader_program = create_shader(
    g_screen_vertex_shader_source, g_screen_fragment_shader_source);

  float vertices[] = {
    0.5f,  0.5f,  0.0f, // top right
    0.5f,  -0.5f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f, // bottom left
    -0.5f, 0.5f,  0.0f // top left
  };

  float screen_vertices[] = {
    -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top left
    1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top right
    1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -1.0f, 1.0f,  0.0f, 0.0f, 1.0f, // top left
    1.0f,  -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
    -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
  };

  uint32_t indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3 // second triangle
  };

  uint32_t quad_vbo, quad_vao;
  glGenVertexArrays(1, &quad_vao);
  glGenBuffers(1, &quad_vbo);

  glBindVertexArray(quad_vao);
  glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
  glBufferData(
    GL_ARRAY_BUFFER, sizeof(screen_vertices), screen_vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(
    1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

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

  uint32_t framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

  uint32_t texture_colorbuffer;
  glGenTextures(1, &texture_colorbuffer);
  glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_colorbuffer,
    0);
  glBindTexture(GL_TEXTURE_2D, 0);

  uint32_t rbo;
  glGenRenderbuffers(1, &rbo);
  glBindRenderbuffer(GL_RENDERBUFFER, rbo);

  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
  glFramebufferRenderbuffer(
    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!"
              << std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glViewport(0, 0, width, height);

  asc::Camera camera;
  camera.pivot = as::vec3(0.0f, 0.0f, 2.0f);

  const as::mat4 perspective_projection = as::perspective_gl_rh(
    as::radians(60.0f), float(width) / float(height), 0.01f, 100.0f);

  glDepthFunc(GL_LESS);

  for (bool quit = false; !quit;) {
    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(main_shader_program);

    const as::mat4 view = as::mat4_from_affine(camera.view());
    const as::mat4 view_projection = perspective_projection * view;

    const as::mat4 translation_left =
      as::mat4_from_vec3(as::vec3(-0.25f, 0.25f, -1.0f));
    const as::mat4 translation_right =
      as::mat4_from_vec3(as::vec3(0.25f, -0.25f, -3.0f));

    const uint32_t mvp_loc = glGetUniformLocation(main_shader_program, "mvp");
    const as::mat4 model_view_projection_l = view_projection * translation_left;
    glUniformMatrix4fv(
      mvp_loc, 1, GL_FALSE, as::mat_const_data(model_view_projection_l));

    const uint32_t color_loc =
      glGetUniformLocation(main_shader_program, "color");
    const as::vec4 color_l = as::vec4(1.0f, 0.5f, 0.2f, 1.0f);
    glUniform4fv(color_loc, 1, as::vec_const_data(color_l));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    const as::mat4 model_view_projection_r =
      view_projection * translation_right;
    glUniformMatrix4fv(
      mvp_loc, 1, GL_FALSE, as::mat_const_data(model_view_projection_r));

    const as::vec4 color_r = as::vec4(1.0f, 0.2f, 0.1f, 1.0f);
    glUniform4fv(color_loc, 1, as::vec_const_data(color_r));

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(screen_shader_program);

    glBindVertexArray(quad_vao);
    glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    SDL_GL_SwapWindow(window);
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteVertexArrays(1, &quad_vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &quad_vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteProgram(main_shader_program);
  glDeleteProgram(screen_shader_program);

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
