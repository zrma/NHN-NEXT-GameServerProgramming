#pragma once

#include "mypacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#define MAX_BUFFER_SIZE 2048

struct MessageHeader
{
	MessageHeader(): mSize( 0 ), mType( -1 ) {}
	short mSize;
	short mType;
};

const int MessageHeaderSize = sizeof( MessageHeader );
