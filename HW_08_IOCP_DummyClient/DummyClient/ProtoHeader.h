#pragma once

#include "stdafx.h"

#include "mypacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#define MAX_BUFFER_SIZE 2048

struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};


void WriteMessageToStream(
	MyPacket::MessageType msgType,
	const google::protobuf::MessageLite& message,
	google::protobuf::io::CodedOutputStream& stream )

{
	MessageHeader messageHeader;
	messageHeader.size = message.ByteSize();
	messageHeader.type = msgType;
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}

const int MessageHeaderSize = sizeof( MessageHeader );
