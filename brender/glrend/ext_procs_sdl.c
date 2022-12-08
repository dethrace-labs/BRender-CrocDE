#include "drv.h"
#include <SDL_render.h>

static void *sdl_gl_create_context(struct br_device *dev, void *user)
{
    void *ctx;

    (void)dev;

    if((ctx = SDL_GL_CreateContext((SDL_Window *)user)) == NULL) {
        BrLogError("SDL", "OpenGL context creation failed: %s", SDL_GetError());
        return NULL;
    }

    return ctx;
}

static void sdl_gl_delete_context(struct br_device *dev, void *ctx, void *user)
{
    (void)dev;
    (void)user;
    SDL_GL_DeleteContext(ctx);
}

static br_error sdl_gl_make_current(struct br_device *dev, void *ctx, void *user)
{
    (void)dev;

    if(SDL_GL_MakeCurrent((SDL_Window *)user, (SDL_GLContext)ctx) < 0) {
        BrLogError("SDL", "OpenGL MakeCurrent failed: %s", SDL_GetError());
        return BRE_FAIL;
    }

    return BRE_OK;
}


static void *sdl_gl_get_proc_address(const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

static void sdl_gl_swap_buffers(struct br_device *dev, struct br_device_pixelmap *pm, void *user)
{
    (void)dev;
    (void)pm;
    SDL_GL_SwapWindow((SDL_Window *)user);
}

const br_device_gl_ext_procs sdl_procs = {
    .create_context   = sdl_gl_create_context,
    .delete_context   = sdl_gl_delete_context,
    .make_current     = sdl_gl_make_current,
    .get_proc_address = sdl_gl_get_proc_address,
    .swap_buffers     = sdl_gl_swap_buffers,
    .preswap_hook     = NULL,
};
