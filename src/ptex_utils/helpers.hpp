#pragma once

#include <memory>
#include <string>

#include <Ptexture.h>

std::string strbasename(const char* s);


template <typename T>
struct releaser {
    void operator()(T *r) const {
        r->release();
    }
};

using MetaPtr = std::unique_ptr<PtexMetaData, releaser<PtexMetaData> >;
using PtxPtr = std::unique_ptr<PtexTexture, releaser<PtexTexture> >;
