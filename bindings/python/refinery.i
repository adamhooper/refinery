%module(directors="1") refinery

%{

#include <limits>

#include "refinery/color.h"
#include "refinery/exif.h"
#include "refinery/camera.h"
#include "refinery/filters.h"
#include "refinery/gamma.h"
#include "refinery/histogram.h"
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
%warnfilter(302) refinery::RGBImage;
%warnfilter(302) refinery::GrayImage;
%extend refinery::TypedImage {
  void fillRgb8(char* rgb8) const {
    typedef T PixelType;
    typedef typename PixelType::ValueType ValueType;
    typedef typename PixelType::ColorType ColorType;

    const unsigned int shift =
      std::numeric_limits<ValueType>::digits
      + std::numeric_limits<ValueType>::is_signed
      - std::numeric_limits<unsigned char>::digits;

    const PixelType* i = $self->constPixels();
    const PixelType* lastPixel = $self->constPixelsEnd();

    for (char* o = rgb8; i < lastPixel; i++, o += 3) {
      for (ColorType c = 0; c < PixelType::NColors; c++) {
        o[c] = static_cast<unsigned char>(i->at(c) >> shift);
      }
    }
  }
};
%template(RGBImage) refinery::TypedImage<refinery::u16RGBPixel>;
%template(GrayImage) refinery::TypedImage<refinery::u16GrayPixel>;

%include "refinery/color.h"

%include "refinery/filters.h"
%extend refinery::ScaleColorsFilter {
  %template(filter) filter<refinery::GrayImage>;
};
%extend refinery::ConvertToRgbFilter {
  %template(filter) filter<refinery::RGBImage>;
};
%extend refinery::GammaFilter {
  %template(filter) filter<refinery::RGBImage, refinery::GammaCurve<unsigned short> >;
};

%warnfilter(314) refinery::GammaCurve::at;
%include "refinery/gamma.h"
namespace refinery {
  %extend GammaCurve {
    %template(GammaCurve) GammaCurve<refinery::Histogram<refinery::RGBImage, 3> >;
  };
}
%template(GammaCurveU16) refinery::GammaCurve<unsigned short>;

%include "refinery/histogram.h"
%ignore refinery::Histogram<refinery::RGBImage, 3>::NColors;
%template(RGBHistogram) refinery::Histogram<refinery::RGBImage, 3>;

%include "refinery/image_tile.h"

%include "refinery/interpolate.h"

%include "refinery/output.h"

%include "refinery/unpack.h"
