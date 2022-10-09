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
    gl_Position = vec4(aPos, 1.0);
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

const char* const g_screen_depth_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;

// return depth value in range near to far
// ref: https://learnopengl.com/Advanced-OpenGL/Depth-testing
float LinearizeDepth(in vec2 uv)
{
    float near = 5.0;
    float far  = 100.0;
    float depth = texture(screenTexture, uv).x;
    // map from [0,1] to [-1,1]
    float z_n = 2.0 * depth - 1.0;
    // inverse of perspective projection matrix transformation
    return 2.0 * near * far / (far + near - z_n * (far - near));
}

void main()
{
    float c = LinearizeDepth(TexCoords);
    // convert to [0,1] range by dividing by far plane
    vec3 range = vec3(c - 5.0)/(100.0 - 5.0);
    FragColor = vec4(range, 1.0); // 5.0 is near, 100.0 is far
})";

enum class render_mode_e
{
  normal,
  depth
};

render_mode_e g_render_mode = render_mode_e::depth;

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

void draw_quad(
  const as::mat4& view_projection, const as::mat4& model, const as::vec4& color,
  const uint32_t mvp_loc, const uint32_t color_loc, const uint32_t vao)
{
  const as::mat4 model_view_projection = as::mat_mul(model, view_projection);
  glUniformMatrix4fv(
    mvp_loc, 1, GL_FALSE, as::mat_const_data(model_view_projection));
  glUniform4fv(color_loc, 1, as::vec_const_data(color));
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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
  const uint32_t depth_screen_shader_program = create_shader(
    g_screen_vertex_shader_source, g_screen_depth_fragment_shader_source);

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

  uint32_t texture_colorbuffer;
  glGenTextures(1, &texture_colorbuffer);
  glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);
  glBindTexture(GL_TEXTURE_2D, 0);

  uint32_t texture_depth_stencil_buffer;
  glGenTextures(1, &texture_depth_stencil_buffer);
  glBindTexture(GL_TEXTURE_2D, texture_depth_stencil_buffer);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH32F_STENCIL8, width, height);
  glBindTexture(GL_TEXTURE_2D, 0);

  uint32_t framebuffer;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_colorbuffer,
    0);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
    texture_depth_stencil_buffer, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glViewport(0, 0, width, height);

  asc::Camera camera;
  camera.pivot = as::vec3(0.0f, 0.0f, 4.0f);

  const as::mat4 perspective_projection = as::perspective_gl_rh(
    as::radians(60.0f), float(width) / float(height), 5.0f, 100.0f);

  glDepthFunc(GL_LESS);

  for (bool quit = false; !quit;) {
    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
      if (current_event.type == SDL_KEYDOWN) {
        const auto* keyboard_event = (SDL_KeyboardEvent*)&current_event;
        if (keyboard_event->keysym.scancode == SDL_SCANCODE_R) {
          if (g_render_mode == render_mode_e::depth) {
            g_render_mode = render_mode_e::normal;
          } else {
            g_render_mode = render_mode_e::depth;
          }
        }
        if (keyboard_event->keysym.scancode == SDL_SCANCODE_S) {
          camera.pivot += as::vec3::axis_z(0.1f);
        }
        if (keyboard_event->keysym.scancode == SDL_SCANCODE_W) {
          camera.pivot -= as::vec3::axis_z(0.1f);
        }
      }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(main_shader_program);

    const as::mat4 view = as::mat4_from_affine(camera.view());
    const as::mat4 view_projection = as::mat_mul(view, perspective_projection);

    const uint32_t mvp_loc = glGetUniformLocation(main_shader_program, "mvp");
    const uint32_t color_loc =
      glGetUniformLocation(main_shader_program, "color");

    draw_quad(
      view_projection, as::mat4_from_vec3(as::vec3(-0.25f, 0.25f, -1.0f)),
      as::vec4(1.0f, 0.5f, 0.2f, 1.0f), mvp_loc, color_loc, vao);
    draw_quad(
      view_projection, as::mat4_from_vec3(as::vec3(0.25f, -0.25f, -3.0f)),
      as::vec4(1.0f, 0.0f, 0.0f, 1.0f), mvp_loc, color_loc, vao);
    draw_quad(
      view_projection, as::mat4_from_vec3(as::vec3(-0.25f, 5.5f, -20.0f)),
      as::vec4(0.1f, 0.2f, 0.6f, 1.0f), mvp_loc, color_loc, vao);
    draw_quad(
      view_projection, as::mat4_from_vec3(as::vec3(-30.0f, 0.0f, -80.0f)),
      as::vec4(0.1f, 0.8f, 0.2f, 1.0f), mvp_loc, color_loc, vao);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_render_mode == render_mode_e::normal) {
      glUseProgram(screen_shader_program);
    } else {
      glUseProgram(depth_screen_shader_program);
    }

    glBindVertexArray(quad_vao);

    if (g_render_mode == render_mode_e::normal) {
      glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
    } else {
      glBindTexture(GL_TEXTURE_2D, texture_depth_stencil_buffer);
    }

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
  glDeleteProgram(depth_screen_shader_program);
  glDeleteTextures(1, &texture_colorbuffer);
  glDeleteTextures(1, &texture_depth_stencil_buffer);
  glDeleteFramebuffers(1, &framebuffer);

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
