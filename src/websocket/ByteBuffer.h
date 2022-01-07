#pragma once

#include <juce_core/juce_core.h>
#include <boost/beast/core/flat_buffer.hpp>

class ByteBuffer: public boost::beast::flat_buffer {
public:
	ByteBuffer() : boost::beast::flat_buffer() {};
	char readInt8();
	short readInt16();
	long int readInt32();
	long long readInt64();
	unsigned char readUInt8();
	unsigned short readUInt16();
	unsigned long int readUInt32();
	unsigned long long readUInt64();
	std::string readString();

	void writeInt8(char val);
	void writeInt16(short val);
	void writeInt32(long int val);
	void writeInt64(long long val);
	void writeUInt8(unsigned char val);
	void writeUInt16(unsigned short val);
	void writeUInt32(unsigned long int val);
	void writeUInt64(unsigned long long val);
	void writeString(std::string val);
	void writeString(juce::String val);
};
