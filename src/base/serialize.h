#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

// TODO: Bitpacking. Right now we are just using 32 bit data positions.

enum BitstreamMode {
	BITSTREAM_WRITE,
	BITSTREAM_READ
};

struct BitStream {
	BitstreamMode mode;

	u32* data;
	u32 position;
};

void serialize_bool(BitStream* stream, bool* value);
void serialize_u8(BitStream* stream, u8* value);
void serialize_u32(BitStream* stream, u32* value);
void serialize_i32(BitStream* stream, i32* value);
void serialize_f32(BitStream* stream, f32* value);

#ifdef CSM_BASE_IMPLEMENTATION

void serialize_bool(BitStream* stream, bool* value)
{
	if(stream->mode == BITSTREAM_WRITE) {
		stream->data[stream->position] = (u32)*value;
	} else { // BITSTREAM_READ
		*value = (bool)stream->data[stream->position];
	}
}

void serialize_u8(BitStream* stream, u8* value)
{
	if(stream->mode == BITSTREAM_WRITE) {
		stream->data[stream->position] = (u32)*value;
	} else { // BITSTREAM_READ
		*value = (u8)stream->data[stream->position];
	}
}

void serialize_u32(BitStream* stream, u32* value)
{
	if(stream->mode == BITSTREAM_WRITE) {
		stream->data[stream->position] = *value;
	} else { // BITSTREAM_READ
		*value = stream->data[stream->position];
	}
}

void serialize_i32(BitStream* stream, i32* value)
{
	if(stream->mode == BITSTREAM_WRITE) {
		stream->data[stream->position] = (u32)*value;
	} else { // BITSTREAM_READ
		*value = (i32)stream->data[stream->position];
	}
}

void serialize_f32(BitStream* stream, f32* value)
{
	if(stream->mode == BITSTREAM_WRITE) {
		stream->data[stream->position] = (u32)*value;
	} else { // BITSTREAM_READ
		*value = (f32)stream->data[stream->position];
	}
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
