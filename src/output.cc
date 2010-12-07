#include "refinery/output.h"

#include <fstream>
#include <iostream>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

#include "refinery/image.h"

namespace refinery {

class PpmImageWriter {
  std::ostream& mOutput;

public:
  PpmImageWriter(std::ostream& output) : mOutput(output) {}

  void writeImage(const Image& image)
  {
    mOutput << "P6\n";
    mOutput << image.width() << " " << image.height() << "\n";
    mOutput << "65535\n"; // FIXME

    for (int row = 0; row < image.height(); row++) {
      Image::ConstRowType pixels(image.constPixelsRow(row));

      for (int col = 0; col < image.width(); col++, pixels++) {
        unsigned char s[6];

        s[0] = static_cast<unsigned char>(pixels[0][0] >> 8);
        s[1] = static_cast<unsigned char>(pixels[0][0] & 0xff);
        s[2] = static_cast<unsigned char>(pixels[0][1] >> 8);
        s[3] = static_cast<unsigned char>(pixels[0][1] & 0xff);
        s[4] = static_cast<unsigned char>(pixels[0][2] >> 8);
        s[5] = static_cast<unsigned char>(pixels[0][2] & 0xff);

        mOutput.write(reinterpret_cast<char*>(s), 6);
      }
    }
  }
};

#include <iostream>

void ImageWriter::writeImage(
    const Image& image, std::ostream& ostream, const char* type)
{
  if (!type) type = "ppm";

  std::string stype(type);
  boost::algorithm::to_upper(stype);

  if (stype == "PPM") {
    PpmImageWriter(ostream).writeImage(image);
  } else {
    throw "FIXME";
  }
}

void ImageWriter::writeImage(
    const Image& image, const char* filename, const char* type)
{
  std::ofstream out(filename);
  writeImage(image, out, type);
}

} // namespace refinery
