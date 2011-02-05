%module(directors="1") refinery

%{

#include "refinery/color.h"
#include "refinery/exif.h"
#include "refinery/camera.h"
#include "refinery/filters.h"
#include "refinery/image.h"
#include "refinery/image_tile.h"
#include "refinery/interpolate.h"
#include "refinery/output.h"
#include "refinery/unpack.h"

%}

%include "file.i"
%include "exception.i"
%include "std_string.i"
%include "std_vector.i"

%exception {
  try {
    $action
  }
  SWIG_CATCH_STDEXCEPT
  catch (Swig::DirectorException &e) {
    SWIG_fail; /* for Python exceptions coming back to Python */
  }
  catch (...) {
    SWIG_exception_fail(SWIG_UnknownError, "Unknown exception");
  }
}

%feature("autodoc", "1");
%feature("director") refinery::ExifData;
%feature("director") refinery::InMemoryExifData;

namespace std {
  %template(byte_vector) vector<unsigned char>;
};

%ignore refinery::ImageReader::readGrayImage(std::streambuf&, const char*, int, int, const ExifData&);
%ignore refinery::ImageReader::readRgbImage(std::streambuf&, const char*, int, int, const ExifData&);

%warnfilter(325) refinery::Camera::ColorConversionData;
%include "refinery/camera.h"

%include "refinery/exif.h"

%warnfilter(389) refinery::RGBPixel::operator[];
%include "refinery/image.h"
%warnfilter(302) refinery::Image;
%warnfilter(302) refinery::GrayImage;

%include "refinery/color.h"

%include "refinery/filters.h"
%extend refinery::ScaleColorsFilter {
  %template(filter) filter<GrayImage>;
};
%extend refinery::ConvertToRgbFilter {
  %template(filter) filter<Image>;
};

%include "refinery/image_tile.h"

%include "refinery/interpolate.h"

%include "refinery/output.h"

%include "refinery/unpack.h"
