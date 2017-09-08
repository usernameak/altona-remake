/*+**************************************************************************/
/***                                                                      ***/
/***   This file is distributed under a BSD license.                      ***/
/***   See LICENSE.txt for details.                                       ***/
/***                                                                      ***/
/**************************************************************************+*/

#include <GL/glew.h>
//#include <GLEW/glext.h>

#include "base/types.hpp"
#include "graphics.hpp"
#include "base/windows.hpp"
#include <GL/glx.h>

void sInitGfxCommon();
void sExitGfxCommon();
void ResizeGFX(sInt x, sInt y);
extern sInt sSystemFlags;

extern Display *sXDisplay();
extern XVisualInfo *sXVisualInfo;
extern Visual *sXVisual;
extern Window sXWnd;
extern sInt sXScreen;

static sScreenMode GLScreenMode;

static GLXContext GLXC;

/****************************************************************************/

static sGraphicsStats Stats;
static sGraphicsStats BufferedStats;
static sGraphicsStats DisabledStats;
static sBool StatsEnable;
static sGraphicsCaps GraphicsCapsMaster;
static sTexture2D *GLBackBuffer;

static sU16 QuadIndexBuffer[0x10000 * 6 / 4];

struct sGeoBuffer
{
    GLuint GLName; // gl name
    sInt GLType;   // GL_ARRAY_BUFFER or GL_ELEMENT_ARRAY_BUFFER
    sInt GLUsage;  // GL_STATIC_DRAW, ...

    sGeometryDuration Duration; // sGD_???
    sInt Type;                  // 0 = VB, 1 = IB

    sInt Alloc; // total available bytes
    sInt Used;  // alreaedy used bytes
    sInt Freed; // bytes freed. when Freed == Used, then the buffer is empty and may be reset
};

#define sMAX_GEOBUFFER 256

sGeoBuffer sGeoBuffers[sMAX_GEOBUFFER];
sInt sGeoBufferCount;

static sInt OGLGfxWidth;
static sInt OGLGfxHeight;

/****************************************************************************/
/***                                                                      ***/
/***   Fake                                                               ***/
/***                                                                      ***/
/****************************************************************************/

//#import <GLES/gl.h>
//#import <GLES/glext.h>

/*GLint GLES_ScreenX;
GLint GLES_ScreenY;*/
GLuint GLES_FrameBuffer;
GLuint GLES_ColorBuffer = 0;
GLuint GLES_DepthBuffer = 0;

#ifdef __GLIBC__
#include <execinfo.h>
#endif

void GLERR()
{
    sBool ok = 1;
    GLenum err;
    for (;;)
    {
        err = glGetError();
        if (err == GL_NO_ERROR)
            break;
        ok = 0;
        switch (err)
        {
        case GL_INVALID_ENUM:
            sLog(L"gfx", L"ERROR invalid enum\n");
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            sLogF(L"gfx", L"ERROR invalid fb, code %d\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
            break;
        case GL_INVALID_VALUE:
            sLog(L"gfx", L"ERROR invalid value\n");
            break;
        case GL_INVALID_OPERATION:
            sLog(L"gfx", L"ERROR invalid op\n");
            break;
        case GL_OUT_OF_MEMORY:
            sLog(L"gfx", L"ERROR out of mem\n");
            break;
        default:
            sLog(L"gfx", L"ERROR unknown\n");
            break;
        }
    }
#ifdef __GLIBC__
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    if(!ok) {
        backtrace_symbols_fd(array, size, 2);
        /*for (int i = 1; i < size; ++i)
        {
            //sPrintF(L"[bt] #%d %s\n", i, messages[i]);
            sString<256> syscom;
            syscom.PrintF(L"addr2line %p -e altona_wireapp_example", array[i]); //last parameter is the name of this app
            sExecuteShell(syscom);
        }*/
    }
    
#endif
    sVERIFY(ok);
}

void CompileShader(GLuint *shaderp, GLenum type, const sChar *src)
{
    GLint status;
    GLuint shader = 0;

    char buffer[4096];

    sCopyString(buffer, src, 4096);

    const GLchar *bufp = buffer;
    shader = glCreateShader(type);
    glShaderSource(shader, 1, &bufp, 0);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0)
    {
        char buf8[1024];
        sChar buf[1024];
        glGetShaderInfoLog(shader, 1024, 0, buf8);
        glDeleteShader(shader);
        sCopyString(buf, buf8, 1024);
        sLog(L"gfx", L"shader compiler failed\n");
        sLog(L"gfx", buf);
        shader = 0;
        sVERIFYFALSE;
    }
    *shaderp = shader;
    GLERR();
}

/****************************************************************************/
/***                                                                      ***/
/***   Initialisation                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void PreInitGFX(sInt &flags, sInt &xs, sInt &ys)
{
//xs = 1024;
//ys = 640;
//flags = 0;
#if sPLATFORM == sPLAT_WINDOWS

    if (flags & sISF_FULLSCREEN)
    {
        DEVMODE dm;
        sClear(dm);
        dm.dmSize = sizeof(dm);
        dm.dmPelsWidth = xs;
        dm.dmPelsHeight = ys;
        dm.dmBitsPerPel = 32;
        dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

        if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
        {
            dm.dmPelsWidth = xs = 800;
            dm.dmPelsHeight = ys = 600;
            if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
            {
                sFatal(L"could not set displaymode %dx%d", xs, ys);
            }
        }
    }

#elif sPLATFORM == sPLAT_LINUX

    static int attribList[] =
        {
            GLX_RGBA, GLX_DOUBLEBUFFER,
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            GLX_STENCIL_SIZE, 8,
            None};

    Display *dpy = sXDisplay();

    sXVisualInfo = glXChooseVisual(dpy, sXScreen, attribList);
    if (!sXVisualInfo)
        sFatal(L"glXChooseVisual returned 0!\n");

#endif
}

void InitGFX(sInt flags_, sInt xs_, sInt ys_)
{
    sLog(L"gfx", L"init\n");
#if sPLATFORM == sPLAT_LINUX

    Display *dpy = sXDisplay();
    GLXC = glXCreateContext(dpy, sXVisualInfo, 0, True);
    if (!GLXC)
        sFatal(L"glXCreateContext failed!\n");

    if (!glXMakeCurrent(dpy, sXWnd, GLXC))
        sFatal(L"glXMakeCurrent failed!\n");

#endif
    // query

    sChar buffer[2048];
    const GLubyte *str;

    str = glGetString(GL_VENDOR);
    sCopyString(buffer, (const sChar8 *)str, 2048);
    sLogF(L"gfx", L"GL Vendor: %q\n", buffer);

    str = glGetString(GL_RENDERER);
    sCopyString(buffer, (const sChar8 *)str, 2048);
    sLogF(L"gfx", L"GL Renderer: %q\n", buffer);

    str = glGetString(GL_VERSION);
    sCopyString(buffer, (const sChar8 *)str, 2048);
    sLogF(L"gfx", L"GL Version: %q\n", buffer);

    str = glGetString(GL_SHADING_LANGUAGE_VERSION);
    sCopyString(buffer, (const sChar8 *)str, 2048);
    sLogF(L"gfx", L"GL Shader Model: %q\n", buffer);

    str = glGetString(GL_EXTENSIONS);
    while (*str)
    {
        sInt n = 0;
        while (str[n] != ' ')
            n++;
        sCopyString(buffer, (const sChar8 *)str, n + 1);
        str += n;
        while (*str == ' ')
            str++;
        sLogF(L"gfx", L"GLES Extension: %q\n", buffer);
    }

    int err = glewInit();
    if (err != GLEW_OK)
    {
        sFatal(L"GLEW error code %d", err);
    }

    // set up framebuffer

    /*GLBackBuffer = new sTexture2D(xs_, ys_, sTEX_ARGB8888);
    glBindTexture(GL_TEXTURE_2D, GLBackBuffer->GLName);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xs_, ys_, 0, GL_RGBA, GL_UNSIGNED_BYTE, sNULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &GLES_FrameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, GLES_FrameBuffer);*/
    /*
  glGenRenderbuffers(1, &GLES_ColorBuffer);

  glBindRenderbuffer(GL_RENDERBUFFER, GLES_ColorBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, xs_, ys_);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, GLES_ColorBuffer);
  GLERR();*/
    ResizeGFX(xs_, ys_);

    // intialize quad index buffer

    sU16 *ip = QuadIndexBuffer;
    for (sInt i = 0; i < 0x4000; i++)
        sQuad(ip, i * 4, 0, 1, 2, 3);

    // ..

    sInitGfxCommon();
}

void IOSResize1()
{
    //glBindRenderbuffer(GL_RENDERBUFFER,GLES_ColorBuffer);
    GLERR();
}

/*void IOSResize2()
{
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH , &GLES_ScreenX);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &GLES_ScreenY);
  glDeleteRenderbuffers(1,&GLES_DepthBuffer);
  glGenRenderbuffers(1,&GLES_DepthBuffer);
  glBindRenderbuffer(GL_RENDERBUFFER,GLES_DepthBuffer);
  glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,GLES_ScreenX,GLES_ScreenY);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,GLES_DepthBuffer);

  GLenum err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  sVERIFY(err==GL_FRAMEBUFFER_COMPLETE);
}*/

void ExitGFX()
{
    sLog(L"gfx", L"exit\n");
    sExitGfxCommon();

    // Tear down GL
    /*glDeleteFramebuffers(1, &GLES_FrameBuffer);
    glDeleteRenderbuffers(1, &GLES_DepthBuffer);
    glDeleteRenderbuffers(1, &GLES_ColorBuffer);*/
}

void ResizeGFX(sInt w, sInt h)
{
    glViewport(0, 0, w, h);
    /*if (GLES_ColorBuffer != 0)
        glDeleteRenderbuffers(1, &GLES_ColorBuffer);
    glGenRenderbuffers(1, &GLES_ColorBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, GLES_ColorBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, w, h);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GLBackBuffer->GLName, 0);
    if (GLES_DepthBuffer != 0)
        glDeleteRenderbuffers(1, &GLES_DepthBuffer);
    glGenRenderbuffers(1, &GLES_DepthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, GLES_DepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, GLES_DepthBuffer);
    GLERR();*/
}

/****************************************************************************/
/***                                                                      ***/
/***   Draw Loop                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sBool sRender3DBegin()
{
    XLockDisplay(sXDisplay());
    /*glBindFramebuffer(GL_FRAMEBUFFER, GLES_FrameBuffer);*/
    glViewport(0, 0, OGLGfxWidth, OGLGfxHeight);
    GLERR();

    return 1;
}

void sRender3DEnd(sBool flip)
{
    XUnlockDisplay(sXDisplay());
    GLERR();
    /*glBindFramebuffer(GL_FRAMEBUFFER, GLES_FrameBuffer);
    glEnable(GL_TEXTURE_2D);

    glDisable(GL_TEXTURE_2D);
    glFinish();*/
    //sSleep(1000);
    sPreFlipHook->Call();
    #if sPLATFORM == sPLAT_WINDOWS
    //  SwapBuffers(GLDC);
    #elif sPLATFORM == sPLAT_LINUX
        glXSwapBuffers(sXDisplay(), sXWnd);
    #endif

    sFlipMem();
    sPostFlipHook->Call();
    GLERR();
}

void sRender3DFlush()
{
    GLERR();
    glFlush();
    GLERR();
}

/****************************************************************************/
/***                                                                      ***/
/***   Display Info                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sInt sGetDisplayCount()
{
    return 0;
}
void sGetScreenInfo(sScreenInfo &si, sInt flags, sInt display)
{
}
void sGetScreenMode(sScreenMode &sm)
{
}
sBool sSetScreenMode(const sScreenMode &smorg)
{
    return 0;
}

/****************************************************************************/
/***                                                                      ***/
/***   General Engine Interface, new style                                ***/
/***                                                                      ***/
/****************************************************************************/

void sSetTarget(const sTargetPara &para)
{
    //sPrint(L"settarget\n");
    glClearColor(para.ClearColor[0].x, para.ClearColor[0].y, para.ClearColor[0].z, para.ClearColor[0].w);
    glClearDepthf(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    sGFXRendertargetX = para.Window.SizeX();
    sGFXRendertargetY = para.Window.SizeY();
    sGFXRendertargetAspect = sF32(sGFXRendertargetX) / sGFXRendertargetY;
    sGFXViewRect.Init(0, 0, sGFXRendertargetX, sGFXRendertargetY);

    glScissor(para.Window.x0, para.Window.y0, para.Window.SizeX(), para.Window.SizeY());
    glEnable(GL_SCISSOR_TEST);
    glViewport(para.Window.x0, para.Window.y0, para.Window.SizeX(), para.Window.SizeY());

    GLERR();
}

void sResolveTarget()
{
}
void sResolveTexture(sTextureBase *tex)
{
}
void sSetScissor(const sRect &r)
{
}
sInt sGetScreenCount()
{
    return 0;
}
sTexture2D *sGetScreenColorBuffer(sInt screen)
{
    return GLBackBuffer;
}
sTexture2D *sGetScreenDepthBuffer(sInt screen)
{
    return 0;
}
sTexture2D *sGetRTDepthBuffer()
{
    return 0;
}
void sEnlargeRTDepthBuffer(sInt x, sInt y)
{
}
void sBeginReadTexture(const sU8 *&_data, sS32 &pitch, enum sTextureFlags &flags, sTexture2D *tex)
{
    glActiveTexture(GL_TEXTURE0);
    GLERR();
    glBindTexture(GL_TEXTURE_2D, tex->GLName);
    GLERR();
    sInt w;
    sInt h;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
    GLERR();
    sU8 *data = new sU8[w * h * 4];
    glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
    _data = data;
    flags = sTEX_ARGB8888;
    pitch = w * 4;
    GLERR();
}
void sEndReadTexture()
{
    GLERR();
}
void sCopyTexture(const sCopyTexturePara &para)
{
}

void sGetGraphicsStats(sGraphicsStats &stat)
{
    stat = BufferedStats;
}

void sEnableGraphicsStats(sBool enable)
{
    if (!enable && StatsEnable)
    {
        DisabledStats = Stats;
        StatsEnable = 0;
    }
    if (enable && !StatsEnable)
    {
        Stats = DisabledStats;
        StatsEnable = 1;
    }
}

void sGetGraphicsCaps(sGraphicsCaps &caps)
{
    caps = GraphicsCapsMaster;
}

/****************************************************************************/
/***                                                                      ***/
/***   Obsolete Functions                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sConvertsRGBTex(sBool e)
{
    sVERIFYFALSE;
}

sBool sConvertsRGBTex()
{
    sVERIFYFALSE;
    return 0;
}

sTexture2D *sGetCurrentFrontBuffer()
{
    sVERIFYFALSE;
    return 0;
}

void sSetScreen(class sTexture2D *tex, sGrabFilterFlags filter, const sRect *dst, const sRect *src)
{
    sVERIFYFALSE;
}

void sSetScreen(const sRect &rect, sU32 *data)
{
    sVERIFYFALSE;
}

void sBeginSaveRT(const sU8 *&data, sS32 &pitch, enum sTextureFlags &flags)
{
    sVERIFYFALSE;
}

void sEndSaveRT()
{
    sVERIFYFALSE;
}

void sSetVSParam(sInt o, sInt count, const sVector4 *vsf)
{
    sVERIFYFALSE;
}

void sSetPSParam(sInt o, sInt count, const sVector4 *psf)
{
    sVERIFYFALSE;
}

void sSetVSBool(sU32 bits, sU32 mask)
{
    sVERIFYFALSE;
}

void sSetPSBool(sU32 bits, sU32 mask)
{
    sVERIFYFALSE;
}

void sCopyCubeFace(sTexture2D *dest, sTextureCube *src, sTexCubeFace cf)
{
    sVERIFYFALSE;
}

sBool sReadTexture(sReader &s, sTextureBase *&tex)
{
    sVERIFYFALSE;
    return 0;
}

void sPackDXT(sU8 *d, sU32 *bmp, sInt xs, sInt ys, sInt format, sBool dither)
{
    sVERIFYFALSE;
}

void sTexture2D::BeginLoadPartial(const sRect &rect, sU8 *&data, sInt &pitch, sInt mipmap)
{
    sVERIFYFALSE;
}

void *sTexture2D::BeginLoadPalette()
{
    sVERIFYFALSE;
    return 0;
}

void sTexture2D::EndLoadPalette()
{
    sVERIFYFALSE;
}

void sTexture2D::CalcOneMiplevel(const sRect &rect)
{
    sVERIFYFALSE;
}

void sTexture3D::Load(sU8 *data)
{
    sVERIFYFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Vertex Formats                                                     ***/
/***                                                                      ***/
/****************************************************************************/

void sVertexFormatHandle::Create()
{
    //MorphTargetId = ~0;
    sInt i, b[sVF_STREAMMAX];
    sInt stream;

    for (i = 0; i < sVF_STREAMMAX; i++)
        b[i] = 0;

    i = 0;
    while (Data[i])
    {
        stream = (Data[i] & sVF_STREAMMASK) >> sVF_STREAMSHIFT;
        //    sVERIFY(stream==0)
        sVERIFY(i < 31);

        Attr[i].Offset = b[stream];
        switch (Data[i] & sVF_TYPEMASK)
        {
        case sVF_F2:
            b[stream] += 2 * 4;
            Attr[i].Size = 2;
            Attr[i].Type = GL_FLOAT;
            Attr[i].Normalized = 0;
            break;
        case sVF_F3:
            b[stream] += 3 * 4;
            Attr[i].Size = 3;
            Attr[i].Type = GL_FLOAT;
            Attr[i].Normalized = 0;
            break;
        case sVF_F4:
            b[stream] += 4 * 4;
            Attr[i].Size = 4;
            Attr[i].Type = GL_FLOAT;
            Attr[i].Normalized = 0;
            break;
        case sVF_I4:
            b[stream] += 1 * 4;
            Attr[i].Size = 4;
            Attr[i].Type = GL_UNSIGNED_BYTE;
            Attr[i].Normalized = 0;
            break;
        case sVF_C4:
            b[stream] += 1 * 4;
            Attr[i].Size = 4;
            Attr[i].Type = GL_UNSIGNED_BYTE;
            Attr[i].Normalized = 1;
            break;
        case sVF_S2:
            b[stream] += 1 * 4;
            Attr[i].Size = 2;
            Attr[i].Type = GL_SHORT;
            Attr[i].Normalized = 1;
            break;
        case sVF_S4:
            b[stream] += 2 * 4;
            Attr[i].Size = 4;
            Attr[i].Type = GL_SHORT;
            Attr[i].Normalized = 1;
            break;
        /*
      case sVF_H2:  b[stream]+=1*4; Attr[i].Size=2; Attr[i].Type=GL_FLOAT; Attr[i].Normalized=0; break;
      case sVF_H3:  b[stream]+=3*2; Attr[i].Size=3; Attr[i].Type=GL_FLOAT; Attr[i].Normalized=0; break;
      case sVF_H4:  b[stream]+=2*4; Attr[i].Size=4; Attr[i].Type=GL_FLOAT; Attr[i].Normalized=0; break;
 */
        case sVF_F1:
            b[stream] += 1 * 4;
            Attr[i].Size = 1;
            Attr[i].Type = GL_FLOAT;
            Attr[i].Normalized = 0;
            break;
        default:
            sVERIFYFALSE;
        }
        switch (Data[i] & sVF_USEMASK)
        {
        case sVF_POSITION:
            Attr[i].Name = "in_pos";
            break;
        case sVF_NORMAL:
            Attr[i].Name = "in_norm";
            break;
        case sVF_TANGENT:
            Attr[i].Name = "in_tang";
            break;
        case sVF_BONEINDEX:
            Attr[i].Name = "in_boneindex";
            break;
        case sVF_BONEWEIGHT:
            Attr[i].Name = "in_boneweight";
            break;
        case sVF_BINORMAL:
            Attr[i].Name = "in_binorm";
            break;
        case sVF_COLOR0:
            Attr[i].Name = "in_col0";
            break;
        case sVF_COLOR1:
            Attr[i].Name = "in_col1";
            break;
        case sVF_COLOR2:
            Attr[i].Name = "in_col2";
            break;
        case sVF_COLOR3:
            Attr[i].Name = "in_col3";
            break;
        case sVF_UV0:
            Attr[i].Name = "in_uv0";
            break;
        case sVF_UV1:
            Attr[i].Name = "in_uv1";
            break;
        case sVF_UV2:
            Attr[i].Name = "in_uv2";
            break;
        case sVF_UV3:
            Attr[i].Name = "in_uv3";
            break;
        case sVF_UV4:
            Attr[i].Name = "in_uv4";
            break;
        case sVF_UV5:
            Attr[i].Name = "in_uv5";
            break;
        case sVF_UV6:
            Attr[i].Name = "in_uv6";
            break;
        case sVF_UV7:
            Attr[i].Name = "in_uv7";
            break;
        default:
            sVERIFYFALSE;
        }

        AvailMask |= 1 << (Data[i] & sVF_USEMASK);
        Streams = sMax(Streams, stream + 1);
        i++;
    }
    AttrCount = i;

    for (sInt i = 0; i < sVF_STREAMMAX; i++)
        VertexSize[i] = b[i];
}

void sVertexFormatHandle::Destroy()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   Shader Interface                                                   ***/
/***                                                                      ***/
/****************************************************************************/

void sCreateShader2(sShader *shader, sShaderBlob *blob)
{
}
void sDeleteShader2(sShader *shader)
{
}

sShaderTypeFlag sGetShaderPlatform()
{
    return sSTF_GLSL;
}

sInt sGetShaderProfile()
{
    return 0;
}

/****************************************************************************/
/****************************************************************************/

sCBufferBase::sCBufferBase()
{
    DataPtr = 0;
    DataPersist = 0;
}

sCBufferBase::~sCBufferBase()
{
}

void sCBufferBase::OverridePtr(void *ptr)
{
    DataPtr = 0;
    DataPersist = ptr;
    Modify();
}

void sCBufferBase::SetPtr(void **dataptr, void *data)
{
    DataPtr = dataptr;
    DataPersist = data;
    *DataPtr = DataPersist;
}

void sCBufferBase::Modify()
{
    if (DataPtr)
        *DataPtr = DataPersist;

    // always set all buffers. because of program binding!
}

void sCBufferBase::SetCfg(sInt slot, sInt start, sInt count)
{
    Slot = slot;
    RegStart = start;
    RegCount = count;
}
/*
void sCBufferBase::SetRegs()
{
  if(RegStart==0 && RegCount>=4 && (Slot&sCBUFFER_SHADERMASK)==sCBUFFER_VS)
  {
    void *m = *DataPtr;
    glUniform4fv(uniforms[UNIFORM_MVP],4,(GLfloat *)m);
  }
}

void sSetCBuffers(sCBufferBase **cbuffers,sInt cbcount)
{
  for(sInt i=0;i<cbcount;i++)
    cbuffers[i]->SetRegs();
}
*/
/*
void sClearCurrentCBuffers()
{
}

sCBufferBase *sGetCurrentCBuffer(sInt slot)
{
  return 0;
}
*/

/****************************************************************************/
/***                                                                      ***/
/***   Geometry                                                           ***/
/***                                                                      ***/
/****************************************************************************/

sGeometryPrivate::sGeometryPrivate()
{
    VPtr = 0;
    VAlloc = 0;
    VUsed = 0;
    VUsedElements = 0;

    IPtr = 0;
    IAlloc = 0;
    IUsed = 0;
    VUsed = 0;
    VUsedElements = 0;

    IPtr = 0;
    IAlloc = 0;
    IUsed = 0;
    IUsedElements = 0;
}

sGeometryPrivate::~sGeometryPrivate()
{
    delete[] VPtr;
    delete[] IPtr;
}

void sGeometry::Init(sInt flags, sVertexFormatHandle *vf)
{
    Flags = flags;
    Format = vf;
    switch (flags & sGF_INDEXMASK)
    {
    case sGF_INDEX16:
        IndexSize = 2;
        break;
    case sGF_INDEX32:
        IndexSize = 4;
        break;
    default:
        IndexSize = 0;
        break;
    }
}

void sGeometry::InitPrivate()
{
}

void sGeometry::ExitPrivate()
{
}

void sGeometry::Clear()
{
    VUsed = 0;
    IUsed = 0;
}

void sGeometry::Draw()
{
    sGeometry::Draw(0, 0, 0, 0);
}

void sGeometry::Draw(sDrawRange *ir, sInt irc, sInt instancecount, sVertexOffset *off)
{
    GLERR();

    sInt n = 0;
    sInt stride = Format->GetSize(0);
    for (; n < Format->AttrCount; n++)
    {
        sVertexFormatHandlePrivate::AttrData *a = &Format->Attr[n];
        glEnableVertexAttribArray(n);
        glVertexAttribPointer(n, a->Size, a->Type, a->Normalized, stride, VPtr + a->Offset);
        GLERR();
    }
    for (; n < sVF_MAXATTRIB; n++)
    {
        glDisableVertexAttribArray(n);
    }

    if ((Flags & sGF_PRIMMASK) == sGF_QUADLIST)
    {
        glDrawElements(GL_TRIANGLES, VUsedElements * 6 / 4, GL_UNSIGNED_SHORT, QuadIndexBuffer);
    }
    else
    {
        if (IUsedElements > 0)
        {
            sVERIFY(IndexSize == 2);
            glDrawElements(GL_TRIANGLES, IUsedElements, GL_UNSIGNED_SHORT, IPtr);
        }
        else
        {
            glDrawArrays(GL_TRIANGLES, 0, VUsedElements);
        }
    }
    GLERR();
}

/****************************************************************************/
/***                                                                      ***/
/***   Geometry Begin/End interface                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::BeginLoadIB(sInt ic, sGeometryDuration duration, void **ip)
{
    sInt size = IndexSize * ic;
    if (size > IAlloc)
    {
        IAlloc = size;
        IPtr = new sU8[size];
    }
    IUsed = size;
    IUsedElements = ic;
    *ip = IPtr;
}

void sGeometry::BeginLoadVB(sInt vc, sGeometryDuration duration, void **vp, sInt stream)
{
    sVERIFY(stream == 0);

    sInt size = Format->GetSize(stream) * vc;
    if (size > VAlloc)
    {
        VAlloc = size;
        VPtr = new sU8[size];
    }
    VUsed = size;
    VUsedElements = vc;
    *vp = VPtr;
}

void sGeometry::EndLoadIB(sInt ic)
{
    sVERIFY(ic == -1);
}

void sGeometry::EndLoadVB(sInt vc, sInt stream)
{
    sVERIFY(vc == -1);
    sVERIFY(stream == 0);
}

void sGeometry::BeginLoad(sVertexFormatHandle *vf, sInt flags, sGeometryDuration duration, sInt vc, sInt ic, void **vp, void **ip)
{
    Init(flags, vf);
    BeginLoadVB(vc, duration, vp, 0);
    BeginLoadIB(ic, duration, ip);
    //IndexPart.Clear();
}

void sGeometry::EndLoad(sInt vc, sInt ic)
{
    EndLoadVB(vc, 0);
    EndLoadIB(ic);
    //VertexPart[0].Unlock(vc, Format->GetSize(0));
    ///if (IndexPart.Buffer)
    //  IndexPart.Unlock(ic, IndexSize);
}
void sGeometry::Merge(sGeometry *geo0, sGeometry *geo1)
{
    sVERIFYFALSE;
}
void sGeometry::MergeVertexStream(sInt DestStream, sGeometry *src, sInt SrcStream)
{
    sVERIFYFALSE;
}

/****************************************************************************/
/***                                                                      ***/
/***   Dynamic Management                                                 ***/
/***                                                                      ***/
/****************************************************************************/

void sGeometry::InitDyn(sInt ic, sInt vc0, sInt vc1, sInt vc2, sInt vc3)
{
}
void *sGeometry::BeginDynVB(sBool discard, sInt stream)
{
    return 0;
}
void *sGeometry::BeginDynIB(sBool discard)
{
    return 0;
}
void sGeometry::EndDynVB(sInt stream)
{
}
void sGeometry::EndDynIB()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureBase                                                       ***/
/***                                                                      ***/
/****************************************************************************/

sU64 sGetAvailTextureFormats()
{
    return (1ULL << sTEX_ARGB8888) |
           0;
}

sTextureBasePrivate::sTextureBasePrivate()
{
    LoadBuffer = 0;
    LoadMipmap = -1;
    GLName = 0;
    GLIFormat = 0;
    GLFormat = 0;
    GLType = 0;
}

sTextureBasePrivate::~sTextureBasePrivate()
{
    sVERIFY(LoadBuffer == 0);
}

/****************************************************************************/
/***                                                                      ***/
/***   sTexture2D                                                         ***/
/***                                                                      ***/
/****************************************************************************/

void sTexture2D::Create2(sInt flags)
{
    glGenTextures(1, (GLuint *)&GLName);

    switch (flags & sTEX_FORMAT)
    {
    case sTEX_ARGB8888:
        GLIFormat = GL_RGBA;
        GLFormat = GL_BGRA;
        GLType = GL_UNSIGNED_BYTE;
        break;

    case sTEX_A8:
        GLIFormat = GL_ALPHA;
        GLFormat = GL_ALPHA;
        GLType = GL_UNSIGNED_BYTE;
        break;

    case sTEX_I8:
        GLIFormat = GL_LUMINANCE;
        GLFormat = GL_LUMINANCE;
        GLType = GL_UNSIGNED_BYTE;
        break;

    default:
        sVERIFYFALSE;
        break;
    }
}

void sTexture2D::Destroy2()
{
    glDeleteTextures(1, (GLuint *)&GLName);
}

void sTexture2D::BeginLoad(sU8 *&data, sInt &pitch, sInt mipmap)
{
    sVERIFY(LoadBuffer == 0);
    pitch = (SizeX >> mipmap) * BitsPerPixel / 8;
    LoadMipmap = mipmap;
    LoadBuffer = new sU8[pitch * (SizeY >> mipmap)];
    data = LoadBuffer;
}

void sTexture2D::EndLoad()
{
    GLERR();
    glActiveTexture(GL_TEXTURE0);
    GLERR();
    glBindTexture(GL_TEXTURE_2D, GLName);
    GLERR();
    glTexImage2D(GL_TEXTURE_2D, LoadMipmap, GLIFormat, SizeX >> LoadMipmap, SizeY >> LoadMipmap, 0, GLFormat, GLType, LoadBuffer);
    GLERR();
    sDelete(LoadBuffer);
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureCube                                                       ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureCube::Create2(sInt flags)
{
}
void sTextureCube::Destroy2()
{
}
void sTextureCube::BeginLoad(sTexCubeFace cf, sU8 *&data, sInt &pitch, sInt mipmap /*=0*/)
{
}
void sTextureCube::EndLoad()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sTexture3D                                                         ***/
/***                                                                      ***/
/****************************************************************************/

sTexture3D::sTexture3D(sInt xs, sInt ys, sInt zs, sU32 flags)
{
}
sTexture3D::~sTexture3D()
{
}
void sTexture3D::BeginLoad(sU8 *&data, sInt &rpitch, sInt &spitch, sInt mipmap /*=0*/)
{
}
void sTexture3D::EndLoad()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sTextureProxy                                                      ***/
/***                                                                      ***/
/****************************************************************************/

void sTextureProxy::Connect2()
{
}
void sTextureProxy::Disconnect2()
{
}

/****************************************************************************/
/***                                                                      ***/
/***   sMaterial                                                          ***/
/***                                                                      ***/
/****************************************************************************/

sMaterialPrivate::sMaterialPrivate()
{
    GLName = 0;
    sClear(VSSlot);
    sClear(GLTexMin);
    sClear(GLTexMax);
    sClear(GLTexS);
    sClear(GLTexT);

    GLBCFunc = GL_FUNC_ADD;
    GLBCSrc = GL_ONE;
    GLBCDst = GL_ZERO;
    GLBAFunc = GL_FUNC_ADD;
    GLBASrc = GL_ONE;
    GLBADst = GL_ZERO;
}

sMaterialPrivate::~sMaterialPrivate()
{
    if (GLName)
        glDeleteProgram(GLName);
}

void sMaterial::Create2()
{
}

void sMaterial::Destroy2()
{
}

void sMaterial::Prepare(sVertexFormatHandle *format)
{
    GLuint vertShader, fragShader;
    GLint status;

    sBool _n = format->Has(sVF_NORMAL);
    sBool _u = format->Has(sVF_UV0);
    sBool _c = format->Has(sVF_COLOR0);

    sTextBuffer vs, ps;

    vs.Print(L"attribute vec4 in_pos;\n");
    if (_n)
        vs.Print(L"attribute vec4 in_norm;\n");
    if (_c)
    {
        vs.Print(L"attribute vec4 in_col0;\n");
        vs.Print(L"varying vec4 out_col0;\n");
        ps.Print(L"varying vec4 out_col0;\n");
    }
    if (_u)
    {
        vs.Print(L"attribute vec2 in_uv0;\n");
        vs.Print(L"varying vec2 out_uv0;\n");
        ps.Print(L"varying vec2 out_uv0;\n");
        ps.Print(L"uniform sampler2D tex0;\n");
    }
    vs.Print(L"uniform vec4 mvp[4];\n");
    vs.Print(L"void main()\n");
    ps.Print(L"void main()\n");
    vs.Print(L"{\n");
    ps.Print(L"{\n");
    vs.Print(L"  gl_Position = in_pos.x*mvp[0] + in_pos.y*mvp[1] + in_pos.z*mvp[2] + in_pos.w*mvp[3];\n");
    ps.Print(L"  vec4 col = vec4(1,1,1,1);\n");
    if (_u)
    {
        vs.Print(L"  out_uv0 = in_uv0;\n");
        ps.Print(L"  col *= texture2D(tex0,out_uv0);\n");
    }
    if (_c)
    {
        vs.Print(L"  out_col0 = in_col0;\n");
        ps.Print(L"  col *= out_col0;\n");
    }
    vs.Print(L"}\n");
    ps.Print(L"  gl_FragColor = col;\n");
    ps.Print(L"}\n");

    GLERR();

    if (0)
    {
        sDPrint(vs.Get());
        sDPrint(ps.Get());
    }

    CompileShader(&vertShader, GL_VERTEX_SHADER, vs.Get());
    CompileShader(&fragShader, GL_FRAGMENT_SHADER, ps.Get());

    sVERIFY(vertShader && fragShader)
        GLName = glCreateProgram();

    GLERR();
    for (sInt i = 0; i < format->AttrCount; i++)
        glBindAttribLocation(GLName, i, format->Attr[i].Name);
    GLERR();

    glAttachShader(GLName, vertShader);
    glAttachShader(GLName, fragShader);
    GLERR();

    glLinkProgram(GLName);
    GLERR();
    glGetProgramiv(GLName, GL_LINK_STATUS, &status);
    sVERIFY(status != 0);

    VSSlot[0] = glGetUniformLocation(GLName, "mvp");
    GLERR();

    if (vertShader)
        glDeleteShader(vertShader);
    if (fragShader)
        glDeleteShader(fragShader);
    GLERR();

    // texture options

    for (sInt i = 0; i < sMTRL_MAXTEX; i++)
    {
        if (Texture[i] || (TFlags[i] & sMTF_EXTERN))
        {
            if (Texture[i] && Texture[i]->Mipmaps == 1)
            {
                switch (TFlags[i] & sMTF_LEVELMASK)
                {
                case sMTF_LEVEL0:
                    GLTexMin[i] = GL_NEAREST;
                    GLTexMax[i] = GL_NEAREST;
                    break;
                case sMTF_LEVEL1:
                case sMTF_LEVEL2:
                case sMTF_LEVEL3:
                    GLTexMin[i] = GL_LINEAR;
                    GLTexMax[i] = GL_LINEAR;
                    break;
                }
            }
            else
            {
                switch (TFlags[i] & sMTF_LEVELMASK)
                {
                case sMTF_LEVEL0:
                    GLTexMin[i] = GL_NEAREST_MIPMAP_NEAREST;
                    GLTexMax[i] = GL_NEAREST;
                    break;
                case sMTF_LEVEL1:
                    GLTexMin[i] = GL_LINEAR_MIPMAP_NEAREST;
                    GLTexMax[i] = GL_LINEAR;
                    break;
                case sMTF_LEVEL2:
                case sMTF_LEVEL3:
                    GLTexMin[i] = GL_LINEAR_MIPMAP_LINEAR;
                    GLTexMax[i] = GL_LINEAR;
                    break;
                }
            }
            switch (TFlags[i] & sMTF_ADDRMASK_U)
            {
            case sMTF_TILE_U:
                GLTexS[i] = GL_REPEAT;
                break;
            case sMTF_CLAMP_U:
                GLTexS[i] = GL_CLAMP_TO_EDGE;
                break;
            case sMTF_MIRROR_U:
                GLTexS[i] = GL_MIRRORED_REPEAT;
                break;
            default:
                sVERIFYFALSE;
                break;
            }
            switch (TFlags[i] & sMTF_ADDRMASK_V)
            {
            case sMTF_TILE_V:
                GLTexT[i] = GL_REPEAT;
                break;
            case sMTF_CLAMP_V:
                GLTexT[i] = GL_CLAMP_TO_EDGE;
                break;
            case sMTF_MIRROR_V:
                GLTexT[i] = GL_MIRRORED_REPEAT;
                break;
            default:
                sVERIFYFALSE;
                break;
            }
        }
    }

    // blend unit

    if (BlendColor != sMB_OFF)
    {
        const static sInt func[16] =
            {
                0,
                GL_FUNC_ADD,
                GL_FUNC_SUBTRACT,
                GL_FUNC_REVERSE_SUBTRACT,
                GL_MIN_EXT,
                GL_MAX_EXT,
            };
        const static sInt fact[16] =
            {
                0,
                GL_ZERO,
                GL_ONE,
                GL_SRC_COLOR,

                GL_ONE_MINUS_SRC_COLOR,
                GL_SRC_ALPHA,
                GL_ONE_MINUS_SRC_ALPHA,
                GL_DST_COLOR,

                GL_ONE_MINUS_DST_COLOR,
                GL_DST_ALPHA,
                GL_ONE_MINUS_DST_ALPHA,
                GL_SRC_ALPHA_SATURATE,

                0,
                0,
                GL_CONSTANT_COLOR,
                GL_ONE_MINUS_CONSTANT_COLOR,
            };
        GLBlendFactor.InitColor(BlendFactor);
        GLBCFunc = func[(BlendColor & sMBO_MASK) >> 8];
        GLBCSrc = fact[(BlendColor & sMBS_MASK)];
        GLBCDst = fact[(BlendColor & sMBD_MASK) >> 16];
        if (BlendAlpha == sMB_SAMEASCOLOR)
        {
            GLBAFunc = GLBCFunc;
            GLBASrc = GLBCSrc;
            GLBADst = GLBCDst;
        }
        else
        {
            GLBAFunc = func[(BlendAlpha & sMBO_MASK) >> 8];
            GLBASrc = fact[(BlendAlpha & sMBS_MASK)];
            GLBADst = fact[(BlendAlpha & sMBD_MASK) >> 16];
        }
    }
}

void sMaterial::Set(sCBufferBase **cbuffers, sInt cbcount, sInt variant)
{
    GLERR();

    glUseProgram(GLName);
    for (sInt i = 0; i < sMTRL_MAXTEX; i++)
    {
        if (Texture[i])
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, Texture[i]->GLName);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GLTexMin[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GLTexMax[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GLTexS[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GLTexT[i]);
        }
        else
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    GLERR();

    if (BlendColor != sMB_OFF)
    {
        glBlendEquationSeparate(GLBCFunc, GLBAFunc);
        glBlendFuncSeparate(GLBCSrc, GLBCDst, GLBASrc, GLBADst);
        glBlendColor(GLBlendFactor.x, GLBlendFactor.y, GLBlendFactor.z, GLBlendFactor.w);
        glEnable(GL_BLEND);
    }
    else
    {
        glDisable(GL_BLEND);
    }
    GLERR();

    switch (Flags & sMTRL_ZMASK)
    {
    case sMTRL_ZOFF:
        glDisable(GL_DEPTH_TEST);
        break;
    case sMTRL_ZREAD:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_FALSE);
        break;
    case sMTRL_ZWRITE:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);
        glDepthMask(GL_TRUE);
        break;
    case sMTRL_ZON:
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        break;
    }
    switch (Flags & sMTRL_CULLMASK)
    {
    case sMTRL_CULLOFF:
        glCullFace(GL_FRONT_AND_BACK);
        glDisable(GL_CULL_FACE);
        break;
    case sMTRL_CULLON:
        glCullFace(GL_FRONT);
        glEnable(GL_CULL_FACE);
        break;
    case sMTRL_CULLINV:
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);
        break;
    default:
        sVERIFYFALSE;
    }

    for (sInt i = 0; i < cbcount; i++)
    {
        sCBufferBase *cb = cbuffers[i];
        if (cb->RegStart == 0 && cb->RegCount >= 4 && (cb->Slot & sCBUFFER_SHADERMASK) == sCBUFFER_VS)
            glUniform4fv(VSSlot[0], 4, (GLfloat *)(*cb->DataPtr));
    }
    GLERR();
}

void sMaterial::SetStates(sInt var)
{
}
void sMaterial::InitVariants(sInt max)
{
}
void sMaterial::DiscardVariants()
{
}
void sMaterial::SetVariant(sInt var)
{
}

void sGetScreenSafeArea(sF32 &xs, sF32 &ys)
{
    xs = ys = 1;
}

static sGeoBuffer *sFindGeoBuffer(sInt bytes, sInt type, sGeometryDuration duration)
{
    // find available buffer (don't allocate, just find!)

    bytes = sAlign(bytes, 128);
    for (sInt i = 0; i < sGeoBufferCount; i++)
    {
        sGeoBuffer *gb = &sGeoBuffers[i];
        if (gb->Duration == duration && gb->Type == type && gb->Used + bytes <= gb->Alloc)
            return gb;
    }

    // allocate new buffer

    sVERIFY(sGeoBufferCount < sMAX_GEOBUFFER);
    sGeoBuffer *gb = &sGeoBuffers[sGeoBufferCount++];
    switch (type)
    {
    case 0:
        gb->Alloc = sMax(bytes, 1024 * 1024);
        gb->GLType = GL_ARRAY_BUFFER;
        break;
    case 1:
        gb->Alloc = sMax(bytes, 128 * 1024);
        gb->GLType = GL_ELEMENT_ARRAY_BUFFER;
        break;
    default:
        sVERIFYFALSE;
    }
    gb->Used = 0;
    gb->Freed = 0;
    gb->Duration = duration;
    gb->Type = type;
    gb->GLName = 0;
    sVERIFY(bytes <= gb->Alloc);

    // create GL object for buffer

    switch (gb->Duration)
    {
    case sGD_STREAM:
        gb->GLUsage = GL_STREAM_DRAW;
        break;
    case sGD_FRAME:
        gb->GLUsage = GL_DYNAMIC_DRAW;
        break;
    case sGD_STATIC:
        gb->GLUsage = GL_STATIC_DRAW;
        break;
    default:
        sVERIFYFALSE;
    }
    glGenBuffers(1, &gb->GLName);
    GLERR();
    glBindBuffer(gb->GLType, gb->GLName);
    glBufferData(gb->GLType, gb->Alloc, 0, gb->GLUsage);
    GLERR();

    // done

    return gb;
}

/*sGeoBufferPart::sGeoBufferPart()
{

  sFatal(L"sGeoBufferPart::sGeoBufferPart() is not implemented");
  exit(1);
}

sGeoBufferPart::~sGeoBufferPart() {
  sFatal(L"sGeoBufferPart::~sGeoBufferPart() is not implemented");
  exit(1);
}*/
sGeoBufferPart::sGeoBufferPart()
{
    Buffer = 0;
    Start = 0;
    Count = 0;
}

sGeoBufferPart::~sGeoBufferPart()
{
    Clear();
}

void sGeoBufferPart::Clear()
{
    if (Buffer)
    {
        Buffer->Freed += Count;
        if (Buffer->Freed == Buffer->Used && Buffer->Duration == sGD_STATIC)
            Buffer->Used = Buffer->Freed = 0;
        Buffer = 0;
    }
    Start = 0;
    Count = 0;
}

void sGeoBufferPart::Init(sInt count, sInt size, sGeometryDuration duration, sInt buffertype)
{
    Buffer = sFindGeoBuffer(size * count, buffertype, duration);
    Start = Buffer->Used;
    Count = count;
}

void sGeoBufferPart::Lock(void **ptr)
{
    glBindBuffer(Buffer->GLType, Buffer->GLName);
    sU8 *data = (sU8 *)glMapBuffer(Buffer->GLType, GL_WRITE_ONLY);
    GLERR();
    glBindBuffer(Buffer->GLType, 0);
    data += Start;
    *ptr = data;
}

void sGeoBufferPart::Unlock(sInt count, sInt size)
{
    if (count != -1)
        Count = count;
    Buffer->Used += sAlign(Count * size, 128);
    sVERIFY(Buffer->Used <= Buffer->Alloc);
    glBindBuffer(Buffer->GLType, Buffer->GLName);
    glUnmapBuffer(Buffer->GLType);
    GLERR();
    glBindBuffer(Buffer->GLType, 0);
}

void sSetRenderClipping(sRect *r, sInt count)
{
}

/****************************************************************************/
/***                                                                      ***/
/***                                                                      ***/
/***                                                                      ***/
/****************************************************************************/
