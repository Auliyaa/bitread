#include "image_utils.h"

#include <iostream>
#include <cstring>

template<typename fmt>
void process(const std::string& src_path, const std::string& tgt_path, size_t w, size_t h)
{
  const size_t src_frame_sz = v210::frame_size(w,h);
  const size_t tgt_frame_sz = fmt::frame_size(w,h);
  const auto src_rowbytes = v210::row_size(w);
  const auto tgt_planes = fmt::plane_offsets(w,h);

  std::cout << "(src) frame size=" << src_frame_sz << std::endl;
  std::cout << "(tgt) frame size=" << tgt_frame_sz << std::endl;
  std::cout << "(tgt) planes=[" << tgt_planes[0] << "," << tgt_planes[1] << "," << tgt_planes[2] << "]" << std::endl;

  uint8_t* src_frame_buf = static_cast<uint8_t*>(std::malloc(src_frame_sz));
  uint8_t* tgt_frame_buf = static_cast<uint8_t*>(std::malloc(tgt_frame_sz));

  FILE* src_fd = fopen(src_path.c_str(), "rb");
  FILE* tgt_fd = fopen(tgt_path.c_str(), "wb");

  // read source frames
  size_t loop_idx{0};
  size_t read=fread(src_frame_buf, 1, src_frame_sz, src_fd);

  while (read == src_frame_sz)
  {
    std::cout << loop_idx << " >> read " << read << "bytes" << std::endl;

    #pragma omp parallel for collapse(2)
    for(size_t xx=0; xx<w; xx+=6)
    {
      for(size_t yy=0; yy<h; ++yy)
      {
        auto yuv = v210::read_group(src_frame_buf, xx, yy, src_rowbytes);
        fmt::write2(tgt_frame_buf, yuv.y0, yuv.y1, yuv.u0_1, yuv.v0_1, xx, yy, w, tgt_planes);
        fmt::write2(tgt_frame_buf, yuv.y2, yuv.y3, yuv.u2_3, yuv.v2_3, xx, yy, w, tgt_planes);
        fmt::write2(tgt_frame_buf, yuv.y4, yuv.y5, yuv.u4_5, yuv.v4_5, xx, yy, w, tgt_planes);
      }
    }
    fwrite(tgt_frame_buf, 1, tgt_frame_sz, tgt_fd);
    read=fread(src_frame_buf, 1, src_frame_sz, src_fd);
    ++loop_idx;
  }

  fclose(src_fd);
  fclose(tgt_fd);
}

int main(int argc, char** argv)
{
  if (argc != 5)
  {
    std::cerr << "usage: " << argv[0] << " <source> <target> <width> <height>" << std::endl;
    return 1;
  }

  std::string src_path = argv[1];
  std::string tgt_path = argv[2];
  const size_t w=std::stoi(argv[3]);
  const size_t h=std::stoi(argv[4]);

  process<yuv422p10le>(src_path, tgt_path, w, h);
  //process<yuv444p>(src_path, tgt_path, w, h);

  return 0;
}
