#include "Packets.h"
#include "../Main.h"

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

boost::shared_ptr<ByteBuffer> makeProjectStatusPacket() {
	auto &masterTrack = EIMApplication::getEIMInstance()->mainWindow->masterTrack;
	auto &info = masterTrack->currentPositionInfo;
	auto buf = makePacket(ClientboundPacket::ClientboundProjectStatus);
	buf->writeInt16(1);
	buf->writeDouble(info.bpm);
	buf->writeDouble(info.ppqPosition);
	buf->writeBoolean(info.isPlaying);
	buf->writeUInt8(info.timeSigNumerator);
	buf->writeUInt8(info.timeSigDenominator);
	buf->writeUInt16((unsigned short)masterTrack->getSampleRate());
	return buf;
}
