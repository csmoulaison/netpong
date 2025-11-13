#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

// NOW: Implement serialization on the packets, first with u32s as now, then
// implement bit packing.
enum class SerializeMode {
	Write,
	Read
};

struct BitStream {
	SerializeMode mode;
	u32* data;
	u64 position;
};

BitStream bitstream_init(SerializeMode mode, void* data);
void serialize_bool(BitStream* stream, bool* value, Arena* arena);
void serialize_u8(BitStream* stream, u8* value, Arena* arena);
void serialize_u32(BitStream* stream, u32* value, Arena* arena);
void serialize_i32(BitStream* stream, i32* value, Arena* arena);
void serialize_f32(BitStream* stream, f32* value, Arena* arena);

#ifdef CSM_BASE_IMPLEMENTATION

BitStream bitstream_init(SerializeMode mode, void* data) {
	return (BitStream){ .mode = mode, .data = (u32*)data, .position = 0 };
}

void bitstream_resize_arena(BitStream* stream, Arena* arena, u32 new_bits)
{
	if(stream->mode == SerializeMode::Write) {
		u64 new_size_min = stream->position / 8 + new_bits / 8;
		if(arena->size <= new_size_min) {
			arena_alloc(arena, new_size_min - arena->size);
		}
	}
}

void serialize_bool(BitStream* stream, bool* value, Arena* arena)
{
	bitstream_resize_arena(stream, arena, sizeof(u32));
	if(stream->mode == SerializeMode::Write) {
		stream->data[stream->position] = (u32)*value;
	} else { // Read
		*value = (bool)stream->data[stream->position];
	}
}

void serialize_u8(BitStream* stream, u8* value, Arena* arena)
{
	bitstream_resize_arena(stream, arena, sizeof(u32));
	if(stream->mode == SerializeMode::Write) {
		stream->data[stream->position] = (u32)*value;
	} else { // Read
		*value = (u8)stream->data[stream->position];
	}
}

void serialize_u32(BitStream* stream, u32* value, Arena* arena)
{
	if(stream->mode == SerializeMode::Write) {
		stream->data[stream->position] = *value;
	} else { // Read
		*value = stream->data[stream->position];
	}
}

void serialize_i32(BitStream* stream, i32* value, Arena* arena)
{
	if(stream->mode == SerializeMode::Write) {
		stream->data[stream->position] = (u32)*value;
	} else { // Read
		*value = (i32)stream->data[stream->position];
	}
}

void serialize_f32(BitStream* stream, f32* value, Arena* arena)
{
	if(stream->mode == SerializeMode::Write) {
		stream->data[stream->position] = (u32)*value;
	} else { // Read
		*value = (f32)stream->data[stream->position];
	}
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
