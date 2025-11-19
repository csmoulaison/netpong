#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

enum class SerializeMode { Write, Read };

struct Bitstream {
	SerializeMode mode;
	u32 byte_offset;
	u32 bit_offset;

	char* data;
	Arena* arena;
	u64 arena_offset;
};

struct SerializeResult {
	u64 size_bytes;
	char* data;
};

Bitstream bitstream_init(SerializeMode mode, char* data, Arena* arena);
SerializeResult serialize_result(Bitstream* stream);
void serialize_bool(Bitstream* stream, bool* value);
void serialize_u8(Bitstream* stream, u8* value);
void serialize_u32(Bitstream* stream, u32* value);
void serialize_i32(Bitstream* stream, i32* value);
void serialize_f32(Bitstream* stream, f32* value);

#ifdef CSM_BASE_IMPLEMENTATION

Bitstream bitstream_init(SerializeMode mode, char* data, Arena* arena)
{
	assert((arena == nullptr && mode == SerializeMode::Read) || 
		   (arena != nullptr && mode == SerializeMode::Write));

	Bitstream stream = (Bitstream) {
		.mode = mode, 
		.byte_offset = 0,
		.bit_offset = 0,
		.arena = arena,
		.arena_offset = 0
	};
	if(stream.mode == SerializeMode::Write) {
		stream.data = (char*)arena_head(arena);
		stream.arena_offset = arena->index;
	} else {
		stream.data = data;
	}
	return stream;
}

SerializeResult serialize_result(Bitstream* stream)
{
	return (SerializeResult) { .size_bytes = stream->byte_offset + 1, .data = stream->data };
}

void bitstream_advance_bit(u32* byte_off, u32* bit_off)
{
	u32 original_bit = *bit_off;
	u32 original_byte = *byte_off;

	if(*bit_off == 7) {
		*byte_off += 1;
		*bit_off = 0;
	} else {
		*bit_off += 1;
	}

	assert(*bit_off == 0 || *bit_off == original_bit + 1);
	assert(*byte_off == original_byte || *byte_off == original_byte + 1);
}

void bitstream_write_bits(Bitstream* stream, char* value, u32 size_bits)
{
	u64 new_size_min = stream->arena_offset + stream->byte_offset + size_bits / 8;
	if(stream->arena->index <= new_size_min) {
		arena_alloc(stream->arena, new_size_min - stream->arena->index);
	}

	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 i = 0; i < size_bits; i++) {
		char* write_byte = &stream->data[stream->byte_offset];
		u8 bit_to_set = 1 << stream->bit_offset;

		// TODO: Found this alternative online. bit_is_set must be 0 or 1. Think
		// about it and test after getting serialization working generally.
		// byte = (byte & ~(1<<bit_to_set)) | (bit_is_set<<bit_to_set);
		if(value[val_byte_off] & 1 << val_bit_off) {
			*write_byte |= bit_to_set;
		} else {
			*write_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void bitstream_read_bits(Bitstream* stream, char* value, u32 size_bits)
{
	*value = 0;
	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 i = 0; i < size_bits; i++) {
		char* write_byte = &value[val_byte_off];
		u8 bit_to_set = 1 << val_bit_off;

		if(stream->data[stream->byte_offset] & 1 << stream->bit_offset) {
			*write_byte |= bit_to_set;
		} else {
			*write_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void serialize_bits(Bitstream* stream, char* value, u32 size_bits)
{
	if(stream->mode == SerializeMode::Write) {
		bitstream_write_bits(stream, value, size_bits);
	} else {
		bitstream_read_bits(stream, value, size_bits);
	}
}

void serialize_bool(Bitstream* stream, bool* value)
{
	serialize_bits(stream, (char*)value, 1);
}

void serialize_u8(Bitstream* stream, u8* value)
{
	serialize_bits(stream, (char*)value, 8);
}

void serialize_u32(Bitstream* stream, u32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

void serialize_i32(Bitstream* stream, i32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

void serialize_f32(Bitstream* stream, f32* value)
{
	serialize_bits(stream, (char*)value, 32);
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
