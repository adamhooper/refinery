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

template<typename T, std::size_t N = 3> class TypedImageTile {
public:
  typedef TypedImage<T,N> ImageType;
  typedef typename ImageType::ValueType ValueType;
  typedef ValueType (*PixelsType)[N];
  typedef const ValueType (*ConstPixelsType)[N];

private:
  typedef std::vector<ValueType> StorageType;
  Point mImageSize;
  Point mTopLeft;
  Point mSize;
  unsigned int mEdgeSize;
  StorageType mPixels;

  void allocate()
  {
    mPixels.assign(mSize.row * mSize.col * N, 0);
  }

  ptrdiff_t offsetForImagePoint(const Point& imagePoint) const
  {
    Point tilePoint(imagePoint - mTopLeft);
    return (tilePoint.row * mSize.col + tilePoint.col) * N;
  }

public:
  TypedImageTile(
      const Point& imageSize, const Point& topLeft, const Point& size,
      unsigned int border, unsigned int margin)
    : mImageSize(imageSize), mTopLeft(topLeft), mSize(size),
      mEdgeSize(static_cast<unsigned int>(border - margin))
  {
    this->allocate();
  }

  unsigned int top() const {
    return std::max<unsigned int>(mTopLeft.row, mEdgeSize);
  }
  unsigned int left() const {
    return std::max<unsigned int>(mTopLeft.col, mEdgeSize);
  }
  unsigned int height() const {
    return mSize.row;
  }
  unsigned int width() const {
    return mSize.col;
  }
  unsigned int bottom() const {
    return std::min<unsigned int>(
        mImageSize.row - mEdgeSize,
        mTopLeft.row + mSize.row);
  }
  unsigned int right() const {
    return std::min<unsigned int>(
        mImageSize.col - mEdgeSize,
        mTopLeft.col + mSize.col);
  }

  void setTopLeft(const Point& topLeft) { mTopLeft = topLeft; }
  void setSize(int height, int width) {
    Point newSize(height, width);
    if (newSize != mSize) {
      mSize = newSize;
      this->allocate();
    }
  }

  PixelsType pixelsAtImageCoords(const Point& point) {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return reinterpret_cast<PixelsType>(&mPixels[offset]);
  }
  PixelsType pixelsAtImageCoords(int row, int col) {
    return pixelsAtImageCoords(Point(row, col));
  }
  ConstPixelsType constPixelsAtImageCoords(const Point& point) const {
    const ptrdiff_t offset(offsetForImagePoint(point));
    return reinterpret_cast<ConstPixelsType>(&mPixels[offset]);
  }
  ConstPixelsType constPixelsAtImageCoords(int row, int col) const {
    return constPixelsAtImageCoords(Point(row, col));
  }
};

typedef TypedImageTile<unsigned short, 3> RGBImageTile;
typedef RGBImageTile ImageTile;
typedef TypedImageTile<short, 3> LABImageTile;
typedef TypedImageTile<unsigned char, 2> HomogeneityTile;

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
      xyzCbrtLookup.reserve(0x20000);

      for (int i = 0; i < 0x10000; i++) {
        double r = i / 65535.0;
        xyzCbrtLookup[i] = 64.0f * (r > 0.008856 ? std::pow(r, 1.0/3) : 7.787*r + 16.0/116);
      }

      xyzCbrtMin = xyzCbrtLookup[0];
      xyzCbrtMax = xyzCbrtLookup[0xffff];

      for (int i = 0x10000; i <= 0x17fff; i++) {
        xyzCbrtLookup[i] = xyzCbrtMax;
      }
      for (int i = 0x18000; i <= 0x1ffff; i++) {
        xyzCbrtLookup[i] = xyzCbrtMin;
      }
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
   * Returns cbrt(f/65536), unless f is <= 580 in which case it's a linear
   * formula.
   *
   * Google how to convert RGB to XYZ to LAB. This formula pops up.
   *
   * The return value is multiplied by 64.0f, because we convert to an integer.
   *
   * If f is a small negative number or a bit over 65535, it will be clamped.
   * This will only work between around -32767 and 98302.
   */
  float xyz64Cbrt(float f) const
  {
    unsigned int i(static_cast<unsigned int>(static_cast<int>(f)) & 0x1ffff);

    return xyzCbrtLookup[i];
  }

  /**
   * Returns v bounded by bound1 and bound2.
   *
   * bound1 is the min and bound2 is the max, or vice-versa.
   */
  Image::ValueType bound(
      const Image::ValueType& v,
      const Image::ValueType& bound1, const Image::ValueType& bound2)
  {
    return std::min(
      std::max(bound1, std::min(v, bound2)),
      std::max(bound2, std::min(v, bound1))
      );
  }

  void createGreenDirectionalImages(
      const Image& image, ImageTile& hImageTile, ImageTile& vImageTile)
  {
    const unsigned int top = hImageTile.top();
    const unsigned int left = hImageTile.left();
    const unsigned int right = hImageTile.right();
    const unsigned int bottom = hImageTile.bottom();

    const int width = image.width();

    for (unsigned int row = top; row < bottom; row++) {
      unsigned int col =
          left + (image.colorAtPoint(Point(row, left)) & 1); // 1st R or B

      Image::ConstRowType pix(&image.constPixelsRow(row)[col]);

      Image::ConstRowType pixAbove(&pix[-width]);
      Image::ConstRowType pix2Above(&pix[-2 * width]);
      Image::ConstRowType pixBelow(&pix[width]);
      Image::ConstRowType pix2Below(&pix[2 * width]);

      ImageTile::PixelsType hPix(&hImageTile.pixelsAtImageCoords(row, col)[0]);
      ImageTile::PixelsType vPix(&vImageTile.pixelsAtImageCoords(row, col)[0]);

      Color c(image.colorAtPoint(Point(row, col)));

      for (; col < right;
          col += 2, pix += 2, hPix += 2, vPix += 2,
          pixAbove += 2, pix2Above += 2, pixBelow += 2, pix2Below += 2) {
        Image::ValueType hValue =
          ((pix[-1][G] + pix[0][c] + pix[1][G]) * 2
           - pix[-2][c] - pix[2][c]) >> 2;
        hPix[0][G] = bound(hValue, pix[-1][G], pix[1][G]);

        Image::ValueType vValue =
          ((pixAbove[0][G] + pix[0][c] + pixBelow[0][G]) * 2
            - pix2Above[0][c] - pix2Below[0][c]) >> 2;
        vPix[0][G] = bound(vValue, pixAbove[0][G], pixBelow[0][G]);
      }
    }
  }

  Image::ValueType clamp16(int val) {
    const unsigned int trashBits = val >> 16;
    return trashBits ? (~trashBits >> 16) : val;
  }

  inline void incrPointers(
      int n, Image::ConstRowType& pix,
      Image::ConstRowType& pixAbove, Image::ConstRowType& pixBelow,
      ImageTile::ConstPixelsType& dPixAbove,
      ImageTile::ConstPixelsType& dPixBelow)
  {
    pix += n;
    pixAbove += n;
    pixBelow += n;
    dPixAbove += n;
    dPixBelow += n;
  }

  inline void fillRandBinGPixel(
      ImageTile::PixelsType& dPix, const Color& rowC, const Color& colC,
      Image::ConstRowType& pix,
      Image::ConstRowType& pixAbove, Image::ConstRowType& pixBelow,
      ImageTile::ConstPixelsType& dPixAbove,
      ImageTile::ConstPixelsType& dPixBelow)
  {
    const Image::ValueType colCValue =
        pix[0][G] +
        ((pixAbove[0][colC] + pixBelow[0][colC]
          - dPixAbove[0][G] - dPixBelow[0][G]) >> 1);
    dPix[0][colC] = clamp16(colCValue);

    const Image::ValueType rowCValue =
        pix[0][G] +
        ((pix[-1][rowC] + pix[1][rowC] - dPix[-1][G] - dPix[1][G])
          >> 1);
    dPix[0][rowC] = clamp16(rowCValue);
  }

  inline void fillRandBinBorRPixel(
      ImageTile::PixelsType& dPix, const Color& rowC, const Color& colC,
      Image::ConstRowType& pix,
      Image::ConstRowType& pixAbove, Image::ConstRowType& pixBelow,
      ImageTile::ConstPixelsType& dPixAbove,
      ImageTile::ConstPixelsType& dPixBelow)
  {
    const Image::ValueType colCValue =
        dPix[0][G] +
        ((pixAbove[-1][colC] + pixAbove[1][colC]
          + pixBelow[-1][colC] + pixBelow[1][colC]
          - dPixAbove[-1][G] - dPixAbove[1][G]
          - dPixBelow[-1][G] - dPixBelow[1][G]
          + 1) >> 2);
    dPix[0][colC] = clamp16(colCValue);
  }

  void fillDirectionalImage(const Image& image, ImageTile& dirImageTile)
  {
    const unsigned int top = dirImageTile.top() + 1;
    const unsigned int left = dirImageTile.left() + 1;
    const unsigned int right = dirImageTile.right() - 1;
    const unsigned int bottom = dirImageTile.bottom() - 1;

    const int width = image.width();
    const int dWidth = dirImageTile.width();

    for (unsigned int row = top; row < bottom; row++) {
      /*
       * We assume the bayer pattern in the filter looks like one of these
       * (copied from dcraw comments):
       *
       *    0 1 2 3 4 5    0 1 2 3 4 5    0 1 2 3 4 5    0 1 2 3 4 5
       *    0 B G B G B G  0 G R G R G R  0 G B G B G B  0 R G R G R G
       *    1 G R G R G R  1 B G B G B G  1 R G R G R G  1 G B G B G B
       *    2 B G B G B G  2 G R G R G R  2 G B G B G B  2 R G R G R G
       *    3 G R G R G R  3 B G B G B G  3 R G R G R G  3 G B G B G B
       *
       * So we know:
       * - every 2nd row starts with the same color; the other is green
       * - horizontally, we only see two colors on a row (one is green)
       * - vertically, we only see two colors on a column (one is green)
       * - if we're looking at a green pixel and we know the row's other
       *   color c, the column's color is 2-c
       */

      Color rowC;
      Color colC;

      Color c(image.colorAtPoint(Point(row, left)));
      if (c == G) {
        rowC = image.colorAtPoint(Point(row, left + 1));
        colC = 2 - rowC;
      } else {
        rowC = c;
        colC = 2 - c;
      }

      ImageTile::PixelsType dPix(
          &dirImageTile.pixelsAtImageCoords(row, left + (c != G))[0]);
      ImageTile::ConstPixelsType dPixAbove(&dPix[-dWidth]);
      ImageTile::ConstPixelsType dPixBelow(&dPix[dWidth]);

      Image::ConstRowType pix(&image.constPixelsRow(row)[left + (c != G)]);
      Image::ConstRowType pixAbove(&pix[-width]);
      Image::ConstRowType pixBelow(&pix[width]);

      for (unsigned int col = left + (c != G); col < right; col += 2) {
        dPix[0][G] = pix[0][G];

        fillRandBinGPixel(
            dPix, rowC, colC, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
        dPix += 2;
        incrPointers(2, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
      }

      dPix = &dirImageTile.pixelsAtImageCoords(row, left + (c == G))[0];
      dPixAbove = &dPix[-dWidth];
      dPixBelow = &dPix[dWidth];

      pix = &image.constPixelsRow(row)[left + (c == G)];
      pixAbove = &pix[-width];
      pixBelow = &pix[width];

      for (unsigned int col = left + (c == G); col < right; col += 2) {
        dPix[0][rowC] = pix[0][rowC];

        fillRandBinBorRPixel(
            dPix, rowC, colC, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);

        dPix += 2;
        incrPointers(2, pix, pixAbove, pixBelow, dPixAbove, dPixBelow);
      }
    }
  }

  void rgbToLab(
      const Image::ValueType (&rgb)[3], LABImage::ValueType (&lab)[3],
      const float (&xyzCam)[3][3])
  {
    float x = xyz64Cbrt(0.5f
        + xyzCam[X][R] * rgb[R]
        + xyzCam[X][G] * rgb[G]
        + xyzCam[X][B] * rgb[B]);
    float y = xyz64Cbrt(0.5f
        + xyzCam[Y][R] * rgb[R]
        + xyzCam[Y][G] * rgb[G]
        + xyzCam[Y][B] * rgb[B]);
    float z = xyz64Cbrt(0.5f
        + xyzCam[Z][R] * rgb[R]
        + xyzCam[Z][G] * rgb[G]
        + xyzCam[Z][B] * rgb[B]);

    lab[L] = static_cast<LABImage::ValueType>(116.0f * y - (64.0f*16.0f));
    lab[A] = static_cast<LABImage::ValueType>(500.0f * (x - y));
    lab[B] = static_cast<LABImage::ValueType>(200.0f * (y - z));
  }

  void createCielabImage(
      const ImageTile& imageTile, LABImageTile& labImageTile,
      const float (&xyzCam)[3][3])
  {
    const unsigned int top = imageTile.top() + 1;
    const unsigned int left = imageTile.left() + 1;
    const unsigned int right = imageTile.right() - 1;
    const unsigned int bottom = imageTile.bottom() - 1;

    for (unsigned int row = top; row < bottom; row++) {
      ImageTile::ConstPixelsType pix(
          &imageTile.constPixelsAtImageCoords(row, left)[0]);

      LABImageTile::PixelsType lPix(
          &labImageTile.pixelsAtImageCoords(row, left)[0]);

      for (unsigned int col = left; col < right; col++) {
        rgbToLab(pix[0], lPix[0], xyzCam);
        pix++;
        lPix++;
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
      const LABImageTile& hLabImageTile, const LABImageTile& vLabImageTile,
      HomogeneityTile& homoTile)
  {
    const unsigned int top = hLabImageTile.top() + 2;
    const unsigned int left = hLabImageTile.left() + 2;
    const unsigned int right = hLabImageTile.right() - 2;
    const unsigned int bottom = hLabImageTile.bottom() - 2;

    const int width = hLabImageTile.width();

    const int ADJ[4] = { -1, 1, -width, width };

    for (unsigned int row = top; row < bottom; row++) {
      HomogeneityTile::PixelsType homoPix(
          &homoTile.pixelsAtImageCoords(row, left)[0]);

      LABImageTile::ConstPixelsType labPix[2] = {
        &hLabImageTile.constPixelsAtImageCoords(row, left)[0],
        &vLabImageTile.constPixelsAtImageCoords(row, left)[0]
      };

      LABImageTile::ConstPixelsType labAdjPix[2][4];
      for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
        labAdjPix[H][adjDir] = &labPix[H][ADJ[adjDir]];
        labAdjPix[V][adjDir] = &labPix[V][ADJ[adjDir]];
      }

      for (unsigned int col = left; col < right; col++, homoPix++) {
        unsigned int lDiff[2][4], abDiff[2][4];

        for (unsigned int dir = H; dir <= V; dir++) {
          LABImageTile::ConstPixelsType dirLabPix(labPix[dir]);

          for (unsigned int adjDir = 0; adjDir < 4; adjDir++) {
            LABImageTile::ConstPixelsType adjLabPix(labAdjPix[dir][adjDir]);

            const int adjDiffL = dirLabPix[0][L] - adjLabPix[0][L];
            lDiff[dir][adjDir] = std::abs(adjDiffL);

            const int adjDiffA = dirLabPix[0][A] - adjLabPix[0][A];
            const int adjDiffB = dirLabPix[0][B] - adjLabPix[0][B];
            abDiff[dir][adjDir] = adjDiffA * adjDiffA + adjDiffB * adjDiffB;

            labAdjPix[dir][adjDir]++;
          }

          labPix[dir]++;
        }

        const unsigned int lEps = epsilon(lDiff);
        const unsigned int abEps = epsilon(abDiff);

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
                >> (sizeof(homogeneity) * 8 - 1));
#endif
          }

          homoPix[0][dir] = homogeneity;
        }
      }
    }
  }

  void refillImage(
      Image& image, const ImageTile& hImageTile, const ImageTile& vImageTile,
      const HomogeneityTile& homoTile)
  {
    const unsigned int top = hImageTile.top() + 3;
    const unsigned int left = hImageTile.left() + 3;
    const unsigned int right = hImageTile.right() - 3;
    const unsigned int bottom = hImageTile.bottom() - 3;

    const int tileWidth = hImageTile.width();

    for (unsigned int row = top; row < bottom; row++) {
      Image::RowType pix(&image.pixelsRow(row)[left]);

      ImageTile::ConstPixelsType hPix(
          &hImageTile.constPixelsAtImageCoords(row, left)[0]);
      ImageTile::ConstPixelsType vPix(
          &vImageTile.constPixelsAtImageCoords(row, left)[0]);

      HomogeneityTile::ConstPixelsType homoPix(
          &homoTile.constPixelsAtImageCoords(row, left)[0]);
      HomogeneityTile::ConstPixelsType homoPixAbove(&homoPix[-tileWidth]);
      HomogeneityTile::ConstPixelsType homoPixBelow(&homoPix[tileWidth]);

      for (unsigned int col = left; col < right;
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
    const unsigned int border = 5;

    float xyzCam[3][3];
    fillXyzCam(image, xyzCam);

    interpolateBorder(image, border);

    const unsigned int height = image.height();
    const unsigned int width = image.width();

    const unsigned int tileHeight = 256;
    const unsigned int tileWidth = 256;

    const unsigned int margin = 3;
    const unsigned int left = border - margin;
    const unsigned int top = border - margin;
    const unsigned int bottom = height - border;
    const unsigned int right = width - border;

    Point imageSize(height, width);
    Point tileTopLeft(border - margin, border - margin);
    Point tileSize(tileHeight, tileWidth);

    ImageTile hImageTile(imageSize, tileTopLeft, tileSize, border, margin);
    ImageTile vImageTile(imageSize, tileTopLeft, tileSize, border, margin);
    LABImageTile hLabImageTile(
        imageSize, tileTopLeft, tileSize, border, margin);
    LABImageTile vLabImageTile(
        imageSize, tileTopLeft, tileSize, border, margin);
    HomogeneityTile homoTile(imageSize, tileTopLeft, tileSize, border, margin);

    for (unsigned int row = top; row < bottom; row += tileHeight - 2*margin) {
      tileTopLeft.row = row;

      for (unsigned int col = left; col < right; col += tileWidth - 2*margin) {
        tileTopLeft.col = col;

        hImageTile.setTopLeft(tileTopLeft);
        vImageTile.setTopLeft(tileTopLeft);
        hLabImageTile.setTopLeft(tileTopLeft);
        vLabImageTile.setTopLeft(tileTopLeft);
        homoTile.setTopLeft(tileTopLeft);

        createGreenDirectionalImages(image, hImageTile, vImageTile);

        fillDirectionalImage(image, hImageTile);
        fillDirectionalImage(image, vImageTile);

        createCielabImage(hImageTile, hLabImageTile, xyzCam);
        createCielabImage(vImageTile, vLabImageTile, xyzCam);

        fillHomogeneityMap(hLabImageTile, vLabImageTile, homoTile);

        refillImage(image, hImageTile, vImageTile, homoTile);
      }
    }
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
