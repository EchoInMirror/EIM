#include "Packets.h"

boost::shared_ptr<ByteBuffer> makePacket(unsigned int id) {
	auto buf = boost::make_shared<ByteBuffer>();
	buf->writeUInt8((unsigned char) id);
	return buf;
}

boost::shared_ptr<ByteBuffer> makeReplyPacket(unsigned int id) {
	auto buf = boost::make_shared<ByteBuffer>();
	buf->writeUInt8(0);
	buf->writeUInt32(id);
	return buf;
}
