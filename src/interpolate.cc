#include "refinery/interpolate.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "refinery/camera.h"
#include "refinery/image.h"
#include "refinery/image_tile.h"

namespace refinery {

namespace {
  void interpolateBorder(Image& image, int border) {
    typedef Image::Color Color;

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
} // namespace {}

class BilinearInterpolator {
  struct PixelInstructions {
    unsigned int adjacentWeights[8];
    unsigned int adjacentColors[8];
    unsigned int otherColors[2];
    unsigned int divisions[2];
  };

  class PixelsInstructions {
    PixelInstructions pixels[16][16];

    public:
    PixelsInstructions(const Image& image)
    {
      for (unsigned int row = 0; row < 16; row++) {
        for (unsigned int col = 0; col < 16; col++) {
          PixelInstructions& instructions = pixels[row & 15][col & 15];

          unsigned int sums[3] = { 0, 0, 0 };

          unsigned int adjIndex = 0;
          for (int y = -1; y <= 1; y++) {
            for (int x = -1; x <= 1; x++) {
              unsigned int shift = (x == 0) + (y == 0);
              if (shift == 2) continue;

              instructions.adjacentWeights[adjIndex] = shift;
              unsigned int color = image.colorAtPoint(row + y, col + x);
              instructions.adjacentColors[adjIndex] = color;

              sums[color] += 1 << shift;

              adjIndex++;
            }
          }

          const unsigned int colorAtPixel = image.colorAtPoint(row, col);
          unsigned int colorIndex = 0;
          for (unsigned int color = 0; color < 3; color++) {
            if (color == colorAtPixel) continue;

            instructions.otherColors[colorIndex] = color;
            instructions.divisions[colorIndex] = 256 / sums[color];

            colorIndex++;
          }
        }
      }
    }

    const PixelInstructions& getPixelInstructions(
        unsigned int row, unsigned int col) const
    {
      return pixels[row & 15][col & 15];
    }
  };

public:
  void interpolate(Image& image) {
    interpolateBorder(image, 1);

    const unsigned int width = image.width();
    const unsigned int height = image.height();

    const unsigned int top = 1;
    const unsigned int bottom = height - 1;
    const unsigned int left = 1;
    const unsigned int right = width - 1;

    const int adjacentOffsets[8] = {
      -width - 1, -width, -width + 1, -1, 1, width -1 , width, width + 1
    };
    const PixelsInstructions pixelsInstructions(image);

    for (unsigned int row = top; row < bottom; row++) {
      Image::RowType pix(&image.pixelsRow(row)[left]);
      for (unsigned int col = left; col < right; col++, pix++) {
        const PixelInstructions& instructions(
            pixelsInstructions.getPixelInstructions(row, col));

        unsigned int sums[3] = { 0, 0, 0 };

        for (unsigned int adjIndex = 0; adjIndex < 8; adjIndex++) {
          const int adjOffset = adjacentOffsets[adjIndex];
          const unsigned int adjWeight = instructions.adjacentWeights[adjIndex];
          const unsigned int adjColor = instructions.adjacentColors[adjIndex];

          sums[adjColor] +=
              static_cast<unsigned int>(pix[adjOffset][adjColor]) << adjWeight;
        }

        for (unsigned int colorIndex = 0; colorIndex < 2; colorIndex++) {
          const unsigned int color = instructions.otherColors[colorIndex];
          const unsigned int division = instructions.divisions[colorIndex];

          pix[0][color] =
              static_cast<Image::ValueType>(sums[color] * division >> 8);
        }
      }
    }
  }
};

namespace { std::vector<float> xyzCbrtLookup; } // FIXME make singleton

class AHDInterpolator {
private:

  typedef Image::Color Color;
  typedef TypedImageTile<char, 3> HomogeneityTile; // 1: H; 2: V; 3: diff

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
    unsigned int i(static_cast<int>(f) & 0x1ffff);

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
    // Assume right-shift of signed values leaves 1s, not 0s
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
    const int colCValue =
        pix[0][G] +
        ((pixAbove[0][colC] + pixBelow[0][colC]
          - dPixAbove[0][G] - dPixBelow[0][G]) >> 1);
    dPix[0][colC] = clamp16(colCValue);

    const int rowCValue =
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
    const int colCValue =
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
      const float (&cameraToXyz)[3][4])
  {
    float cbrtX = xyz64Cbrt(0.5f
        + cameraToXyz[X][R] * rgb[R]
        + cameraToXyz[X][G] * rgb[G]
        + cameraToXyz[X][B] * rgb[B]);
    float cbrtY = xyz64Cbrt(0.5f
        + cameraToXyz[Y][R] * rgb[R]
        + cameraToXyz[Y][G] * rgb[G]
        + cameraToXyz[Y][B] * rgb[B]);
    float cbrtZ = xyz64Cbrt(0.5f
        + cameraToXyz[Z][R] * rgb[R]
        + cameraToXyz[Z][G] * rgb[G]
        + cameraToXyz[Z][B] * rgb[B]);

    lab[L] = static_cast<LABImage::ValueType>(116.0f * cbrtY - (64.0f*16.0f));
    lab[A] = static_cast<LABImage::ValueType>(500.0f * (cbrtX - cbrtY));
    lab[B] = static_cast<LABImage::ValueType>(200.0f * (cbrtY - cbrtZ));
  }

  void createCielabImage(
      const ImageTile& imageTile, LABImageTile& labImageTile,
      const float (&cameraToXyz)[3][4])
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
        rgbToLab(pix[0], lPix[0], cameraToXyz);
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
                >> (sizeof(lEps) * 8 - 1));
#endif
          }

          homoPix[0][dir] = homogeneity;
        }
      }
    }
  }

  void refillImage(
      Image& image, const ImageTile& hImageTile, const ImageTile& vImageTile,
      HomogeneityTile& homoTile)
  {
    const unsigned int top = hImageTile.top() + 3;
    const unsigned int left = hImageTile.left() + 3;
    const unsigned int right = hImageTile.right() - 3;
    const unsigned int bottom = hImageTile.bottom() - 3;

    const int tileWidth = hImageTile.width();

    for (unsigned int row = top; row < bottom; row++) {
      HomogeneityTile::PixelsType homoPix(
          &homoTile.pixelsAtImageCoords(row, left)[0]);
      HomogeneityTile::ConstPixelsType homoPixAbove(&homoPix[-tileWidth]);
      HomogeneityTile::ConstPixelsType homoPixBelow(&homoPix[tileWidth]);

      for (unsigned int col = left; col < right;
          col++, homoPix++, homoPixAbove++, homoPixBelow++) {

        unsigned int hm[2] = { 0, 0 };
        for (unsigned int dir = H; dir <= V; dir++) {
          for (int i = -1; i <= 1; i++) {
            hm[dir] += homoPixAbove[i][dir];
            hm[dir] += homoPix[i][dir];
            hm[dir] += homoPixBelow[i][dir];
          }
        }

        homoPix[0][2] = hm[H] - hm[V];
      }
    }

    for (unsigned int row = top; row < bottom; row++) {
      Image::RowType pix(&image.pixelsRow(row)[left]);

      HomogeneityTile::ConstPixelsType homoPix(
          &homoTile.constPixelsAtImageCoords(row, left)[0]);
      ImageTile::ConstPixelsType hPix(
          &hImageTile.constPixelsAtImageCoords(row, left)[0]);
      ImageTile::ConstPixelsType vPix(
          &vImageTile.constPixelsAtImageCoords(row, left)[0]);

      for (unsigned int col = left; col < right;
          col++, pix++, hPix++, vPix++, homoPix++) {
        if (homoPix[0][2] > 0) {
          for (Color c = R; c <= B; c++) {
            pix[0][c] = hPix[0][c];
          }
        } else if (homoPix[0][2] < 0) {
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

public:
  void interpolate(Image& image) {
    const unsigned int border = 5;

    const Camera::ColorConversionData colorData(
        image.cameraData().colorConversionData());
    float cameraToXyz[3][4];
    for (unsigned int i = 0; i < 3; i++) {
      for (unsigned int j = 0; j < image.cameraData().colors(); j++) {
        // convert from double to float, for speed
        cameraToXyz[i][j] = colorData.cameraToXyz[i][j];
      }
    }

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
    Point tileSize(tileHeight, tileWidth);

#pragma omp parallel
    {
      Point tileTopLeft(border - margin, border - margin);

      ImageTile hImageTile(imageSize, tileTopLeft, tileSize, border, margin);
      ImageTile vImageTile(imageSize, tileTopLeft, tileSize, border, margin);
      LABImageTile hLabImageTile(
          imageSize, tileTopLeft, tileSize, border, margin);
      LABImageTile vLabImageTile(
          imageSize, tileTopLeft, tileSize, border, margin);
      HomogeneityTile homoTile(imageSize, tileTopLeft, tileSize, border, margin);

#pragma omp for schedule(dynamic)
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

          createCielabImage(hImageTile, hLabImageTile, cameraToXyz);
          createCielabImage(vImageTile, vLabImageTile, cameraToXyz);

          fillHomogeneityMap(hLabImageTile, vLabImageTile, homoTile);

          refillImage(image, hImageTile, vImageTile, homoTile);
        }
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
      {
        AHDInterpolator ahdInterpolator;
        ahdInterpolator.interpolate(image);
        break;
      }
    case INTERPOLATE_BILINEAR:
      {
        BilinearInterpolator bilinearInterpolator;
        bilinearInterpolator.interpolate(image);
        break;
      }
  }
}

}
