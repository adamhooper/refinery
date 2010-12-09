#include "refinery/interpolate.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "refinery/image.h"

namespace refinery {

static const float XyzRgb[3][3] = {
  { 0.412453, 0.357580, 0.180423 },
  { 0.212671, 0.715160, 0.072169 },
  { 0.019334, 0.119193, 0.950227 }
};

static const float RgbCam[3][4] = { // FIXME: not const!
  { 1.826925, -0.654972, -0.171953, 0 },
  { -0.006813, 1.332168, -0.325355, 0 },
  { 0.062587, -0.400578, 1.337991, 0 }
};

static const float D65White[3] = { 0.950456, 1, 1.088754 };

static std::vector<float> xyzCbrtLookup; // FIXME make singleton

class AHDInterpolator {
private:
  typedef TypedImage<unsigned char, 2> HomogeneityMap;

  typedef Image::Color Color;

  static const Color R = Image::R;
  static const Color G = Image::G;
  static const Color B = Image::B;

  static const Color X = 0;
  static const Color Y = 1;
  static const Color Z = 2;

  static const Color L = 0;
  static const Color A = 1;
  /*static const Color B = 2;*/ /* (lucky they're the same!) */

  static const Color H = 0;
  static const Color V = 1;

  float xyzCbrtMin;
  float xyzCbrtMax;

public:
  AHDInterpolator()
  {
    if (xyzCbrtLookup.size() == 0) {
      // This is thread-safe: worst-case we just set the same value multiple
      // times.
      xyzCbrtLookup.reserve(0x10000);

      for (int i = 0; i < 0x10000; i++) {
        double r = i / 65535.0;
        xyzCbrtLookup[i] = (r > 0.008856 ? std::pow(r, 1.0/3) : 7.787*r + 16.0/116);
      }

      xyzCbrtMin = xyzCbrtLookup[0];
      xyzCbrtMax = xyzCbrtLookup[0xffff];
    }
  }

private:
  void interpolateBorder(Image& image, int border) {
    const int width = image.width(), height = image.height();
    const int top = 0, left = 0, right = width, bottom = height;

    for (int row = top; row < bottom; row++) {
      for (int col = left; col < right; col++) {
        if (col == border && row >= border && row < bottom - border) {
          col = right - border;
        }

        unsigned int sum[4] = { 0, 0, 0, 0 };
        unsigned int count[4] = { 0, 0, 0, 0 };

        for (int y = row - 1; y <= row + 1; y++) {
          if (y < top || y >= bottom) continue;

          for (int x = col - 1; x <= col + 1; x++) {
            if (x < left || x >= right) continue;

            Point p(y, x);
            Color c = image.colorAtPoint(p);
            sum[c] += image.pixel(p)[c];
            count[c]++;
          }
        }

        Point curP(row, col);
        Color curC = image.colorAtPoint(curP);
        for (Color c = 0; c < 4; c++) {
          if (c != curC && count[c]) {
            image.pixel(curP)[c] = sum[c] / count[c];
          }
        }
      }
    }
  }

  /**
   * Returns cbrt(f), unless f is <= 580 in which case it's a linear thing.
   *
   * Google how to convert RGB to XYZ to LAB. This formula pops up.
   */
  float xyzCbrt(float f) const
  {
    if (f <= 0.0f) return xyzCbrtMin;
    unsigned int u = static_cast<unsigned int>(f);
    if (u >= 0xffff) return xyzCbrtMax;

    return xyzCbrtLookup[u];
  }

  /**
   * Returns v bounded by bound1 and bound2.
   *
   * bound1 is the min and bound2 is the max, or vice-versa.
   */
  Image::ValueType bound(
      const Image::ValueType& v, const Image::ValueType& bound1,
      const Image::ValueType& bound2)
  {
    if (bound1 <= bound2) {
      if (v <= bound1) {
        return bound1;
      } else if (v >= bound2) {
        return bound2;
      } else {
        return v;
      }
    } else {
      if (v <= bound2) {
        return bound2;
      } else if (v <= bound1) {
        return v;
      } else {
        return bound1;
      }
    }
  }

  void createGreenDirectionalImages(
      const Image& image, Image& hImage, Image& vImage)
  {
    const int width = image.width(), height = image.height();
    const int top = 2, left = 2, right = width - 2, bottom = height - 2;

    hImage.pixels().assign(image.constPixels().size(), 0);
    vImage.pixels().assign(image.constPixels().size(), 0);

    for (int row = top; row < bottom; row++) {
      int col = left + (image.colorAtPoint(Point(row, left)) & 1); // 1st R or B

      Image::ConstRowType pix(&image.constPixelsRow(row)[col]);
      Image::RowType hPix(&hImage.pixelsRow(row)[col]);
      Image::RowType vPix(&vImage.pixelsRow(row)[col]);

      Color c(image.colorAtPoint(Point(row, col)));

      for (; col < right; col += 2, pix += 2, hPix += 2, vPix += 2) {
        Image::ValueType hValue =
          ((pix[-1][G] + pix[0][c] + pix[1][G]) * 2
           - pix[-2][c] - pix[2][c]) >> 2;
        hPix[0][G] = bound(hValue, pix[-1][G], pix[1][G]);

        Image::ValueType vValue =
          ((pix[-width][G] + pix[0][c] + pix[width][G]) * 2
            - pix[-2*width][c] - pix[2*width][c]) >> 2;
        vPix[0][G] = bound(vValue, pix[-width][G], pix[width][G]);
      }
    }
  }

  Image::ValueType clip(int val) {
    if (val < 0) return 0;
    if (val > std::numeric_limits<Image::ValueType>::max()) {
      return std::numeric_limits<Image::ValueType>::max();
    }
    return static_cast<Image::ValueType>(val);
  }

  void rgbToLab(
      const Image::ValueType (&rgb)[3], LABImage::ValueType (&lab)[3],
      const float (&xyz_cam)[3][3])
  {
    float xyz[3] = {
      xyz_cam[X][R] * rgb[R] + xyz_cam[X][G] * rgb[G] + xyz_cam[X][B] * rgb[B]
        + 0.5,
      xyz_cam[Y][R] * rgb[R] + xyz_cam[Y][G] * rgb[G] + xyz_cam[Y][B] * rgb[B]
        + 0.5,
      xyz_cam[Z][R] * rgb[R] + xyz_cam[Z][G] * rgb[G] + xyz_cam[Z][B] * rgb[B]
        + 0.5
    };

    for (Color xyzC = X; xyzC <= Z; xyzC++) {
      xyz[xyzC] = xyzCbrt(xyz[xyzC]);
    }

    lab[L] = static_cast<LABImage::ValueType>(
        64.0f * (116.0f * xyz[Y] - 16.0f));
    lab[A] = static_cast<LABImage::ValueType>(
        64.0f * 500.0f * (xyz[X] - xyz[Y]));
    lab[B] = static_cast<LABImage::ValueType>(
        64.0f * 200.0f * (xyz[Y] - xyz[Z]));
  }

  void fillRandBinGPixel(
      Image::RowType& dPix, const Color& rowC, const Color& colC,
      Image::ConstRowType& pix,
      Image::ConstRowType& pixAbove, Image::ConstRowType& pixBelow,
      Image::ConstRowType& dPixAbove, Image::ConstRowType& dPixBelow)
  {
    dPix[0][G] = pix[0][G];

    Image::ValueType colCValue = clip(
        pix[0][G] +
        ((pixAbove[0][colC] + pixBelow[0][colC]
          - dPixAbove[0][G] - dPixBelow[0][G]) >> 1));
    dPix[0][colC] = colCValue;

    Image::ValueType rowCValue = clip(
        pix[0][G] +
        ((pix[-1][rowC] + pix[1][rowC] - dPix[-1][G] - dPix[1][G])
          >> 1));
    dPix[0][rowC] = rowCValue;
  }

  void fillRandBinBorRPixel(
      Image::RowType& dPix, const Color& pixelC, const Color& otherC,
      Image::ConstRowType& pix,
      Image::ConstRowType& pixAbove, Image::ConstRowType& pixBelow,
      Image::ConstRowType& dPixAbove, Image::ConstRowType& dPixBelow)
  {
    dPix[0][pixelC] = pix[0][pixelC];

    Image::ValueType otherCValue = clip(
        dPix[0][G] +
        ((pixAbove[-1][otherC] + pixAbove[1][otherC]
          + pixBelow[-1][otherC] + pixBelow[1][otherC]
          - dPixAbove[-1][G] - dPixAbove[1][G]
          - dPixBelow[-1][G] - dPixBelow[1][G]
          + 1) >> 2));
    dPix[0][otherC] = otherCValue;
  }

  void fillRandBinDirectionalImageAndCreateCielab(
      const Image& image, Image& dirImage, LABImage& labImage,
      const float (&xyz_cam)[3][3])
  {
    labImage.pixels().assign(image.constPixels().size(), 0);

    const int width = image.width(), height = image.height();
    const int top = 3, left = 3, right = width - 3, bottom = height - 3;

    for (int row = top; row < bottom; row++) {
      Image::ConstRowType pix(&image.constPixelsRow(row)[left]);
      Image::RowType dPix(&dirImage.pixelsRow(row)[left]);
      LABImage::RowType lPix(&labImage.pixelsRow(row)[left]);

      /*
       * We assume the bayer pattern in the filter looks like one of these
       * (copied from dcraw comments):
       *
       *    0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5   0 1 2 3 4 5
       *    0 B G B G B G 0 G R G R G R 0 G B G B G B 0 R G R G R G
       *    1 G R G R G R 1 B G B G B G 1 R G R G R G 1 G B G B G B
       *    2 B G B G B G 2 G R G R G R 2 G B G B G B 2 R G R G R G
       *    3 G R G R G R 3 B G B G B G 3 R G R G R G 3 G B G B G B
       *
       * So we know:
       * - every 2nd row starts with the same color; the other is green
       * - horizontally, we only see two colors on a row (one is green)
       * - vertically, we only see two colors on a column (one is green)
       * - if we're looking at a green pixel and we know the row's other
       *   color c, the column's color is 2-c
       */


      for (int col = left; col < right; col++, lPix++, pix++, dPix++) {
        Image::ConstRowType pixAbove(&pix[-width]);
        Image::ConstRowType pixBelow(&pix[width]);
        Image::ConstRowType dPixAbove(&dPix[-width]);
        Image::ConstRowType dPixBelow(&dPix[width]);

        Color c(image.colorAtPoint(Point(row, col)));

        if (c == G) {
          Color colC(image.colorAtPoint(Point(row + 1, col)));
          fillRandBinGPixel(
              dPix, 2 - colC, colC,
              pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
        } else {
          Color otherC(2 - c);
          fillRandBinBorRPixel(
              dPix, c, otherC, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
        }

        rgbToLab(dPix[0], lPix[0], xyz_cam);
      }
    }
  }

  unsigned int epsilon(const unsigned int (&diff)[2][4])
  {
    return std::min(
        std::max(diff[H][0], diff[H][1]),
        std::max(diff[V][2], diff[V][3])) + 1;
  }

  void fillHomogeneityMap(
      const LABImage& hLabImage, const LABImage& vLabImage,
      HomogeneityMap& homoMap)
  {
    const int width = hLabImage.width(), height = hLabImage.height();
    const int top = 4, left = 4, right = width - 4, bottom = height - 4;
    unsigned int lDiff[2][4], abDiff[2][4];

    const int adj[4] = { -1, 1, -width, width };

    homoMap.pixels().assign(hLabImage.constPixels().size() * 2 / 3, 0);

    for (int row = top; row < bottom; row++) {
      HomogeneityMap::RowType homoPix(&homoMap.pixelsRow(row)[left]);

      LABImage::ConstRowType labPix[2];
      labPix[H] = &hLabImage.constPixelsRow(row)[left];
      labPix[V] = &vLabImage.constPixelsRow(row)[left];

      LABImage::ConstRowType labAdjPix[2][4];
      for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
        labAdjPix[H][adjDir] = &labPix[H][adj[adjDir]];
        labAdjPix[V][adjDir] = &labPix[V][adj[adjDir]];
      }

      for (int col = left; col < right; col++, homoPix++) {

        for (unsigned int dir = H; dir <= V; dir++) {
          LABImage::ConstRowType dirLabPix(labPix[dir]);
          LABImage::ConstRowType (&dirLabAdjPix)[4](labAdjPix[dir]);

          for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
            LABImage::ConstRowType adjLabPix(dirLabAdjPix[adjDir]);

            int labDiff[3];
            labDiff[L] = dirLabPix[0][L] - adjLabPix[0][L];
            labDiff[A] = dirLabPix[0][A] - adjLabPix[0][A];
            labDiff[B] = dirLabPix[0][B] - adjLabPix[0][B];

            lDiff[dir][adjDir] = std::abs(labDiff[L]);
            abDiff[dir][adjDir] =
                labDiff[A] * labDiff[A] + labDiff[B] * labDiff[B];

            adjLabPix++;
          }

          dirLabPix++;
        }

        unsigned int lEps = epsilon(lDiff);
        unsigned int abEps = epsilon(abDiff);

        for (unsigned int dir = H; dir <= V; dir++) {
          unsigned int homogeneity = 0;
          for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
#if 0
            /* This is the slow way--it uses branches */
            if (lDiff[dir][adjDir] < lEps && abDiff[dir][adjDir] < abEps)
            {
              homogeneity++;
            }
#else
            /*
             * Here's a branch-free equivalent. It assumes negative numbers
             * start with a 1 bit (which the C standard mandates)
             */
            homogeneity += (
                ((lDiff[dir][adjDir] - lEps) & (abDiff[dir][adjDir] - abEps))
                >> (sizeof(unsigned int) * 8 - 1));
#endif
          }

          homoPix[0][dir] = homogeneity;
        }
      }
    }
  }

  void refillImage(
      Image& image, const Image& hImage, const Image& vImage,
      const HomogeneityMap& homoMap)
  {
    const int width = image.width(), height = image.height();
    const int top = 5, left = 5, right = width - 5, bottom = height - 5;

    for (int row = top; row < bottom; row++) {
      Image::RowType pix(&image.pixelsRow(row)[left]);
      Image::ConstRowType hPix(&hImage.constPixelsRow(row)[left]);
      Image::ConstRowType vPix(&vImage.constPixelsRow(row)[left]);
      HomogeneityMap::ConstRowType
          homoPixAbove(&homoMap.constPixelsRow(row-1)[left]);
      HomogeneityMap::ConstRowType
          homoPix(&homoMap.constPixelsRow(row)[left]);
      HomogeneityMap::ConstRowType
          homoPixBelow(&homoMap.constPixelsRow(row+1)[left]);

      for (int col = left; col < right;
          col++, pix++, hPix++, vPix++,
          homoPix++, homoPixAbove++, homoPixBelow++) {

        unsigned int hm[2] = { 0, 0 };
        for (unsigned int dir = H; dir <= V; dir++) {
          for (int i = -1; i <= 1; i++) {
            hm[dir] += homoPixAbove[i][dir];
            hm[dir] += homoPix[i][dir];
            hm[dir] += homoPixBelow[i][dir];
          }
        }

        if (hm[H] > hm[V]) {
          for (Color c = R; c <= B; c++) {
            pix[0][c] = hPix[0][c];
          }
        } else if (hm[V] > hm[H]) {
          for (Color c = R; c <= B; c++) {
            pix[0][c] = vPix[0][c];
          }
        } else {
          for (Color c = R; c <= B; c++) {
            pix[0][c] = (hPix[0][c] + vPix[0][c]) >> 1;
          }
        }
      }
    }
  }

  void fillXyzCam(const Image& image, float (&xyzCam)[3][3])
  {
    // FIXME pass RgbCam through Image
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        for (int k = xyzCam[i][j] = 0; k < 3; k++) {
          xyzCam[i][j] += XyzRgb[i][k] * RgbCam[k][j] / D65White[i];
        }
      }
    }
  }

public:
  void interpolate(Image& image) {
    interpolateBorder(image, 5);

    Image hImage;
    hImage.importAttributes(image);

    Image vImage;
    vImage.importAttributes(image);

    createGreenDirectionalImages(image, hImage, vImage);

    LABImage hLabImage;
    hLabImage.importAttributes(image);

    LABImage vLabImage;
    vLabImage.importAttributes(image);

    float xyzCam[3][3];
    fillXyzCam(image, xyzCam);

    fillRandBinDirectionalImageAndCreateCielab(
        image, hImage, hLabImage, xyzCam);
    fillRandBinDirectionalImageAndCreateCielab(
        image, vImage, vLabImage, xyzCam);

    HomogeneityMap homoMap;
    homoMap.importAttributes(image);

    fillHomogeneityMap(hLabImage, vLabImage, homoMap);

    refillImage(image, hImage, vImage, homoMap);
  }
};

Interpolator::Interpolator(const Interpolator::Type& type)
  : mType(type)
{
}

void Interpolator::interpolate(Image& image)
{
  switch (mType) {
    case INTERPOLATE_AHD:
      AHDInterpolator ahdInterpolator;
      ahdInterpolator.interpolate(image);
  }
}

}
