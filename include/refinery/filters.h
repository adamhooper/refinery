#ifndef _REFINERY_FILTERS_H
#define _REFINERY_FILTERS_H

namespace refinery {

class Filter {
public:
  void filter(Image& image);

protected:
  virtual void doFilter(Image& image);
};

class SubtractBlackFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class PreInterpolateFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class MedianFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class BlendHighlightsFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class RecoverHighlightsFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class ConvertToRgbFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

class AhdInterpolateFilter : Filter {
protected:
  virtual void doFilter(Image& image);
};

};

#endif /* REFINERY_FILTERS_H */
