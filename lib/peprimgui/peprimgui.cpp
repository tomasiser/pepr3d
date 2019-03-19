// clang-format off
/*
 Cinder-ImGui
 This code is intended for use with Cinder
 and Omar Cornut ImGui C++ libraries.
 
 http://libcinder.org
 https://github.com/ocornut
 
 Copyright (c) 2013-2015, Simon Geilfus - All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
// clang-format on

// Linux fixes thanks to the help of @petros, see this thread:
// https://forum.libcinder.org/#Topic/23286000002634083

/**
 * Modified for Pepr3D
 **/

#include "peprimgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_PLACEMENT_NEW
#include "imgui_internal.h"

#include "cinder/CinderAssert.h"
#include "cinder/Clipboard.h"
#include "cinder/Log.h"
#include "cinder/app/App.h"
#include "cinder/gl/Context.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/draw.h"
#include "cinder/gl/scoped.h"

using namespace std;
using namespace ci;
using namespace ci::app;

namespace peprimgui {

//! cinder renderer
Renderer::Renderer() {
    initGlslProg();
    initBuffers();
}

#define USE_MAP_BUFFER

//! renders imgui drawlist
void Renderer::render(ImDrawData* draw_data) {
    const float width = ImGui::GetIO().DisplaySize.x;
    const float height = ImGui::GetIO().DisplaySize.y;
    const auto& vao = getVao();
    const auto& vbo = getVbo();
    const auto& shader = getGlslProg();
    auto ctx = gl::context();

    const mat4 mat = {
        {2.0f / width, 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / -height, 0.0f, 0.0f},
        {0.0f, 0.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 0.0f, 1.0f},
    };

    gl::ScopedVao scopedVao(vao.get());
    gl::ScopedBuffer scopedVbo(GL_ARRAY_BUFFER, vbo->getId());
    gl::ScopedBuffer scopedIbo(GL_ELEMENT_ARRAY_BUFFER, mIbo->getId());
    gl::ScopedGlslProg scopedShader(shader);
    gl::ScopedDepth scopedDepth(false);
    gl::ScopedBlendAlpha scopedBlend;
    gl::ScopedFaceCulling scopedFaceCulling(false);
    shader->uniform("uModelViewProjection", mat);

    GLuint currentTextureId = 0;
    ctx->pushTextureBinding(GL_TEXTURE_2D, CINDER_IMGUI_TEXTURE_UNIT);
    ctx->pushBoolState(GL_SCISSOR_TEST, GL_TRUE);
    ctx->pushScissor();
    for(int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawIdx* idx_buffer_offset = 0;

#if defined(USE_MAP_BUFFER)
        // grow vertex buffer if needed
        int needed_vtx_size = cmd_list->VtxBuffer.size() * sizeof(ImDrawVert);
        if(vbo->getSize() < needed_vtx_size) {
            GLsizeiptr size = needed_vtx_size + 2000 * sizeof(ImDrawVert);
#ifndef CINDER_LINUX_EGL_RPI2
            mVbo->bufferData(size, nullptr, GL_STREAM_DRAW);
#else
            mVbo->bufferData(size, nullptr, GL_DYNAMIC_DRAW);
#endif
        }

        // update vertex data
        {
            ImDrawVert* vtx_data = static_cast<ImDrawVert*>(vbo->mapReplace());
            if(!vtx_data)
                continue;
            memcpy(vtx_data, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
            vbo->unmap();
        }

        // grow index buffer if needed
        int needed_idx_size = cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
        if(mIbo->getSize() < needed_idx_size) {
            GLsizeiptr size = needed_idx_size + 2000 * sizeof(ImDrawIdx);
#if !defined(CINDER_LINUX_EGL_RPI2)
            mIbo->bufferData(size, nullptr, GL_STREAM_DRAW);
#else
            mIbo->bufferData(size, nullptr, GL_DYNAMIC_DRAW);
#endif
        }

        // update index data
        {
            ImDrawIdx* idx_data = static_cast<ImDrawIdx*>(mIbo->mapReplace());
            if(!idx_data)
                continue;
            memcpy(idx_data, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
            mIbo->unmap();
        }
#else
#if !defined(CINDER_LINUX_EGL_RPI2)
        mVbo->bufferData((GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                         (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        mIbo->bufferData((GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                         (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);
#else
        mVbo->bufferData((GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert),
                         (const GLvoid*)cmd_list->VtxBuffer.Data, GL_DYNAMIC_DRAW);
        mIbo->bufferData((GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx),
                         (const GLvoid*)cmd_list->IdxBuffer.Data, GL_DYNAMIC_DRAW);
#endif
#endif

        // issue draw commands
        for(int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if(pcmd->UserCallback) {
                pcmd->UserCallback(cmd_list, pcmd);
            } else {
                bool pushTexture = currentTextureId != (GLuint)(intptr_t)pcmd->TextureId;
                if(pushTexture) {
                    currentTextureId = (GLuint)(intptr_t)pcmd->TextureId;
                    ctx->bindTexture(GL_TEXTURE_2D, currentTextureId, CINDER_IMGUI_TEXTURE_UNIT);
                }
                ctx->setScissor(
                    {ivec2((int)pcmd->ClipRect.x, (int)(height - pcmd->ClipRect.w)),
                     ivec2((int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y))});
                gl::drawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                                 sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset);
            }
            idx_buffer_offset += pcmd->ElemCount;
        }
    }

    ctx->popScissor();
    ctx->popBoolState(GL_SCISSOR_TEST);
    ctx->popTextureBinding(GL_TEXTURE_2D, CINDER_IMGUI_TEXTURE_UNIT);
}

//! initializes and returns the font texture
gl::TextureRef Renderer::getFontTextureRef() {
    if(!mFontTexture) {
        initFontTexture();
    }
    return mFontTexture;
}
//! initializes and returns the vbo mesh
gl::VaoRef Renderer::getVao() {
    if(!mVao) {
        initBuffers();
    }
    return mVao;
}

//! initializes and returns the vbo
gl::VboRef Renderer::getVbo() {
    if(!mVbo) {
        initBuffers();
    }
    return mVbo;
}

//! initializes the vbo mesh
void Renderer::initBuffers(size_t size) {
#if !defined(CINDER_LINUX_EGL_RPI2)
    mVbo = gl::Vbo::create(GL_ARRAY_BUFFER, size, nullptr, GL_STREAM_DRAW);
    mIbo = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, 10, nullptr, GL_STREAM_DRAW);
#else
    mVbo = gl::Vbo::create(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    mIbo = gl::Vbo::create(GL_ELEMENT_ARRAY_BUFFER, 10, nullptr, GL_DYNAMIC_DRAW);
#endif
    mVao = gl::Vao::create();

    gl::ScopedVao mVaoScope(mVao);
    gl::ScopedBuffer mVboScope(mVbo);

    gl::enableVertexAttribArray(0);
    gl::enableVertexAttribArray(1);
    gl::enableVertexAttribArray(2);

    gl::vertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (const GLvoid*)offsetof(ImDrawVert, pos));
    gl::vertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (const GLvoid*)offsetof(ImDrawVert, uv));
    gl::vertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                            (const GLvoid*)offsetof(ImDrawVert, col));
}
//! initalizes the font texture
void Renderer::initFontTexture() {
    unsigned char* pixels;
    int width, height;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    mFontTexture = gl::Texture::create(pixels, GL_RGBA, width, height,
                                       gl::Texture::Format().magFilter(GL_LINEAR).minFilter(GL_LINEAR));
    ImGui::GetIO().Fonts->ClearTexData();
    ImGui::GetIO().Fonts->TexID = (void*)(intptr_t)mFontTexture->getId();
}

//! initalizes and returns the shader
gl::GlslProgRef Renderer::getGlslProg() {
    if(!mShader) {
        initGlslProg();
    }
    return mShader;
}

//! initalizes the shader
void Renderer::initGlslProg() {
    try {
        mShader = gl::GlslProg::create(gl::GlslProg::Format()
                                           .vertex(
#if defined(CINDER_GL_ES_2)
                                               R"(
						       precision highp float;
						       uniform mat4 uModelViewProjection;
						       
						       attribute vec2      iPosition;
						       attribute vec2      iUv;
						       attribute vec4      iColor;
						       
						       varying vec2     vUv;
						       varying vec4     vColor;
						       
						       void main() {
							       vColor       = iColor;
							       vUv          = iUv;
							       gl_Position  = uModelViewProjection * vec4( iPosition, 0.0, 1.0 );
						       } )"
#elif defined(CINDER_GL_ES_3)
                                               R"(
						#version 300 es
					       precision highp float;
					       uniform mat4 uModelViewProjection;
					       
					       in vec2      iPosition;
					       in vec2      iUv;
					       in vec4      iColor;
					       
					       out vec2     vUv;
					       out vec4     vColor;
					       
					       void main() {
						       vColor       = iColor;
						       vUv          = iUv;
						       gl_Position  = uModelViewProjection * vec4( iPosition, 0.0, 1.0 );
					       } )"
#else
                                               R"(
						#version 150
						uniform mat4 uModelViewProjection;
						in vec2      iPosition;
						in vec2      iUv;
						in vec4      iColor;
						out vec2     vUv;
						out vec4     vColor;
						void main() {
							vColor       = iColor;
							vUv          = iUv;
							gl_Position  = uModelViewProjection * vec4( iPosition, 0.0, 1.0 );
						} )"

#endif
                                               )
                                           .fragment(
#if defined(CINDER_GL_ES_2)
                                               R"(
			  precision highp float;
			  
			  varying highp vec2	vUv;
			  varying highp vec4	vColor;
			  uniform sampler2D	uTex;
			  
			  void main() {
				  vec4 color = texture2D( uTex, vUv ) * vColor;
				  gl_FragColor = color;
			  }  )"
#elif defined(CINDER_GL_ES_3)
                                               R"(
		#version 300 es
		precision highp float;
		
		in highp vec2		vUv;
		in highp vec4		vColor;
		out highp vec4		oColor;
		uniform sampler2D	uTex;
		
		void main() {
			vec4 color = texture( uTex, vUv ) * vColor;
			oColor = color;
		}  )"
#else
                                               R"(
		#version 150
		
		in vec2			vUv;
		in vec4			vColor;
		out vec4		oColor;
		uniform sampler2D	uTex;
		
		void main() {
			vec4 color = texture( uTex, vUv ) * vColor;
			oColor = color;
		}  )"

#endif
                                               )
                                           .attribLocation("iPosition", 0)
                                           .attribLocation("iUv", 1)
                                           .attribLocation("iColor", 2));

        mShader->uniform("uTex", CINDER_IMGUI_TEXTURE_UNIT);

    } catch(gl::GlslProgCompileExc exc) {
        CI_LOG_E("Problem Compiling ImGui::Renderer shader " << exc.what());
    }
}

//! sets the right mouseDown IO values in imgui
void PeprImGui::mouseDown(ci::app::MouseEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = toPixels(event.getPos());
    if(event.isLeftDown()) {
        io.MouseDown[0] = true;
        io.MouseDown[1] = false;
    } else if(event.isRightDown()) {
        io.MouseDown[0] = false;
        io.MouseDown[1] = true;
    }

    event.setHandled(io.WantCaptureMouse);
}
//! sets the right mouseMove IO values in imgui
void PeprImGui::mouseMove(ci::app::MouseEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = toPixels(event.getPos());

    event.setHandled(io.WantCaptureMouse);
}
//! sets the right mouseDrag IO values in imgui
void PeprImGui::mouseDrag(ci::app::MouseEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = toPixels(event.getPos());

    event.setHandled(io.WantCaptureMouse);
}
//! sets the right mouseDrag IO values in imgui
void PeprImGui::mouseUp(ci::app::MouseEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[0] = false;
    io.MouseDown[1] = false;

    event.setHandled(io.WantCaptureMouse);
}
//! sets the right mouseWheel IO values in imgui
void PeprImGui::mouseWheel(ci::app::MouseEvent& event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += event.getWheelIncrement();

    event.setHandled(io.WantCaptureMouse);
}

//! sets the right keyDown IO values in imgui
void PeprImGui::keyDown(ci::app::KeyEvent& event) {
    ImGuiIO& io = ImGui::GetIO();

#if defined CINDER_LINUX
    auto character = event.getChar();
#else
    uint32_t character = event.getCharUtf32();
#endif

    io.KeysDown[event.getCode()] = true;

    if(!event.isAccelDown() && character > 0 && character <= 255) {
        io.AddInputCharacter((char)character);
    } else if(event.getCode() != KeyEvent::KEY_LMETA && event.getCode() != KeyEvent::KEY_RMETA && event.isAccelDown() &&
              find(mAccelKeys.begin(), mAccelKeys.end(), event.getCode()) == mAccelKeys.end()) {
        mAccelKeys.push_back(event.getCode());
    }

    io.KeyCtrl = io.KeysDown[KeyEvent::KEY_LCTRL] || io.KeysDown[KeyEvent::KEY_RCTRL];
    io.KeyShift = io.KeysDown[KeyEvent::KEY_LSHIFT] || io.KeysDown[KeyEvent::KEY_RSHIFT];
    io.KeyAlt = io.KeysDown[KeyEvent::KEY_LALT] || io.KeysDown[KeyEvent::KEY_RALT];
    io.KeySuper = io.KeysDown[KeyEvent::KEY_LMETA] || io.KeysDown[KeyEvent::KEY_RMETA] ||
                  io.KeysDown[KeyEvent::KEY_LSUPER] || io.KeysDown[KeyEvent::KEY_RSUPER];

    event.setHandled(io.WantCaptureKeyboard);
}
//! sets the right keyUp IO values in imgui
void PeprImGui::keyUp(ci::app::KeyEvent& event) {
    ImGuiIO& io = ImGui::GetIO();

    io.KeysDown[event.getCode()] = false;

    for(auto key : mAccelKeys) {
        io.KeysDown[key] = false;
    }
    mAccelKeys.clear();

    io.KeyCtrl = io.KeysDown[KeyEvent::KEY_LCTRL] || io.KeysDown[KeyEvent::KEY_RCTRL];
    io.KeyShift = io.KeysDown[KeyEvent::KEY_LSHIFT] || io.KeysDown[KeyEvent::KEY_RSHIFT];
    io.KeyAlt = io.KeysDown[KeyEvent::KEY_LALT] || io.KeysDown[KeyEvent::KEY_RALT];
    io.KeySuper = io.KeysDown[KeyEvent::KEY_LMETA] || io.KeysDown[KeyEvent::KEY_RMETA] ||
                  io.KeysDown[KeyEvent::KEY_LSUPER] || io.KeysDown[KeyEvent::KEY_RSUPER];

    event.setHandled(io.WantCaptureKeyboard);
}

void PeprImGui::render() {
    if(mTimer.isStopped()) {
        mTimer.start();
    }
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = static_cast<float>(mTimer.getSeconds());

    if(io.DeltaTime < 0.0f) {
        CI_LOG_W("WARNING: overriding imgui deltatime because it is " << io.DeltaTime);
        io.DeltaTime = 1.0f / 60.0f;
    }

    mTimer.start();
    ImGui::Render();
    if(mFramebuffer != nullptr) {
        mFramebuffer->bindFramebuffer();
    }
    mRenderer->render(ImGui::GetDrawData());
    if(mFramebuffer != nullptr) {
        mFramebuffer->unbindFramebuffer();
        ci::gl::draw(mFramebuffer->getTexture2d(GL_COLOR_ATTACHMENT0));
    }
    mNewFrame = false;
    App::get()->dispatchAsync([this]() { newFrameGuard(); });
}

void PeprImGui::newFrameGuard() {
    if(!mNewFrame) {
        ImGui::NewFrame();
        mNewFrame = true;
    }
}

void PeprImGui::resize() {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = toPixels(getWindowSize());
    newFrameGuard();
}

void PeprImGui::resetKeys() {
    for(int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().KeysDown); i++) {
        ImGui::GetIO().KeysDown[i] = false;
    }
    for(int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().KeysDownDuration); i++) {
        ImGui::GetIO().KeysDownDuration[i] = 0.0f;
    }
    for(int i = 0; i < IM_ARRAYSIZE(ImGui::GetIO().KeysDownDurationPrev); i++) {
        ImGui::GetIO().KeysDownDurationPrev[i] = 0.0f;
    }
    ImGui::GetIO().KeyCtrl = false;
    ImGui::GetIO().KeyShift = false;
    ImGui::GetIO().KeyAlt = false;
}

void PeprImGui::setup(cinder::app::AppBase* application, WindowRef window) {
    P_ASSERT(window);

    ImGuiContext* context = ImGui::CreateContext();

    // get the window and switch to its context before initializing the renderer
    auto currentContext = gl::context();
    window->getRenderer()->makeCurrentContext();
    mRenderer = std::make_shared<Renderer>();

    // set io and keymap
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = window->getSize();
    io.DeltaTime = 1.0f / 60.0f;
    io.KeyMap[ImGuiKey_Tab] = KeyEvent::KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = KeyEvent::KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = KeyEvent::KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = KeyEvent::KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = KeyEvent::KEY_DOWN;
    io.KeyMap[ImGuiKey_Home] = KeyEvent::KEY_HOME;
    io.KeyMap[ImGuiKey_End] = KeyEvent::KEY_END;
    io.KeyMap[ImGuiKey_Delete] = KeyEvent::KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = KeyEvent::KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter] = KeyEvent::KEY_RETURN;
    io.KeyMap[ImGuiKey_Escape] = KeyEvent::KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A] = KeyEvent::KEY_a;
    io.KeyMap[ImGuiKey_C] = KeyEvent::KEY_c;
    io.KeyMap[ImGuiKey_V] = KeyEvent::KEY_v;
    io.KeyMap[ImGuiKey_X] = KeyEvent::KEY_x;
    io.KeyMap[ImGuiKey_Y] = KeyEvent::KEY_y;
    io.KeyMap[ImGuiKey_Z] = KeyEvent::KEY_z;
    io.KeyMap[ImGuiKey_Insert] = KeyEvent::KEY_INSERT;
    io.KeyMap[ImGuiKey_Space] = KeyEvent::KEY_SPACE;

    // setup config file path
    mIniFilename = (getAssetPath("") / "imgui.ini").string();
    io.IniFilename = mIniFilename.c_str();

    // setup fonts
    ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;
    fontAtlas->Clear();
    mRenderer->initFontTexture();

#ifndef CINDER_LINUX
    // clipboard callbacks
    io.SetClipboardTextFn = [](void* user_data, const char* text) {
        // clipboard text is already zero-terminated
        Clipboard::setString(text);
    };
    io.GetClipboardTextFn = [](void* user_data) {
        static std::string clipboardText;
        clipboardText = Clipboard::getString();
        return (const char*)&clipboardText.c_str()[0];
    };
#endif

    // connect window's signals
    disconnectWindow(window);
    connectWindow(window);

    mWindowConnections += (window->getSignalDraw().connect([this]() { newFrameGuard(); }));
    mWindowConnections += (window->getSignalPostDraw().connect([this]() { render(); }));

    // connect app's signals
    mAppConnections += application->getSignalDidBecomeActive().connect([this]() { resetKeys(); });
    mAppConnections += application->getSignalWillResignActive().connect([this]() { resetKeys(); });
    mAppConnections += application->getSignalCleanup().connect([context, this]() {
        mAppConnections.clear();

        ImGui::DestroyContext(context);
    });

    // switch back to the original gl context
    currentContext->makeCurrent();
}

void PeprImGui::connectWindow(WindowRef window) {
    mWindowConnections +=
        window->getSignalMouseDown().connect([this](ci::app::MouseEvent& event) { mouseDown(event); });
    mWindowConnections += window->getSignalMouseUp().connect([this](ci::app::MouseEvent& event) { mouseUp(event); });
    mWindowConnections +=
        window->getSignalMouseDrag().connect([this](ci::app::MouseEvent& event) { mouseDrag(event); });
    mWindowConnections +=
        window->getSignalMouseMove().connect([this](ci::app::MouseEvent& event) { mouseMove(event); });
    mWindowConnections +=
        window->getSignalMouseWheel().connect([this](ci::app::MouseEvent& event) { mouseWheel(event); });
    mWindowConnections += window->getSignalKeyDown().connect([this](ci::app::KeyEvent& event) { keyDown(event); });
    mWindowConnections += window->getSignalKeyUp().connect([this](ci::app::KeyEvent& event) { keyUp(event); });
    mWindowConnections += window->getSignalResize().connect([this]() { resize(); });
}

void PeprImGui::disconnectWindow(WindowRef window) {
    mWindowConnections.clear();
}

void PeprImGui::refreshFontTexture() {
    mRenderer->initFontTexture();
}

}  // namespace peprimgui