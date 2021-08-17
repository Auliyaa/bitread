class sd_logger: public std::streambuf
{
  int _l;
  std::string _buf;

public:
  inline sd_logger(int lvl): std::streambuf(), _l(lvl) {}

protected:
  virtual std::streamsize xsputn(const char_type* __s, std::streamsize __n) override
  {
    // append
    std::string msg{__s, static_cast<size_t>(__n)};
    _buf+=msg;
    return __n;
  }

  virtual int_type overflow(int_type __c) override
  {
    // flush
    sd_journal_print(_l, _buf.c_str());
    _buf.clear();
    return __c;
  }

} __sd_logger_info(LOG_INFO), __sd_logger_warn(LOG_WARNING), __sd_logger_err(LOG_ERR);

std::ostream sd_logger_info{&__sd_logger_info};
std::ostream sd_logger_warn{&__sd_logger_warn};
std::ostream sd_logger_err{&__sd_logger_err};
