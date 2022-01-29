#include "ByteBuffer.h"
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/core/buffer_traits.hpp>

bool ByteBuffer::readBoolean() {
	return (bool)readUInt8();
}

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
	consume(sizeof(unsigned long long));
	return val;
}

unsigned char ByteBuffer::readUInt8() {
	unsigned char val = *(boost::asio::buffer_cast<unsigned char*>(data()));
	consume(sizeof(unsigned char));
	return val;
}

unsigned short ByteBuffer::readUInt16() {
	unsigned short val = *(boost::asio::buffer_cast<unsigned short*>(data()));
	consume(sizeof(unsigned short));
	return val;
}

unsigned long int ByteBuffer::readUInt32() {
	unsigned long int val = *(boost::asio::buffer_cast<unsigned long int*>(data()));
	consume(sizeof(unsigned long int));
	return val;
}

unsigned long long ByteBuffer::readUInt64() {
	unsigned long long val = *(boost::asio::buffer_cast<unsigned long long*>(data()));
	consume(sizeof(unsigned long long));
	return val;
}

float ByteBuffer::readFloat() {
	float val = *(boost::asio::buffer_cast<float*>(data()));
	consume(sizeof(float));
	return val;
}

double ByteBuffer::readDouble() {
	double val = *(boost::asio::buffer_cast<double*>(data()));
	consume(sizeof(double));
	return val;
}

std::string ByteBuffer::readString() {
	auto len = readUInt32();
	std::string result(boost::asio::buffer_cast<const char*>(data()), len);
	consume(sizeof(const char) * len);
	return result;
}

void ByteBuffer::writeBoolean(bool val) {
	writeUInt8(val);
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

void ByteBuffer::writeUInt8(unsigned char val) {
	*boost::asio::buffer_cast<unsigned char*>(prepare(sizeof(unsigned char))) = val;
	commit(sizeof(unsigned char));
}

void ByteBuffer::writeUInt16(unsigned short val) {
	*boost::asio::buffer_cast<unsigned short*>(prepare(sizeof(unsigned short))) = val;
	commit(sizeof(unsigned short));
}

void ByteBuffer::writeUInt32(unsigned long int val) {
	*boost::asio::buffer_cast<long int*>(prepare(sizeof(unsigned long int))) = val;
	commit(sizeof(unsigned long int));
}

void ByteBuffer::writeUInt64(unsigned long long val) {
	*boost::asio::buffer_cast<unsigned long long*>(prepare(sizeof(unsigned long long))) = val;
	commit(sizeof(unsigned long long));
}

void ByteBuffer::writeFloat(float val) {
	*boost::asio::buffer_cast<float*>(prepare(sizeof(float))) = val;
	commit(sizeof(float));
}

void ByteBuffer::writeDouble(double val) {
	*boost::asio::buffer_cast<double*>(prepare(sizeof(double))) = val;
	commit(sizeof(double));
}

void ByteBuffer::writeString(const char* val) {
	auto len = strlen(val);
	DBG("" << len);
	writeUInt32((unsigned long)len);
	if (!len) return;
	auto size = sizeof(const char) * len;
	memcpy(boost::asio::buffer_cast<char*>(prepare(size)), val, size);
	commit(size);
}

void ByteBuffer::writeString(std::string val) {
	auto len = val.length();
	writeUInt32((unsigned long) len);
	if (!len) return;
	auto size = sizeof(const char) * len;
	val.copy(boost::asio::buffer_cast<char*>(prepare(size)), len);
	commit(size);
}

void ByteBuffer::writeString(juce::String val) {
	writeString(val.toStdString());
}

void ByteBuffer::writeUUID(juce::Uuid val) {
	writeUInt64(val.getNode());
}
