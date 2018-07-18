#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/TextureFont.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class Pepr3D : public App {
   public:
    void setup() override;
    void mouseDown(MouseEvent event) override;
    void update() override;
    void draw() override;

   private:
    Font mFont;
    gl::TextureFontRef mTextureFont;
};

void Pepr3D::setup() {
    setWindowSize(640, 480);
    mFont = Font("Arial", 48);
    mTextureFont = gl::TextureFont::create(mFont);
}

void Pepr3D::mouseDown(MouseEvent event) {}

void Pepr3D::update() {}

void Pepr3D::draw() {
    gl::clear(Color(0.1f, 0.1f, 0.1f));
    mTextureFont->drawString("Hello! This is Pepr3D!", vec2(20.f, 50.f));
}

CINDER_APP(Pepr3D, RendererGl)