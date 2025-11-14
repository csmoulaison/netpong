#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

// NOW: Implement serialization on the packet messages.
enum class SerializeMode {
	Write,
	Read
};

struct Bitstream {
	SerializeMode mode;
	u8* data;
	u32 byte_offset;
	u32 bit_offset;
};

Bitstream bitstream_init(SerializeMode mode, void* data);
void serialize_bool(Bitstream* stream, bool* value, Arena* arena);
void serialize_u8(Bitstream* stream, u8* value, Arena* arena);
void serialize_u32(Bitstream* stream, u32* value, Arena* arena);
void serialize_i32(Bitstream* stream, i32* value, Arena* arena);
void serialize_f32(Bitstream* stream, f32* value, Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

Bitstream bitstream_init(SerializeMode mode, void* data) {
	return (Bitstream) { 
		.mode = mode, 
		.data = (u8*)data, 
		.byte_offset = 0,
		.bit_offset = 0
	};
}

void bitstream_advance_bit(u32* byte_off, u32* bit_off)
{
	if(*bit_off == 7) {
		*byte_off += 1;
		*bit_off = 0;
	} else {
		*bit_off += 1;
	}
}

// TODO: Fold this into serialize_bits function.
void bitstream_write_bits(Bitstream* stream, u8* value, u32 size_bits, Arena* arena)
{
	u64 new_size_min = stream->byte_offset + size_bits / 8;
	if(arena->size <= new_size_min) {
		arena_alloc(arena, new_size_min - arena->size);
	}

	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 i = 0; i < size_bits; i++) {
		u8* cur_byte = &stream->data[stream->byte_offset];
		u8 bit_to_set = 1 << stream->bit_offset;

		// TODO: Found this alternative online. bit_is_set must be 0 or 1. Think
		// about it and test after getting serialization working generally.
		// byte = (byte & ~(1<<bit_to_set)) | (bit_is_set<<bit_to_set);
		if(value[val_byte_off] & 1 << val_bit_off) {
			*cur_byte |= bit_to_set;
		} else {
			*cur_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void bitstream_read_bits(Bitstream* stream, u8* value, u32 size_bits)
{
	*value = 0;

	u32 val_byte_off = 0;
	u32 val_bit_off = 0;
	for(u32 i = 0; i < size_bits; i++) {
		u8* cur_byte = &value[val_byte_off];
		u8 bit_to_set = 1 << val_bit_off;

		if(stream->data[stream->byte_offset] & 1 << stream->bit_offset) {
			*cur_byte |= bit_to_set;
		} else {
			*cur_byte &= ~bit_to_set;
		}

		bitstream_advance_bit(&val_byte_off, &val_bit_off);
		bitstream_advance_bit(&stream->byte_offset, &stream->bit_offset);
	}
}

void serialize_bits(Bitstream* stream, u8* value, u32 size_bits, Arena* arena)
{
	strict_assert(
		(arena == nullptr && stream->mode == SerializeMode::Write) || 
		(arena != nullptr && stream->mode == SerializeMode::Read));

	if(stream->mode == SerializeMode::Write) {
		bitstream_write_bits(stream, value, 1, arena);
	} else {
		bitstream_read_bits(stream, value, 1);
	}
}

void serialize_bool(Bitstream* stream, bool* value, Arena* arena)
{
	serialize_bits(stream, (u8*)value, 1, arena);
}

void serialize_u8(Bitstream* stream, u8* value, Arena* arena)
{
	serialize_bits(stream, (u8*)value, 8, arena);
}

void serialize_u32(Bitstream* stream, u32* value, Arena* arena)
{
	serialize_bits(stream, (u8*)value, 32, arena);
}

void serialize_i32(Bitstream* stream, i32* value, Arena* arena)
{
	serialize_bits(stream, (u8*)value, 32, arena);
}

void serialize_f32(Bitstream* stream, f32* value, Arena* arena)
{
	serialize_bits(stream, (u8*)value, 32, arena);
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
