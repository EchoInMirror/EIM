#pragma once

#include "ByteBuffer.h"
#include <boost/make_shared.hpp>

enum ServerboundPacket {
	ServerboundReply,
	ServerboundGetExplorerData,
	ServerboundCreateTrack,
	ServerboundRefresh,
	ServerboundMidiMessage
};

enum ClientboundPacket {
	ClientboundReply,
	ClientboundProjectStatus,
	ClientboundSyncTrackInfo
};

boost::shared_ptr<ByteBuffer> makePacket(unsigned int id);

boost::shared_ptr<ByteBuffer> makeReplyPacket(unsigned int id);

boost::shared_ptr<ByteBuffer> makeProjectStatusPacket();
