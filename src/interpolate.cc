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
    float xyz[3] = { 0.5, 0.5, 0.5 };
    for (Color rgbC = R; rgbC <= B; rgbC++) {
      xyz[X] += xyz_cam[X][rgbC] * rgb[rgbC];
      xyz[Y] += xyz_cam[Y][rgbC] * rgb[rgbC];
      xyz[Z] += xyz_cam[Z][rgbC] * rgb[rgbC];
    }

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

      for (int col = left; col < right; col++, lPix++, pix++, dPix++) {
        Image::ConstRowType pixAbove(&pix[-width]);
        Image::ConstRowType pixBelow(&pix[width]);
        Image::ConstRowType dPixAbove(&dPix[-width]);
        Image::ConstRowType dPixBelow(&dPix[width]);

        Color c(2 - image.colorAtPoint(Point(row, col)));

        if (c == G) {
          c = image.colorAtPoint(Point(row + 1, col));
          Image::ValueType cValue = clip(
              pix[0][G] +
              ((pixAbove[0][c] + pixBelow[0][c]
                - dPixAbove[0][G] - dPixBelow[0][G]) >> 1));
          dPix[0][c] = cValue;

          Color otherC(2 - c);
          Image::ValueType otherCValue = clip(
              pix[0][G] +
              ((pix[-1][otherC] + pix[1][otherC] - dPix[-1][G] - dPix[1][G])
                >> 1));
          dPix[0][otherC] = otherCValue;
        } else {
          Image::ValueType cValue = clip(
              dPix[0][G] +
              ((pixAbove[-1][c] + pixAbove[1][c]
                + pixBelow[-1][c] + pixBelow[1][c]
                - dPixAbove[-1][G] - dPixAbove[1][G]
                - dPixBelow[-1][G] - dPixBelow[1][G]
                + 1) >> 2));
          dPix[0][c] = cValue;
        }

        c = image.colorAtPoint(Point(row, col));
        dPix[0][c] = pix[0][c];

        rgbToLab(dPix[0], lPix[0], xyz_cam);
      }
    }
  }

  unsigned int epsilon(const unsigned int (&diff)[2][4])
  {
    return std::min(
        std::max(diff[H][0], diff[H][1]),
        std::max(diff[V][2], diff[V][3]));
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
      LABImage::ConstRowType hLabPix(&hLabImage.constPixelsRow(row)[left]);
      LABImage::ConstRowType vLabPix(&vLabImage.constPixelsRow(row)[left]);

      for (int col = left; col < right;
          col++, homoPix++, hLabPix++, vLabPix++) {

        for (unsigned int dir = H; dir <= V; dir++) {
          LABImage::ConstRowType dirLabPix(dir == H ? hLabPix : vLabPix);

          for (int adjIndex = 0; adjIndex < 4; adjIndex++) {
            LABImage::ConstRowType adjLabPix(&dirLabPix[adj[adjIndex]]);

            lDiff[dir][adjIndex] = std::abs(dirLabPix[0][L] - adjLabPix[0][L]);

            int aDiff = dirLabPix[0][A] - adjLabPix[0][A];
            int bDiff = dirLabPix[0][B] - adjLabPix[0][B];
            abDiff[dir][adjIndex] = aDiff * aDiff + bDiff * bDiff;
          }
        }

        unsigned int lEps = epsilon(lDiff);
        unsigned int abEps = epsilon(abDiff);

        for (unsigned int dir = H; dir <= V; dir++) {
          HomogeneityMap::ValueType homogeneity(0);
          for (int adjIndex = 0; adjIndex < 4; adjIndex++) {
            if (lDiff[dir][adjIndex] <= lEps && abDiff[dir][adjIndex] <= abEps)
            {
              homogeneity++;
            }
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
