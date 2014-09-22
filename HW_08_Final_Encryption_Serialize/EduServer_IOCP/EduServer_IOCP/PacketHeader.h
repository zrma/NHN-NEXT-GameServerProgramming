#pragma once

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

const int MessageHeaderSize = sizeof( MessageHeader );

//프로토버프 설정용인데 이거 없어도 빌드가 되네
/*
#ifndef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#else
#undef _ITERATOR_DEBUG_LEVEL
#define _ITERATOR_DEBUG_LEVEL 0
#endif
*/