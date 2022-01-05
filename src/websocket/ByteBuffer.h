#pragma once

#include <boost/beast/core/flat_buffer.hpp>

class ByteBuffer: public boost::beast::flat_buffer {
public:
	ByteBuffer() : boost::beast::flat_buffer() {};
	char readInt8();
	short readInt16();
	long int readInt32();
	long long readInt64();

	void writeInt8(char val);
	void writeInt16(short val);
	void writeInt32(long int val);
	void writeInt64(long long val);
};
