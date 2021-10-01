#include <opencv2/opencv.hpp>

// https://stackoverflow.com/a/34734939/5008845
void reduce_quant(const cv::Mat3b& src, cv::Mat3b& dst)
{
  const uchar N = 64;
  dst = src / N;
  dst *= N;
}

// https://stackoverflow.com/a/34734939/5008845
void reduce_kmeans(const cv::Mat3b& src, cv::Mat3b& dst)
{
  const int K=8;
  int n = src.rows * src.cols;
  cv::Mat data = src.reshape(1, n);
  data.convertTo(data, CV_32F);

  std::vector<int> labels;
  cv::Mat1f colors;
  cv::kmeans(data, K, labels, cv::TermCriteria(), 1, cv::KMEANS_PP_CENTERS, colors);

  for (int i=0; i<n; ++i)
  {
    data.at<float>(i, 0) = colors(labels[i], 0);
    data.at<float>(i, 1) = colors(labels[i], 1);
    data.at<float>(i, 2) = colors(labels[i], 2);
  }

  cv::Mat reduced = data.reshape(3, src.rows);
  reduced.convertTo(dst, CV_8U);
}

void reduce_stylization(const cv::Mat3b& src, cv::Mat3b& dst)
{
  cv::stylization(src, dst);
}

void reduce_edge_preserve(const cv::Mat3b& src, cv::Mat3b& dst)
{
  cv::edgePreservingFilter(src, dst);
}

struct less_vec3b_t
{
  bool operator()(const cv::Vec3b& a, const cv::Vec3b& b) const
  {
    if (a[0] != b[0]) return a[0] < b[0];
    if (a[1] != b[1]) return a[1] < b[1];
    return a[2] < b[2];
  }
};



int main(int argc, char** argv)
{
  // load & reduce image
  cv::Mat3b img = cv::imread(argv[1]);
  cv::Mat3b rdc;
  reduce_quant(img,rdc);

  // get palette
  std::map<cv::Vec3b, int, less_vec3b_t> palette;
  for(size_t r=0; r<rdc.rows; ++r)
  {
    for(size_t c=0; c<rdc.cols; ++c)
    {
      auto bgr = rdc(r,c);
      if (palette.count(bgr)==0) palette[bgr]=1;
      palette[bgr]++;
    }
  }

  // print result
  for (const auto& kv : palette)
  {
    std::cout << kv.first << ": " << kv.second << std::endl;
  }

  return 0;
}
