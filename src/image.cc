#include "refinery/image.h"

namespace refinery {

Image::Image(int width, int height)
  : mWidth(width), mHeight(height), mBpp(0)
{
}

Image::~Image()
{
}

}
