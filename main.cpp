#include <SDL.h>
#include <glad/gl.h>

#include <SDL_opengl.h>
#include <as-camera-input-sdl/as-camera-input-sdl.hpp>
#include <as/as-view.hpp>

#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_sdl.h"

#include <chrono>
#include <cstdint>
#include <iostream>

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

const char* const g_reverse_z_screen_depth_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float near;
uniform float far;

// return depth value in range near to far
float LinearizeDepth(in vec2 uv)
{
  float depth = texture(screenTexture, uv).x;
  // inverse of perspective projection matrix transformation
  return near * far / (far - depth * (far - near));
}

void main()
{
  float c = LinearizeDepth(TexCoords);
  vec3 range = vec3(c - near)/(far - near); // convert to [0,1]
  FragColor = vec4(range, 1.0);
})";

const char* const g_normal_screen_depth_fragment_shader_source =
  R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float near;
uniform float far;

// return depth value in range near to far
// ref: https://learnopengl.com/Advanced-OpenGL/Depth-testing
float LinearizeDepth(in vec2 uv)
{
  float depth = texture(screenTexture, uv).x;
  // map from [0,1] to [-1,1]
  float z_n = 2.0 * depth - 1.0;
  // inverse of perspective projection matrix transformation
  return 2.0 * near * far / (far + near - z_n * (far - near));
}

void main()
{
  float c = LinearizeDepth(TexCoords);
  vec3 range = vec3(c - near)/(far - near); // convert to [0,1]
  FragColor = vec4(range, 1.0);
})";

using fp_seconds = std::chrono::duration<float, std::chrono::seconds::period>;

enum class render_mode_e
{
  color,
  depth
};

enum class depth_mode_e
{
  normal,
  reverse
};

enum class layout_mode_e
{
  near,
  fighting
};

depth_mode_e g_depth_mode = depth_mode_e::normal;
render_mode_e g_render_mode = render_mode_e::depth;
layout_mode_e g_layout_mode = layout_mode_e::near;

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
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
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
  SDL_GL_MakeCurrent(window, context);
  SDL_GL_SetSwapInterval(1); // enable vsync

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
  const uint32_t normal_depth_screen_shader_program = create_shader(
    g_screen_vertex_shader_source,
    g_normal_screen_depth_fragment_shader_source);
  const uint32_t reverse_z_depth_screen_shader_program = create_shader(
    g_screen_vertex_shader_source,
    g_reverse_z_screen_depth_fragment_shader_source);

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
  asc::Camera target_camera = camera;

  asci::CameraSystem camera_system;
  asci::TranslateCameraInput translate_camera{
    asci::lookTranslation, asci::translatePivot};
  asci::RotateCameraInput rotate_camera{asci::MouseButton::Right};
  camera_system.cameras_.addCamera(&translate_camera);
  camera_system.cameras_.addCamera(&rotate_camera);

  float near = 5.0f;
  float far = 100.0f;

  ImGui::CreateContext();

  ImGui_ImplSDL2_InitForOpenGL(window, context);
  ImGui_ImplOpenGL3_Init();

  layout_mode_e prev_layout_mode = g_layout_mode;
  auto prev = std::chrono::system_clock::now();
  for (bool quit = false; !quit;) {
    for (SDL_Event current_event; SDL_PollEvent(&current_event) != 0;) {
      ImGui_ImplSDL2_ProcessEvent(&current_event);
      if (current_event.type == SDL_QUIT) {
        quit = true;
        break;
      }
      camera_system.handleEvents(asci_sdl::sdlToInput(&current_event));
    }

    auto now = std::chrono::system_clock::now();
    auto delta = now - prev;
    prev = now;

    const float delta_time =
      std::chrono::duration_cast<fp_seconds>(delta).count();

    target_camera = camera_system.stepCamera(target_camera, delta_time);
    camera = asci::smoothCamera(
      camera, target_camera, asci::SmoothProps{}, delta_time);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glEnable(GL_DEPTH_TEST);
    if (g_depth_mode == depth_mode_e::reverse) {
      glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
      glClearDepth(0.0f);
      glDepthFunc(GL_GREATER);
    } else if (g_depth_mode == depth_mode_e::normal) {
      glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
      glClearDepth(1.0f);
      glDepthFunc(GL_LESS);
    }

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(main_shader_program);

    const as::mat4 perspective_projection = as::perspective_opengl_rh(
      as::radians(60.0f), float(width) / float(height), near, far);
    const as::mat4 reverse_z_perspective_projection =
      as::reverse_z(as::normalize_unit_range(perspective_projection));

    if (g_layout_mode != prev_layout_mode) {
      if (g_layout_mode == layout_mode_e::fighting) {
        near = 0.01f;
        far = 10000.0f;
      } else if (g_layout_mode == layout_mode_e::near) {
        near = 5.0f;
        far = 100.0f;
      }
      prev_layout_mode = g_layout_mode;
    }

    const as::mat4 view_projection = [camera, perspective_projection,
                                      reverse_z_perspective_projection] {
      const as::mat4 view = as::mat4_from_affine(camera.view());
      if (g_depth_mode == depth_mode_e::normal) {
        return as::mat_mul(view, perspective_projection);
      }
      if (g_depth_mode == depth_mode_e::reverse) {
        return as::mat_mul(view, reverse_z_perspective_projection);
      }
      return as::mat4::identity();
    }();

    const uint32_t mvp_loc = glGetUniformLocation(main_shader_program, "mvp");
    const uint32_t color_loc =
      glGetUniformLocation(main_shader_program, "color");

    switch (g_layout_mode) {
      case layout_mode_e::fighting: {
        draw_quad(
          view_projection,
          as::mat4_from_mat3_vec3(
            as::mat3_scale(100.0f, 100.0f, 1.0),
            as::vec3(-10.0f, 25.0f, -500.02f)),
          as::vec4(1.0f, 0.5f, 0.2f, 1.0f), mvp_loc, color_loc, vao);
        draw_quad(
          view_projection,
          as::mat4_from_mat3_vec3(
            as::mat3_scale(100.0f, 100.0f, 1.0),
            as::vec3(10.0f, -25.0f, -499.98f)),
          as::vec4(1.0f, 0.0f, 0.0f, 1.0f), mvp_loc, color_loc, vao);
        draw_quad(
          view_projection,
          as::mat4_from_mat3_vec3(
            as::mat3_scale(100.0f, 100.0f, 1.0),
            as::vec3(-10.0f, 0.0f, -500.0f)),
          as::vec4(0.1f, 0.2f, 0.6f, 1.0f), mvp_loc, color_loc, vao);
        draw_quad(
          view_projection,
          as::mat4_from_mat3_vec3(
            as::mat3_scale(100.0f, 100.0f, 1.0),
            as::vec3(10.0f, 0.0f, -500.01f)),
          as::vec4(0.1f, 0.8f, 0.2f, 1.0f), mvp_loc, color_loc, vao);
      } break;
      case layout_mode_e::near: {
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
      } break;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_DEPTH_TEST);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (g_render_mode == render_mode_e::color) {
      glUseProgram(screen_shader_program);
    } else {
      const int depth_shader_program = [reverse_z_depth_screen_shader_program,
                                        normal_depth_screen_shader_program] {
        if (g_depth_mode == depth_mode_e::normal) {
          return normal_depth_screen_shader_program;
        }
        if (g_depth_mode == depth_mode_e::reverse) {
          return reverse_z_depth_screen_shader_program;
        }
        return uint32_t(0);
      }();
      glUseProgram(depth_shader_program);
      const uint32_t near_loc =
        glGetUniformLocation(depth_shader_program, "near");
      glUniform1f(near_loc, near);
      const uint32_t far_loc =
        glGetUniformLocation(depth_shader_program, "far");
      glUniform1f(far_loc, far);
    }

    glBindVertexArray(quad_vao);

    if (g_render_mode == render_mode_e::color) {
      glBindTexture(GL_TEXTURE_2D, texture_colorbuffer);
    } else {
      glBindTexture(GL_TEXTURE_2D, texture_depth_stencil_buffer);
    }

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glUseProgram(main_shader_program);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    {
      int depth_mode_index = static_cast<int>(g_depth_mode);
      const char* depth_mode_names[] = {"Normal", "Reverse"};
      ImGui::Combo(
        "Depth Mode", &depth_mode_index, depth_mode_names,
        std::size(depth_mode_names));
      g_depth_mode = static_cast<depth_mode_e>(depth_mode_index);
    }

    {
      int render_mode_index = static_cast<int>(g_render_mode);
      const char* render_mode_names[] = {"Color", "Depth"};
      ImGui::Combo(
        "Render Mode", &render_mode_index, render_mode_names,
        std::size(render_mode_names));
      g_render_mode = static_cast<render_mode_e>(render_mode_index);
    }

    {
      int layout_mode_index = static_cast<int>(g_layout_mode);
      const char* layout_mode_names[] = {"Near", "Fighting"};
      ImGui::Combo(
        "Layout Mode", &layout_mode_index, layout_mode_names,
        std::size(layout_mode_names));
      g_layout_mode = static_cast<layout_mode_e>(layout_mode_index);
    }

    ImGui::SliderFloat("Near Plane", &near, 0.01f, 49.9f);
    ImGui::SliderFloat("Far Plane", &far, 50.0f, 10000.0f);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    SDL_GL_SwapWindow(window);
  }

  glDeleteVertexArrays(1, &vao);
  glDeleteVertexArrays(1, &quad_vao);
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &quad_vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteProgram(main_shader_program);
  glDeleteProgram(screen_shader_program);
  glDeleteProgram(reverse_z_depth_screen_shader_program);
  glDeleteProgram(normal_depth_screen_shader_program);
  glDeleteTextures(1, &texture_colorbuffer);
  glDeleteTextures(1, &texture_depth_stencil_buffer);
  glDeleteFramebuffers(1, &framebuffer);

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
