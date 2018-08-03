// __BEGIN_LICENSE__
//  Copyright (c) 2006-2013, United States Government as represented by the
//  Administrator of the National Aeronautics and Space Administration. All
//  rights reserved.
//
//  The NASA Vision Workbench is licensed under the Apache License,
//  Version 2.0 (the "License"); you may not use this file except in
//  compliance with the License. You may obtain a copy of the License at
//  http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
// __END_LICENSE__


#include <vw/config.h>
#include <vw/FileIO/MemoryImageResource.h>

#if defined(VW_HAVE_PKG_JPEG) && VW_HAVE_PKG_JPEG==1
#  include <vw/FileIO/MemoryImageResourceJPEG.h>
#endif

#if defined(VW_HAVE_PKG_PNG) && VW_HAVE_PKG_PNG==1
#  include <vw/FileIO/MemoryImageResourcePNG.h>
#endif

#if defined(VW_HAVE_PKG_GDAL) && VW_HAVE_PKG_GDAL==1
#  include <vw/FileIO/MemoryImageResourceGDAL.h>
#endif

#if defined(VW_HAVE_PKG_OPENEXR) && VW_HAVE_PKG_OPENEXR==1
#  include <vw/FileIO/MemoryImageResourceOpenEXR.h>
#endif

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/trim.hpp>


namespace {
   std::string clean_type(const std::string& type) {
     return boost::to_lower_copy(boost::trim_left_copy_if(type, boost::is_any_of(".")));
   }
}

namespace vw {

  SrcMemoryImageResource* SrcMemoryImageResource::open( const std::string& type, const uint8* data, size_t len ) {
    boost::shared_array<const uint8> p(data, NOP());
    return SrcMemoryImageResource::open(type, p, len);
  }

  SrcMemoryImageResource* SrcMemoryImageResource::open( const std::string& type,
                                                        boost::shared_array<const uint8> data, size_t len ) {
    const std::string ct = clean_type(type);
    SrcMemoryImageResource* result = 0;
#if defined(VW_HAVE_PKG_JPEG) && VW_HAVE_PKG_JPEG==1
    if (ct == "jpg" || ct == "jpeg" || ct == "image/jpeg")
      result = new SrcMemoryImageResourceJPEG(data, len);
#endif
#if defined(VW_HAVE_PKG_PNG) && VW_HAVE_PKG_PNG==1
    if (ct == "png" || ct == "image/png")
      result = new SrcMemoryImageResourcePNG(data, len);
#endif
#if defined(VW_HAVE_PKG_GDAL) && VW_HAVE_PKG_GDAL==1
    if (ct == "tif" || ct == "tiff" || ct == "image/tiff")
      result = new SrcMemoryImageResourceGDAL(data, len);
#endif
#if defined(VW_HAVE_PKG_OPENEXR) && VW_HAVE_PKG_OPENEXR==1
    if (ct == "exr" || ct == "image/exr")
      result = new SrcMemoryImageResourceOpenEXR(data, len);
#endif
    if (result == 0)
      vw_throw( NoImplErr() << "Unsupported file format: " << type );
    return result;
  }

  DstMemoryImageResource* DstMemoryImageResource::create( const std::string& type, const ImageFormat& format ) {

    const std::string ct = clean_type(type);
    DstMemoryImageResource* result = 0;
#if defined(VW_HAVE_PKG_JPEG) && VW_HAVE_PKG_JPEG==1
    if (ct == "jpg" || ct == "jpeg" || ct == "image/jpeg")
      result = new DstMemoryImageResourceJPEG(format);
#endif
#if defined(VW_HAVE_PKG_PNG) && VW_HAVE_PKG_PNG==1
    if (ct == "png" || ct == "image/png")
      result = new DstMemoryImageResourcePNG(format);
#endif
#if defined(VW_HAVE_PKG_GDAL) && VW_HAVE_PKG_GDAL==1
    if (ct == "tif" || ct == "tiff" || ct == "image/tiff")
      result = new DstMemoryImageResourceGDAL(format);
#endif
#if defined(VW_HAVE_PKG_OPENEXR) && VW_HAVE_PKG_OPENEXR==1
    if (ct == "exr" || ct == "image/exr")
      result = new DstMemoryImageResourceOpenEXR(format);
#endif
    
    if (result == 0)
      vw_throw( NoImplErr() << "Unsupported file format: " << type );
    return result;
  }

} // namespace vw
