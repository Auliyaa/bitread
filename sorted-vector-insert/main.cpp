template <typename C>
class sorted_insert_iterator : public std::iterator<std::output_iterator_tag,void,void,void,void>
{
protected:
  C* _cont;

public:
  explicit sorted_insert_iterator (C& c)
    : _cont(&c) {}
  sorted_insert_iterator<C>& operator= (const typename C::value_type& value)
  {
    _cont->insert(std::upper_bound(_cont->begin(), _cont->end(), value), value);
    return *this;
  }
  sorted_insert_iterator<C>& operator*() { return *this; }
  sorted_insert_iterator<C>& operator++() { return *this; }
  sorted_insert_iterator<C> operator++(int) { return *this; }
};

template<typename T>
double median(const std::list<T>& data_cv)
{
  if (data_cv.size()==0) return 0;
  if (data_cv.size()==1) return data_cv.front();

  std::vector<T> data;
  std::copy(data_cv.begin(),data_cv.end(),sorted_insert_iterator<std::vector<T>>(data));

  if (data.size()%2 == 0)
  {
    return static_cast<double>(data[data.size()/2] + data[(data.size()/2)+1])/2.;
  }

  return data[data.size()/2];
}
