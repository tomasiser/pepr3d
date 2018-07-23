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
    void fileDrop(FileDropEvent event) override;
    void update() override;
    void draw() override;

   private:
    Font mFont;
    gl::TextureFontRef mTextureFont;
    std::string mDroppedFiles;
    vec2 mFileDropPosition;
    bool mFilesAlreadyDropped;
};

void Pepr3D::setup() {
    setWindowSize(640, 480);
    mFont = Font("Arial", 28);
    mTextureFont = gl::TextureFont::create(mFont);
    mFilesAlreadyDropped = false;
}

void Pepr3D::mouseDown(MouseEvent event) {}

void Pepr3D::fileDrop(FileDropEvent event) {
    mDroppedFiles = "";
    for(auto path : event.getFiles()) {
        mDroppedFiles += path.u8string();
        mDroppedFiles += '\n';
        mFilesAlreadyDropped = true;
    }
    mFileDropPosition = vec2(event.getX(), event.getY());
}

void Pepr3D::update() {}

void Pepr3D::draw() {
    gl::clear(Color(0.1f, 0.1f, 0.1f));
    mTextureFont->drawString("Hello! This is Pepr3D!", vec2(20.f, 40.f));

    if(!mFilesAlreadyDropped) {
        mTextureFont->drawString("Drag & drop some files!", vec2(20.f, 90.f));
    } else {
        mTextureFont->drawString("Files dropped at (" + std::to_string(mFileDropPosition.x) + "," +
                                     std::to_string(mFileDropPosition.y) + "):\n" + mDroppedFiles,
                                 vec2(20.f, 90.f));
    }
}

CINDER_APP(Pepr3D, RendererGl)