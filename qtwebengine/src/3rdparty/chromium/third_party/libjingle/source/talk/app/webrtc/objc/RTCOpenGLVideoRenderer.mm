/*
 * libjingle
 * Copyright 2014, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#import "RTCOpenGLVideoRenderer.h"

#if TARGET_OS_IPHONE
#import <OpenGLES/ES2/gl.h>
#else
#import <OpenGL/gl3.h>
#endif

#import "RTCI420Frame.h"

// TODO(tkchin): check and log openGL errors. Methods here return BOOLs in
// anticipation of that happening in the future.

#if TARGET_OS_IPHONE
#define RTC_PIXEL_FORMAT GL_LUMINANCE
#define SHADER_VERSION
#define VERTEX_SHADER_IN "attribute"
#define VERTEX_SHADER_OUT "varying"
#define FRAGMENT_SHADER_IN "varying"
#define FRAGMENT_SHADER_OUT
#define FRAGMENT_SHADER_COLOR "gl_FragColor"
#define FRAGMENT_SHADER_TEXTURE "texture2D"
#else
#define RTC_PIXEL_FORMAT GL_RED
#define SHADER_VERSION "#version 150\n"
#define VERTEX_SHADER_IN "in"
#define VERTEX_SHADER_OUT "out"
#define FRAGMENT_SHADER_IN "in"
#define FRAGMENT_SHADER_OUT "out vec4 fragColor;\n"
#define FRAGMENT_SHADER_COLOR "fragColor"
#define FRAGMENT_SHADER_TEXTURE "texture"
#endif

// Vertex shader doesn't do anything except pass coordinates through.
static const char kVertexShaderSource[] =
  SHADER_VERSION
  VERTEX_SHADER_IN " vec2 position;\n"
  VERTEX_SHADER_IN " vec2 texcoord;\n"
  VERTEX_SHADER_OUT " vec2 v_texcoord;\n"
  "void main() {\n"
  "    gl_Position = vec4(position.x, position.y, 0.0, 1.0);\n"
  "    v_texcoord = texcoord;\n"
  "}\n";

// Fragment shader converts YUV values from input textures into a final RGB
// pixel. The conversion formula is from http://www.fourcc.org/fccyvrgb.php.
static const char kFragmentShaderSource[] =
  SHADER_VERSION
  "precision highp float;"
  FRAGMENT_SHADER_IN " vec2 v_texcoord;\n"
  "uniform lowp sampler2D s_textureY;\n"
  "uniform lowp sampler2D s_textureU;\n"
  "uniform lowp sampler2D s_textureV;\n"
  FRAGMENT_SHADER_OUT
  "void main() {\n"
  "    float y, u, v, r, g, b;\n"
  "    y = " FRAGMENT_SHADER_TEXTURE "(s_textureY, v_texcoord).r;\n"
  "    u = " FRAGMENT_SHADER_TEXTURE "(s_textureU, v_texcoord).r;\n"
  "    v = " FRAGMENT_SHADER_TEXTURE "(s_textureV, v_texcoord).r;\n"
  "    u = u - 0.5;\n"
  "    v = v - 0.5;\n"
  "    r = y + 1.403 * v;\n"
  "    g = y - 0.344 * u - 0.714 * v;\n"
  "    b = y + 1.770 * u;\n"
  "    " FRAGMENT_SHADER_COLOR " = vec4(r, g, b, 1.0);\n"
  "  }\n";

// Compiles a shader of the given |type| with GLSL source |source| and returns
// the shader handle or 0 on error.
GLuint CreateShader(GLenum type, const GLchar* source) {
  GLuint shader = glCreateShader(type);
  if (!shader) {
    return 0;
  }
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  GLint compileStatus = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
  if (compileStatus == GL_FALSE) {
    glDeleteShader(shader);
    shader = 0;
  }
  return shader;
}

// Links a shader program with the given vertex and fragment shaders and
// returns the program handle or 0 on error.
GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader) {
  if (vertexShader == 0 || fragmentShader == 0) {
    return 0;
  }
  GLuint program = glCreateProgram();
  if (!program) {
    return 0;
  }
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
  GLint linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus == GL_FALSE) {
    glDeleteProgram(program);
    program = 0;
  }
  return program;
}

// When modelview and projection matrices are identity (default) the world is
// contained in the square around origin with unit size 2. Drawing to these
// coordinates is equivalent to drawing to the entire screen. The texture is
// stretched over that square using texture coordinates (u, v) that range
// from (0, 0) to (1, 1) inclusive. Texture coordinates are flipped vertically
// here because the incoming frame has origin in upper left hand corner but
// OpenGL expects origin in bottom left corner.
const GLfloat gVertices[] = {
  // X, Y, U, V.
  -1, -1, 0, 1,  // Bottom left.
   1, -1, 1, 1,  // Bottom right.
   1,  1, 1, 0,  // Top right.
  -1,  1, 0, 0,  // Top left.
};

// |kNumTextures| must not exceed 8, which is the limit in OpenGLES2. Two sets
// of 3 textures are used here, one for each of the Y, U and V planes. Having
// two sets alleviates CPU blockage in the event that the GPU is asked to render
// to a texture that is already in use.
static const GLsizei kNumTextureSets = 2;
static const GLsizei kNumTextures = 3 * kNumTextureSets;

@implementation RTCOpenGLVideoRenderer {
#if TARGET_OS_IPHONE
  EAGLContext* _context;
#else
  NSOpenGLContext* _context;
#endif
  BOOL _isInitialized;
  NSUInteger _currentTextureSet;
  // Handles for OpenGL constructs.
  GLuint _textures[kNumTextures];
  GLuint _program;
#if !TARGET_OS_IPHONE
  GLuint _vertexArray;
#endif
  GLuint _vertexBuffer;
  GLint _position;
  GLint _texcoord;
  GLint _ySampler;
  GLint _uSampler;
  GLint _vSampler;
}

+ (void)initialize {
  // Disable dithering for performance.
  glDisable(GL_DITHER);
}

#if TARGET_OS_IPHONE
- (instancetype)initWithContext:(EAGLContext*)context {
#else
- (instancetype)initWithContext:(NSOpenGLContext*)context {
#endif
  NSAssert(context != nil, @"context cannot be nil");
  if (self = [super init]) {
    _context = context;
  }
  return self;
}

- (BOOL)drawFrame:(RTCI420Frame*)frame {
  if (!_isInitialized) {
    return NO;
  }
  if (_lastDrawnFrame == frame) {
    return NO;
  }
  [self ensureGLContext];
  if (![self updateTextureSizesForFrame:frame] ||
      ![self updateTextureDataForFrame:frame]) {
    return NO;
  }
  glClear(GL_COLOR_BUFFER_BIT);
#if !TARGET_OS_IPHONE
  glBindVertexArray(_vertexArray);
#endif
  glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#if !TARGET_OS_IPHONE
  [_context flushBuffer];
#endif
  _lastDrawnFrame = frame;
  return YES;
}

- (void)setupGL {
  if (_isInitialized) {
    return;
  }
  [self ensureGLContext];
  if (![self setupProgram]) {
    return;
  }
  if (![self setupTextures]) {
    return;
  }
  if (![self setupVertices]) {
    return;
  }
  glUseProgram(_program);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glClearColor(0, 0, 0, 1);
  _isInitialized = YES;
}

- (void)teardownGL {
  if (!_isInitialized) {
    return;
  }
  [self ensureGLContext];
  glDeleteProgram(_program);
  _program = 0;
  glDeleteTextures(kNumTextures, _textures);
  glDeleteBuffers(1, &_vertexBuffer);
  _vertexBuffer = 0;
#if !TARGET_OS_IPHONE
  glDeleteVertexArrays(1, &_vertexArray);
#endif
  _isInitialized = NO;
}

#pragma mark - Private

- (void)ensureGLContext {
  NSAssert(_context, @"context shouldn't be nil");
#if TARGET_OS_IPHONE
  if ([EAGLContext currentContext] != _context) {
    [EAGLContext setCurrentContext:_context];
  }
#else
  if ([NSOpenGLContext currentContext] != _context) {
    [_context makeCurrentContext];
  }
#endif
}

- (BOOL)setupProgram {
  NSAssert(!_program, @"program already set up");
  GLuint vertexShader = CreateShader(GL_VERTEX_SHADER, kVertexShaderSource);
  NSAssert(vertexShader, @"failed to create vertex shader");
  GLuint fragmentShader =
      CreateShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
  NSAssert(fragmentShader, @"failed to create fragment shader");
  _program = CreateProgram(vertexShader, fragmentShader);
  // Shaders are created only to generate program.
  if (vertexShader) {
    glDeleteShader(vertexShader);
  }
  if (fragmentShader) {
    glDeleteShader(fragmentShader);
  }
  if (!_program) {
    return NO;
  }
  _position = glGetAttribLocation(_program, "position");
  _texcoord = glGetAttribLocation(_program, "texcoord");
  _ySampler = glGetUniformLocation(_program, "s_textureY");
  _uSampler = glGetUniformLocation(_program, "s_textureU");
  _vSampler = glGetUniformLocation(_program, "s_textureV");
  if (_position < 0 || _texcoord < 0 || _ySampler < 0 || _uSampler < 0 ||
      _vSampler < 0) {
    return NO;
  }
  return YES;
}

- (BOOL)setupTextures {
  glGenTextures(kNumTextures, _textures);
  // Set parameters for each of the textures we created.
  for (GLsizei i = 0; i < kNumTextures; i++) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, _textures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  }
  return YES;
}

- (BOOL)updateTextureSizesForFrame:(RTCI420Frame*)frame {
  if (frame.height == _lastDrawnFrame.height &&
      frame.width == _lastDrawnFrame.width &&
      frame.chromaWidth == _lastDrawnFrame.chromaWidth &&
      frame.chromaHeight == _lastDrawnFrame.chromaHeight) {
    return YES;
  }
  GLsizei lumaWidth = frame.width;
  GLsizei lumaHeight = frame.height;
  GLsizei chromaWidth = frame.chromaWidth;
  GLsizei chromaHeight = frame.chromaHeight;
  for (GLint i = 0; i < kNumTextureSets; i++) {
    glActiveTexture(GL_TEXTURE0 + i * 3);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 RTC_PIXEL_FORMAT,
                 lumaWidth,
                 lumaHeight,
                 0,
                 RTC_PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE,
                 0);
    glActiveTexture(GL_TEXTURE0 + i * 3 + 1);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 RTC_PIXEL_FORMAT,
                 chromaWidth,
                 chromaHeight,
                 0,
                 RTC_PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE,
                 0);
    glActiveTexture(GL_TEXTURE0 + i * 3 + 2);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 RTC_PIXEL_FORMAT,
                 chromaWidth,
                 chromaHeight,
                 0,
                 RTC_PIXEL_FORMAT,
                 GL_UNSIGNED_BYTE,
                 0);
  }
  return YES;
}

- (BOOL)updateTextureDataForFrame:(RTCI420Frame*)frame {
  NSUInteger textureOffset = _currentTextureSet * 3;
  NSAssert(textureOffset + 3 <= kNumTextures, @"invalid offset");
  NSParameterAssert(frame.yPitch == frame.width);
  NSParameterAssert(frame.uPitch == frame.chromaWidth);
  NSParameterAssert(frame.vPitch == frame.chromaWidth);

  glActiveTexture(GL_TEXTURE0 + textureOffset);
  // When setting texture sampler uniforms, the texture index is used not
  // the texture handle.
  glUniform1i(_ySampler, textureOffset);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               RTC_PIXEL_FORMAT,
               frame.width,
               frame.height,
               0,
               RTC_PIXEL_FORMAT,
               GL_UNSIGNED_BYTE,
               frame.yPlane);

  glActiveTexture(GL_TEXTURE0 + textureOffset + 1);
  glUniform1i(_uSampler, textureOffset + 1);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               RTC_PIXEL_FORMAT,
               frame.chromaWidth,
               frame.chromaHeight,
               0,
               RTC_PIXEL_FORMAT,
               GL_UNSIGNED_BYTE,
               frame.uPlane);

  glActiveTexture(GL_TEXTURE0 + textureOffset + 2);
  glUniform1i(_vSampler, textureOffset + 2);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               RTC_PIXEL_FORMAT,
               frame.chromaWidth,
               frame.chromaHeight,
               0,
               RTC_PIXEL_FORMAT,
               GL_UNSIGNED_BYTE,
               frame.vPlane);

  _currentTextureSet = (_currentTextureSet + 1) % kNumTextureSets;
  return YES;
}

- (BOOL)setupVertices {
#if !TARGET_OS_IPHONE
  NSAssert(!_vertexArray, @"vertex array already set up");
  glGenVertexArrays(1, &_vertexArray);
  if (!_vertexArray) {
    return NO;
  }
  glBindVertexArray(_vertexArray);
#endif
  NSAssert(!_vertexBuffer, @"vertex buffer already set up");
  glGenBuffers(1, &_vertexBuffer);
  if (!_vertexBuffer) {
#if !TARGET_OS_IPHONE
    glDeleteVertexArrays(1, &_vertexArray);
    _vertexArray = 0;
#endif
    return NO;
  }
  glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(gVertices), gVertices, GL_DYNAMIC_DRAW);

  // Read position attribute from |gVertices| with size of 2 and stride of 4
  // beginning at the start of the array. The last argument indicates offset
  // of data within |gVertices| as supplied to the vertex buffer.
  glVertexAttribPointer(
      _position, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(_position);

  // Read texcoord attribute from |gVertices| with size of 2 and stride of 4
  // beginning at the first texcoord in the array. The last argument indicates
  // offset of data within |gVertices| as supplied to the vertex buffer.
  glVertexAttribPointer(_texcoord,
                        2,
                        GL_FLOAT,
                        GL_FALSE,
                        4 * sizeof(GLfloat),
                        (void*)(2 * sizeof(GLfloat)));
  glEnableVertexAttribArray(_texcoord);

  return YES;
}

@end
