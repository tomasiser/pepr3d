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

/**
 * Modified for Pepr3D
 **/

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "cinder/CinderAssert.h"
#include "cinder/Color.h"
#include "cinder/Filesystem.h"
#include "cinder/Noncopyable.h"
#include "cinder/Signals.h"
#include "cinder/Vector.h"
#include "cinder/app/App.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vao.h"
#include "cinder/gl/Vbo.h"
#include "cinder/gl/Fbo.h"

#include "peprassert.h"

// Custom implicit cast operators
#ifndef CINDER_IMGUI_NO_IMPLICIT_CASTS
#define IM_VEC2_CLASS_EXTRA          \
    ImVec2(const glm::vec2& f) {     \
        x = f.x;                     \
        y = f.y;                     \
    }                                \
    operator glm::vec2() const {     \
        return glm::vec2(x, y);      \
    }                                \
    ImVec2(const glm::ivec2& f) {    \
        x = static_cast<float>(f.x); \
        y = static_cast<float>(f.y); \
    }                                \
    operator glm::ivec2() const {    \
        return glm::ivec2(x, y);     \
    }

#define IM_VEC4_CLASS_EXTRA            \
    ImVec4(const glm::vec4& f) {       \
        x = f.x;                       \
        y = f.y;                       \
        z = f.z;                       \
        w = f.w;                       \
    }                                  \
    operator glm::vec4() const {       \
        return glm::vec4(x, y, z, w);  \
    }                                  \
    ImVec4(const ci::ColorA& f) {      \
        x = f.r;                       \
        y = f.g;                       \
        z = f.b;                       \
        w = f.a;                       \
    }                                  \
    operator ci::ColorA() const {      \
        return ci::ColorA(x, y, z, w); \
    }                                  \
    ImVec4(const ci::Color& f) {       \
        x = f.r;                       \
        y = f.g;                       \
        z = f.b;                       \
        w = 1.0f;                      \
    }                                  \
    operator ci::Color() const {       \
        return ci::Color(x, y, z);     \
    }
#endif

#include "imgui.h"

#define CINDER_IMGUI_TEXTURE_UNIT 8

// Pepr3D custom namespace (to prevent mess in foreign namespaces)
namespace peprimgui {

// forward declarations
typedef std::shared_ptr<class cinder::app::Window> WindowRef;
typedef std::shared_ptr<class cinder::gl::Texture2d> Texture2dRef;

// Cinder/ImGui renderer
class Renderer {
   public:
    Renderer();

    //! renders imgui drawlist
    void render(ImDrawData* draw_data);

    //! initializes and returns the font texture
    Texture2dRef getFontTextureRef();
    //! initializes and returns the vao
    ci::gl::VaoRef getVao();
    //! initializes and returns the vbo
    ci::gl::VboRef getVbo();
    //! initializes and returns the shader
    ci::gl::GlslProgRef getGlslProg();

    //! initializes the font texture
    void initFontTexture();
    //! initializes the vbo mesh
    void initBuffers(size_t size = 1000);
    //! initializes the shader
    void initGlslProg();

   protected:
    Texture2dRef mFontTexture;
    ci::gl::VaoRef mVao;
    ci::gl::VboRef mVbo;
    ci::gl::VboRef mIbo;
    ci::gl::GlslProgRef mShader;
};

using RendererRef = std::shared_ptr<Renderer>;

// Pepr3D custom wrapper (to prevent static variables)
class PeprImGui {
    RendererRef mRenderer;
    ci::signals::ConnectionList mWindowConnections;
    ci::signals::ConnectionList mAppConnections;
    bool mNewFrame = false;
    std::vector<int> mAccelKeys;
    ci::Timer mTimer;
    std::string mIniFilename;
    ci::gl::FboRef mFramebuffer;

   public:
    void setup(cinder::app::AppBase* application, WindowRef window);
    void connectWindow(WindowRef window);
    void disconnectWindow(WindowRef window);

    void refreshFontTexture();

    void useFramebuffer(ci::gl::FboRef framebuffer) {
        mFramebuffer = framebuffer;
    }

   private:
    void mouseDown(ci::app::MouseEvent& event);
    void mouseMove(ci::app::MouseEvent& event);
    void mouseDrag(ci::app::MouseEvent& event);
    void mouseUp(ci::app::MouseEvent& event);
    void mouseWheel(ci::app::MouseEvent& event);
    void keyDown(ci::app::KeyEvent& event);
    void keyUp(ci::app::KeyEvent& event);
    void newFrameGuard();
    void render();
    void resize();
    void resetKeys();
};

}  // namespace peprimgui