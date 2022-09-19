#include <iostream>

#include <SDL.h>
#include <glad/gl.h>

#include <SDL_opengl.h>

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

    SDL_GL_SwapWindow(window);
  }

  SDL_GL_DeleteContext(context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  // glViewport(0, 0, 800, 600);
  return 0;
}
