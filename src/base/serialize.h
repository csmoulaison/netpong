#ifndef serialize_h_INCLUDED
#define serialize_h_INCLUDED

enum SerializeMode {
	Reading,
	Writing
};

struct BitStream {
	void* data;
	SerializeMode mode;
};

void serialize_write_bool(BitStream* stream, bool* value);

#ifdef CSM_BASE_IMPLEMENTATION

void serialize_write_bool(BitStream* stream, bool* value)
{
}

#endif // CSM_BASE_IMPLEMENTATION
#endif // serialize_h_INCLUDED
