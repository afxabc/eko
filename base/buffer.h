#ifndef _BASE_BUFFER_H__
#define _BASE_BUFFER_H__ 

#include <string.h>
#include <vector>

/// A buffer class modeled after muduo Buffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer
{
	static const size_t kMiniSize = 1024;

public:
	Buffer(size_t initSize = kMiniSize);
	Buffer(const char* data, size_t size);
	Buffer(Buffer& rhs);

	void swap(Buffer& rhs)
	{
		buffer_.swap(rhs.buffer_);
		std::swap(readerIndex_, rhs.readerIndex_);
		std::swap(writerIndex_, rhs.writerIndex_);
	}

	Buffer& operator=(Buffer& rhs)
	{
		swap(rhs);
		return *this;
	}

	size_t readableBytes() const
	{ return writerIndex_ - readerIndex_; }

	size_t writableBytes() const
	{ return buffer_.size() - writerIndex_; }

	size_t prependableBytes() const
	{ return readerIndex_; }

	const char* peek() const
	{ return begin() + readerIndex_; }
 
	char* beginWrite()
	{ return begin() + writerIndex_; }

	const char* beginWrite() const
	{ return begin() + writerIndex_; }

	char* beginRead()
	{ return begin() + readerIndex_; }

	const char* beginRead() const
	{ return peek(); }

	void erase()
	{ readerIndex_ = writerIndex_ = 0; }

	void eraseBack(size_t len)
	{
		if (len >= readableBytes())
			erase();
		else writerIndex_ -= len;
	}

	void eraseFront(size_t len)
	{
		if (len >= readableBytes())
			erase();
		else readerIndex_ += len;
	}

	size_t pushBack(const char* data, size_t len, bool resize = true);
	size_t pushBack(const char* str, bool resize = true)
	{ return pushBack(str, ::strlen(str), resize); }

	size_t pushFront(const char* data, size_t len, bool resize = true);
	size_t pushFront(const char* str, bool resize = true)
	{ return pushFront(str, ::strlen(str), resize); }

	size_t takeBack(char* buff, size_t size);
	size_t takeFront(char* buff, size_t size);

private:
	char* begin()
	{ return &*buffer_.begin(); }

	const char* begin() const
	{ return &*buffer_.begin(); }

	void makeSpaceBack(size_t len);
	void makeSpaceFront(size_t len);
	void moveData(int span);

private:
	std::vector<char> buffer_;
	size_t readerIndex_;
	size_t writerIndex_;
};


#endif
