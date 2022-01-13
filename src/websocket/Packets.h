#pragma once

#include "ByteBuffer.h"
#include <boost/make_shared.hpp>

enum ServerboundPacket {
	ServerboundReply,
	ServerboundSetProjectStatus,
	ServerboundGetExplorerData,
	ServerboundCreateTrack,
	ServerboundRefresh,
	ServerboundMidiMessage,
	ServerboundUpdateTrackInfo
};

enum ClientboundPacket {
	ClientboundReply,
	ClientboundProjectStatus,
	ClientboundSyncTrackInfo,
	ClientboundTrackMidiData,
	ClientboundUpdateTrackInfo
};

boost::shared_ptr<ByteBuffer> makePacket(unsigned int id);
boost::shared_ptr<ByteBuffer> makeReplyPacket(unsigned int id);
boost::shared_ptr<ByteBuffer> makeProjectStatusPacket();
boost::shared_ptr<ByteBuffer> makeTrackMidiDataPacket(int size);
boost::shared_ptr<ByteBuffer> makeAllTrackMidiDataPacket();
