#include <QOpenGLFunctions>
#include <QOpenGLFunctions_4_3_Core>
#include <QOpenGLVersionFunctionsFactory>
#include <QOpenGLContext>
#include "engine/rendering/opengl/opengl.hpp"

static QOpenGLFunctions* g_qt = nullptr;
static QOpenGLFunctions_4_3_Core* g_qt43 = nullptr;


static const GLubyte* Qt_GetString(GLenum name) {
    return g_qt->glGetString(name);
}

static void Qt_Clear(GLbitfield mask) {
    g_qt->glClear(mask);
}

static void Qt_ClearColor(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    g_qt->glClearColor(red, green, blue, alpha);
}

static void Qt_BlendFunc(GLenum sfactor, GLenum dfactor) {
    g_qt->glBlendFunc(sfactor, dfactor);
}

static void Qt_PolygonMode(GLenum face, GLenum mode) {
    g_qt43->glPolygonMode(face, mode);
}

static void Qt_GenerateMipmap(GLenum target) {
    g_qt->glGenerateMipmap(target);
}

static void Qt_UseProgram(GLuint program) {
    g_qt->glUseProgram(program);
}

static GLuint Qt_CreateShader(GLenum shaderType) {
    return g_qt->glCreateShader(shaderType);
}

static void Qt_ShaderSource(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length) {
    g_qt43->glShaderSource(shader, count, string, length);
}

static void Qt_CompileShader(GLuint shader) {
    g_qt->glCompileShader(shader);
}

static GLuint Qt_CreateProgram() {
    return g_qt->glCreateProgram();
}

static void Qt_AttachShader(GLuint program, GLuint shader) {
    g_qt->glAttachShader(program, shader);
}

static void Qt_LinkProgram(GLuint program) {
    g_qt->glLinkProgram(program);
}

static void Qt_DeleteShader(GLuint shader) {
    g_qt->glDeleteShader(shader);
}

static void Qt_GetProgramInfoLog(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog) {
    g_qt->glGetProgramInfoLog(program, maxLength, length, infoLog);
}

static void Qt_GetProgramiv(GLuint program, GLenum pname, GLint* params) {
    g_qt->glGetProgramiv(program, pname, params);
}

static void Qt_GetActiveUniform(GLuint program, GLuint index, GLsizei bufSize, GLsizei* length, GLint* size, GLenum* type, char* name) {
    g_qt->glGetActiveUniform(program, index, bufSize, length, size, type, name);
}

static void Qt_DeleteProgram(GLuint program) {
    g_qt->glDeleteProgram(program);
}

static void Qt_GetShaderiv(GLuint shader, GLenum pname, GLint* params) {
    g_qt->glGetShaderiv(shader, pname, params);
}

static void Qt_GetShaderInfoLog(GLuint shader, GLsizei maxLength, GLsizei* length, char* infoLog) {
    g_qt->glGetShaderInfoLog(shader, maxLength, length, infoLog);
}

static void Qt_PixelStorei(GLenum pname, GLint param) {
    g_qt->glPixelStorei(pname, param);
}

static void Qt_BindTexture(GLenum target, GLuint texture) {
    g_qt->glBindTexture(target, texture);
}

static void Qt_Viewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    g_qt->glViewport(x, y, width, height);
}

static void Qt_DrawArrays(GLenum mode, GLint first, GLsizei count) {
    g_qt->glDrawArrays(mode, first, count);
}

static void Qt_DrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices) {
    g_qt->glDrawElements(mode, count, type, indices);
}

static void Qt_Uniform1i(GLint location, GLint v0) {
    g_qt->glUniform1i(location, v0);
}

static void Qt_Uniform1f(GLint location, GLfloat v0) {
    g_qt->glUniform1f(location, v0);
}

static void Qt_Uniform2fv(GLint location, GLsizei count, const GLfloat* value) {
    g_qt->glUniform2fv(location, count, value);
}

static void Qt_Uniform2f(GLint location, GLfloat v0, GLfloat v1) {
    g_qt->glUniform2f(location, v0, v1);
}

static void Qt_Uniform3fv(GLint location, GLsizei count, const GLfloat* value) {
    g_qt->glUniform3fv(location, count, value);
}

static void Qt_Uniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2) {
    g_qt->glUniform3f(location, v0, v1, v2);
}

static void Qt_Uniform4fv(GLint location, GLsizei count, const GLfloat* value) {
    g_qt->glUniform4fv(location, count, value);
}

static void Qt_Uniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
    g_qt->glUniform4f(location, v0, v1, v2, v3);
}

static void Qt_UniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_qt->glUniformMatrix2fv(location, count, transpose, value);
}

static void Qt_UniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_qt->glUniformMatrix3fv(location, count, transpose, value);
}

static void Qt_UniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value) {
    g_qt->glUniformMatrix4fv(location, count, transpose, value);
}

static GLint Qt_GetUniformLocation(GLuint program, const GLchar* name) {
    return g_qt->glGetUniformLocation(program, name);
}

static void Qt_GenVertexArrays(GLsizei n, GLuint* arrays) {
    g_qt43->glGenVertexArrays(n, arrays);
}

static void Qt_BindVertexArray(GLuint array) {
    g_qt43->glBindVertexArray(array);
}

static void Qt_GenBuffers(GLsizei n, GLuint* buffers) {
    g_qt->glGenBuffers(n, buffers);
}

static void Qt_BindBuffer(GLenum target, GLuint buffer) {
    g_qt->glBindBuffer(target, buffer);
}

static void Qt_BufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage) {
    g_qt->glBufferData(target, size, data, usage);
}

static void Qt_BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data) {
    g_qt->glBufferSubData(target, offset, size, data);
}

static void Qt_BindBufferBase(GLenum target, GLuint index, GLuint buffer) {
    g_qt43->glBindBufferBase(target, index, buffer);
}

static void Qt_EnableVertexAttribArray(GLuint index) {
    g_qt->glEnableVertexAttribArray(index);
}

static void Qt_VertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer) {
    g_qt->glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

static void Qt_DeleteVertexArrays(GLsizei n, const GLuint* arrays) {
    g_qt43->glDeleteVertexArrays(n, arrays);
}

static void Qt_DeleteBuffers(GLsizei n, const GLuint* buffers) {
    g_qt->glDeleteBuffers(n, buffers);
}

static void Qt_GenTextures(GLsizei n, GLuint* textures) {
    g_qt->glGenTextures(n, textures);
}

static void Qt_ActiveTexture(GLenum texture) {
    g_qt->glActiveTexture(texture);
}

static void Qt_TexStorage3D(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth) {
    g_qt43->glTexStorage3D(target, levels, internalFormat, width, height, depth);
}

static void Qt_TexParameteri(GLenum target, GLenum pname, GLint param) {
    g_qt->glTexParameteri(target, pname, param);
}

static void Qt_TexImage2D(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data) {
    g_qt->glTexImage2D(target, level, internalFormat, width, height, border, format, type, data);
}

static void Qt_GenFramebuffers(GLsizei n, GLuint* ids) {
    g_qt->glGenFramebuffers(n, ids);
}

static void Qt_BindFramebuffer(GLenum target, GLuint framebuffer) {
    g_qt->glBindFramebuffer(target, framebuffer);
}

static void Qt_FramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    g_qt->glFramebufferTexture2D(target, attachment, textarget, texture, level);
}

static void Qt_FramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer) {
    g_qt43->glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

static void Qt_DrawBuffer(GLenum mode) {
    g_qt43->glDrawBuffer(mode);
}

static void Qt_ReadBuffer(GLenum mode) {
    g_qt43->glReadBuffer(mode);
}

static GLboolean Qt_IsFramebuffer(GLuint framebuffer) {
    return g_qt->glIsFramebuffer(framebuffer);
}

static GLboolean Qt_IsTexture(GLuint texture) {
    return g_qt->glIsTexture(texture);
}

static void Qt_DeleteFramebuffers(GLsizei n, const GLuint* framebuffers) {
    g_qt->glDeleteFramebuffers(n, framebuffers);
}

static void Qt_DeleteTextures(GLsizei n, const GLuint* textures) {
    g_qt->glDeleteTextures(n, textures);
}

static void Qt_BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                               GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                               GLbitfield mask, GLenum filter) {
    g_qt43->glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1,
                                              dstX0, dstY0, dstX1, dstY1,
                                              mask, filter);
}

static void Qt_Enable(GLenum cap) {
    g_qt->glEnable(cap);
}

static void Qt_Disable(GLenum cap) {
    g_qt->glDisable(cap);
}

static void Qt_CullFace(GLenum mode) {
    g_qt->glCullFace(mode);
}

static void Qt_FrontFace(GLenum mode) {
    g_qt->glFrontFace(mode);
}

static void Qt_TexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations) {
    g_qt43->glTexImage2DMultisample(target, samples, internalformat, width, height, fixedsamplelocations);
}

static void Qt_GenRenderbuffers(GLsizei n, GLuint* renderbuffers) {
    g_qt->glGenRenderbuffers(n, renderbuffers);
}

static void Qt_BindRenderbuffer(GLenum target, GLuint renderbuffer) {
    g_qt->glBindRenderbuffer(target, renderbuffer);
}

static void Qt_RenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height) {
    g_qt43->glRenderbufferStorageMultisample(target, samples, internalformat, width, height);
}

static void Qt_RenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height) {
    g_qt->glRenderbufferStorage(target, internalformat, width, height);
}

static void Qt_FramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer) {
    g_qt->glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer);
}

static GLenum Qt_CheckFramebufferStatus(GLenum target) {
    return g_qt->glCheckFramebufferStatus(target);
}

static void Qt_DeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers) {
    g_qt->glDeleteRenderbuffers(n, renderbuffers);
}

static void Qt_DrawBuffers(GLsizei n, const GLenum *bufs){
    g_qt43->glDrawBuffers(n, bufs);
}

static void Qt_GetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params){
    g_qt->glGetFramebufferAttachmentParameteriv(target, attachment, pname, params);
}

static GLenum Qt_GetError(){
    return g_qt->glGetError();
}

static void Qt_GetIntegerv(GLenum pname, GLint *params){
    g_qt->glGetIntegerv(pname, params);
}

// === INIT FUNCTION ===

void OpenGL::InitFromQt() {
    g_qt = QOpenGLContext::currentContext()->functions();
    g_qt43 = QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_4_3_Core>(QOpenGLContext::currentContext());
    if (!g_qt43) {
        qWarning() << "Could not obtain OpenGL 4.3 core functions";
        return;
    }

    g_qt43->initializeOpenGLFunctions();

    GetGL().GetString               = &Qt_GetString;
    GetGL().Clear                   = &Qt_Clear;
    GetGL().ClearColor              = &Qt_ClearColor;
    GetGL().BlendFunc               = &Qt_BlendFunc;
    GetGL().PolygonMode             = &Qt_PolygonMode;
    GetGL().GenerateMipmap          = &Qt_GenerateMipmap;
    GetGL().UseProgram              = &Qt_UseProgram;
    GetGL().CreateShader            = &Qt_CreateShader;
    GetGL().ShaderSource            = &Qt_ShaderSource;
    GetGL().CompileShader           = &Qt_CompileShader;
    GetGL().CreateProgram           = &Qt_CreateProgram;
    GetGL().AttachShader            = &Qt_AttachShader;
    GetGL().LinkProgram             = &Qt_LinkProgram;
    GetGL().DeleteShader            = &Qt_DeleteShader;
    GetGL().GetProgramInfoLog       = &Qt_GetProgramInfoLog;
    GetGL().GetProgramiv            = &Qt_GetProgramiv;
    GetGL().GetActiveUniform        = &Qt_GetActiveUniform;
    GetGL().DeleteProgram           = &Qt_DeleteProgram;
    GetGL().GetShaderiv             = &Qt_GetShaderiv;
    GetGL().GetShaderInfoLog        = &Qt_GetShaderInfoLog;
    GetGL().PixelStorei             = &Qt_PixelStorei;
    GetGL().BindTexture             = &Qt_BindTexture;
    GetGL().Viewport                = &Qt_Viewport;
    GetGL().DrawArrays              = &Qt_DrawArrays;
    GetGL().DrawElements            = &Qt_DrawElements;
    GetGL().Uniform1i               = &Qt_Uniform1i;
    GetGL().Uniform1f               = &Qt_Uniform1f;
    GetGL().Uniform2fv              = &Qt_Uniform2fv;
    GetGL().Uniform2f               = &Qt_Uniform2f;
    GetGL().Uniform3fv              = &Qt_Uniform3fv;
    GetGL().Uniform3f               = &Qt_Uniform3f;
    GetGL().Uniform4fv              = &Qt_Uniform4fv;
    GetGL().Uniform4f               = &Qt_Uniform4f;
    GetGL().UniformMatrix2fv        = &Qt_UniformMatrix2fv;
    GetGL().UniformMatrix3fv        = &Qt_UniformMatrix3fv;
    GetGL().UniformMatrix4fv        = &Qt_UniformMatrix4fv;
    GetGL().GetUniformLocation      = &Qt_GetUniformLocation;
    GetGL().GenVertexArrays         = &Qt_GenVertexArrays;
    GetGL().BindVertexArray         = &Qt_BindVertexArray;
    GetGL().GenBuffers              = &Qt_GenBuffers;
    GetGL().BindBuffer              = &Qt_BindBuffer;
    GetGL().BufferData              = &Qt_BufferData;
    GetGL().BufferSubData           = &Qt_BufferSubData;
    GetGL().BindBufferBase          = &Qt_BindBufferBase;
    GetGL().EnableVertexAttribArray = &Qt_EnableVertexAttribArray;
    GetGL().VertexAttribPointer     = &Qt_VertexAttribPointer;
    GetGL().DeleteVertexArrays      = &Qt_DeleteVertexArrays;
    GetGL().DeleteBuffers           = &Qt_DeleteBuffers;
    GetGL().GenTextures             = &Qt_GenTextures;
    GetGL().ActiveTexture           = &Qt_ActiveTexture;
    GetGL().TexStorage3D            = &Qt_TexStorage3D;
    GetGL().TexParameteri           = &Qt_TexParameteri;
    GetGL().TexImage2D              = &Qt_TexImage2D;
    GetGL().GenFramebuffers         = &Qt_GenFramebuffers;
    GetGL().BindFramebuffer         = &Qt_BindFramebuffer;
    GetGL().FramebufferTexture2D    = &Qt_FramebufferTexture2D;
    GetGL().FramebufferTextureLayer = &Qt_FramebufferTextureLayer;
    GetGL().DrawBuffer              = &Qt_DrawBuffer;
    GetGL().ReadBuffer              = &Qt_ReadBuffer;
    GetGL().IsFramebuffer           = &Qt_IsFramebuffer;
    GetGL().IsTexture               = &Qt_IsTexture;
    GetGL().DeleteFramebuffers      = &Qt_DeleteFramebuffers;
    GetGL().DeleteTextures          = &Qt_DeleteTextures;
    GetGL().BlitFramebuffer         = &Qt_BlitFramebuffer;
    GetGL().Enable                  = &Qt_Enable;
    GetGL().Disable                 = &Qt_Disable;
    GetGL().CullFace                = &Qt_CullFace;
    GetGL().FrontFace               = &Qt_FrontFace;

    GetGL().TexImage2DMultisample   = &Qt_TexImage2DMultisample;
    GetGL().GenRenderbuffers        = &Qt_GenRenderbuffers;
    GetGL().BindRenderbuffer        = &Qt_BindRenderbuffer;
    GetGL().RenderbufferStorageMultisample = &Qt_RenderbufferStorageMultisample;
    GetGL().RenderbufferStorage     = &Qt_RenderbufferStorage;
    GetGL().FramebufferRenderbuffer = &Qt_FramebufferRenderbuffer;
    GetGL().CheckFramebufferStatus  = &Qt_CheckFramebufferStatus;
    GetGL().DeleteRenderbuffers     = &Qt_DeleteRenderbuffers;
    GetGL().DrawBuffers             = &Qt_DrawBuffers;
    GetGL().GetFramebufferAttachmentParameteriv = &Qt_GetFramebufferAttachmentParameteriv;
    GetGL().GetError = &Qt_GetError;
    GetGL().GetIntegerv = &Qt_GetIntegerv;
}
