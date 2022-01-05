#include "ByteBuffer.h"
#include <boost/beast/core/buffer_traits.hpp>

char ByteBuffer::readInt8() {
	char val = *(boost::asio::buffer_cast<char*>(data()));
	consume(sizeof(char));
	return val;
}

short ByteBuffer::readInt16() {
	short val = *(boost::asio::buffer_cast<short*>(data()));
	consume(sizeof(short));
	return val;
}

long int ByteBuffer::readInt32() {
	long int val = *(boost::asio::buffer_cast<long int*>(data()));
	consume(sizeof(long int));
	return val;
}

long long ByteBuffer::readInt64() {
	long long val = *(boost::asio::buffer_cast<long long*>(data()));
	consume(sizeof(long long));
	return val;
}

void ByteBuffer::writeInt8(char val) {
	*boost::asio::buffer_cast<char*>(prepare(sizeof(char))) = val;
	commit(sizeof(char));
}

void ByteBuffer::writeInt16(short val) {
	*boost::asio::buffer_cast<short*>(prepare(sizeof(short))) = val;
	commit(sizeof(short));
}

void ByteBuffer::writeInt32(long int val) {
	*boost::asio::buffer_cast<long int*>(prepare(sizeof(long int))) = val;
	commit(sizeof(long int));
}

void ByteBuffer::writeInt64(long long val) {
	*boost::asio::buffer_cast<long long*>(prepare(sizeof(long long))) = val;
	commit(sizeof(long long));
}
