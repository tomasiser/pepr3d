#pragma once

#include "cinder/Log.h"

namespace pepr3d {

class FatalLogger : public ci::log::LoggerFile {
   public:
    FatalLogger(const ci::fs::path& filePath, bool appendToExisting) : LoggerFile(filePath, appendToExisting) {}

    virtual void write(const ci::log::Metadata& meta, const std::string& text) override {
        if(meta.mLevel != ci::log::Level::LEVEL_FATAL) {
            return;
        }

        LoggerFile::write(meta, "A fatal error has occured and Pepr3D will terminate.");
    }
};

};  // namespace pepr3d
