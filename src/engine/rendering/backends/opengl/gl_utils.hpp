#pragma once

#include "../glad/include/glad/gl.h"

#include "engine/debugging/logger.hpp"

namespace Pulse::Engine::Rendering{

    inline const char* GLErrorToString(GLenum err){
        switch(err){
            case GL_INVALID_ENUM:                  return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:                 return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:             return "GL_INVALID_OPERATION";
            case GL_INVALID_FRAMEBUFFER_OPERATION:  return "GL_INVALID_FRAMEBUFFER_OPERATION";
            case GL_OUT_OF_MEMORY:                 return "GL_OUT_OF_MEMORY";
            case GL_STACK_UNDERFLOW:               return "GL_STACK_UNDERFLOW";
            case GL_STACK_OVERFLOW:                return "GL_STACK_OVERFLOW";
            default:                                return "Unknown GL error";
        }
    }

    // Drains and logs every pending GL error, tagged with a caller-provided context string.
    inline void CheckGLError(const char* context){
        GLenum err;
        while((err = glGetError()) != GL_NO_ERROR){
            DEBUG_ERROR("GL error after ", context, " : ", GLErrorToString(err), " (", err, ")");
        }
    }

}