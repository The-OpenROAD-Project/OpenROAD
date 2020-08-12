template< class T, class Friend > class read_only
{
public:
  read_only() { }
  read_only( T data ) : data_( data ) { }
  operator T() const   { return data_; }
  const T* operator&() const { return &_data; }
private:
  T data_;
  T* operator&()       { return &_data; }

  read_only< T, Friend >& operator=( T data )
  {
	data_ = data;
	return *this;
  }

  friend class Friend;
};
