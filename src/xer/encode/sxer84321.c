#include "sxer84321.h"

#include <string.h>

enum {
	SXER84321_MIN_MATCH = 4,
	SXER84321_MAX_MATCH = 10,
	SXER84321_MAX_OFFSET = 4096,
	SXER84321_MAX_LITERAL = 128
};

static uint8_t sxer84321_mask(uint64_t nonce, size_t position) {
	uint64_t value = nonce ^ (0x9E3779B97F4A7C15ULL + ((uint64_t)position * 0xBF58476D1CE4E5B9ULL));
	value ^= value >> 30;
	value *= 0xBF58476D1CE4E5B9ULL;
	value ^= value >> 27;
	value *= 0x94D049BB133111EBULL;
	value ^= value >> 31;
	return (uint8_t)(value & 0xFFu);
}

uint32_t sxer84321_checksum(const uint8_t* data, size_t size) {
	uint32_t hash = 2166136261u;
	size_t index;

	if (data == NULL) {
		return 0u;
	}

	for (index = 0; index < size; ++index) {
		hash ^= (uint32_t)data[index];
		hash *= 16777619u;
	}
	return hash;
}

size_t sxer84321_max_encoded_size(size_t input_size) {
	return input_size + (input_size / SXER84321_MAX_LITERAL) + 32u;
}

const char* sxer84321_status_string(Sxer84321Status status) {
	switch (status) {
		case SXER84321_STATUS_OK:
			return "ok";
		case SXER84321_STATUS_INVALID_ARGUMENT:
			return "invalid_argument";
		case SXER84321_STATUS_BUFFER_TOO_SMALL:
			return "buffer_too_small";
		case SXER84321_STATUS_CORRUPTED_DATA:
			return "corrupted_data";
	}
	return "unknown";
}

static void sxer84321_find_match(
	const uint8_t* input,
	size_t input_size,
	size_t position,
	size_t* best_length,
	size_t* best_offset) {
	size_t window_start;
	size_t cursor;

	*best_length = 0u;
	*best_offset = 0u;

	if (position == 0u) {
		return;
	}

	window_start = position > SXER84321_MAX_OFFSET ? position - SXER84321_MAX_OFFSET : 0u;
	for (cursor = position; cursor > window_start; --cursor) {
		size_t candidate = cursor - 1u;
		size_t length = 0u;
		while (length < SXER84321_MAX_MATCH
			   && position + length < input_size
			   && input[candidate + length] == input[position + length]) {
			++length;
		}

		if (length >= SXER84321_MIN_MATCH && length > *best_length) {
			*best_length = length;
			*best_offset = position - candidate;
			if (length == SXER84321_MAX_MATCH) {
				return;
			}
		}
	}
}

Sxer84321Status sxer84321_encode(
	const uint8_t* input,
	size_t input_size,
	uint64_t nonce,
	uint8_t* output,
	size_t output_capacity,
	size_t* output_size,
	uint32_t* out_source_crc,
	uint32_t* out_payload_crc) {
	size_t in_pos = 0u;
	size_t out_pos = 0u;
	size_t mask_pos = 0u;

	if (input == NULL || output == NULL || output_size == NULL) {
		return SXER84321_STATUS_INVALID_ARGUMENT;
	}

	while (in_pos < input_size) {
		size_t match_length = 0u;
		size_t match_offset = 0u;
		sxer84321_find_match(input, input_size, in_pos, &match_length, &match_offset);

		if (match_length >= SXER84321_MIN_MATCH) {
			size_t offset_minus_one = match_offset - 1u;
			if (out_pos + 2u > output_capacity) {
				return SXER84321_STATUS_BUFFER_TOO_SMALL;
			}
			output[out_pos++] = (uint8_t)(0x80u | (((match_length - 3u) & 0x07u) << 4u) | ((offset_minus_one >> 8u) & 0x0Fu));
			output[out_pos++] = (uint8_t)(offset_minus_one & 0xFFu);
			in_pos += match_length;
			continue;
		}

		{
			size_t literal_start = in_pos;
			size_t literal_length = 1u;
			++in_pos;

			while (in_pos < input_size && literal_length < SXER84321_MAX_LITERAL) {
				size_t next_length = 0u;
				size_t next_offset = 0u;
				sxer84321_find_match(input, input_size, in_pos, &next_length, &next_offset);
				if (next_length >= SXER84321_MIN_MATCH) {
					break;
				}
				++in_pos;
				++literal_length;
			}

			if (out_pos + 1u + literal_length > output_capacity) {
				return SXER84321_STATUS_BUFFER_TOO_SMALL;
			}

			output[out_pos++] = (uint8_t)(literal_length - 1u);
			while (literal_length-- > 0u) {
				output[out_pos++] = (uint8_t)(input[literal_start] ^ sxer84321_mask(nonce, mask_pos++));
				++literal_start;
			}
		}
	}

	*output_size = out_pos;
	if (out_source_crc != NULL) {
		*out_source_crc = sxer84321_checksum(input, input_size);
	}
	if (out_payload_crc != NULL) {
		*out_payload_crc = sxer84321_checksum(output, out_pos);
	}
	return SXER84321_STATUS_OK;
}

Sxer84321Status sxer84321_decode(
	const uint8_t* input,
	size_t input_size,
	uint64_t nonce,
	uint8_t* output,
	size_t output_capacity,
	size_t* output_size,
	uint32_t expected_source_crc,
	uint32_t expected_payload_crc) {
	size_t in_pos = 0u;
	size_t out_pos = 0u;
	size_t mask_pos = 0u;

	if (input == NULL || output == NULL || output_size == NULL) {
		return SXER84321_STATUS_INVALID_ARGUMENT;
	}

	if (expected_payload_crc != 0u && sxer84321_checksum(input, input_size) != expected_payload_crc) {
		return SXER84321_STATUS_CORRUPTED_DATA;
	}

	while (in_pos < input_size) {
		uint8_t tag = input[in_pos++];
		if ((tag & 0x80u) != 0u) {
			size_t length;
			size_t offset;
			size_t copy_index;

			if (in_pos >= input_size) {
				return SXER84321_STATUS_CORRUPTED_DATA;
			}

			length = ((size_t)((tag >> 4u) & 0x07u)) + 3u;
			offset = ((((size_t)tag) & 0x0Fu) << 8u) | (size_t)input[in_pos++];
			offset += 1u;

			if (offset > out_pos || out_pos + length > output_capacity) {
				return SXER84321_STATUS_CORRUPTED_DATA;
			}

			for (copy_index = 0u; copy_index < length; ++copy_index) {
				output[out_pos] = output[out_pos - offset];
				++out_pos;
			}
		} else {
			size_t length = ((size_t)tag) + 1u;
			size_t copy_index;

			if (in_pos + length > input_size || out_pos + length > output_capacity) {
				return SXER84321_STATUS_CORRUPTED_DATA;
			}

			for (copy_index = 0u; copy_index < length; ++copy_index) {
				output[out_pos++] = (uint8_t)(input[in_pos++] ^ sxer84321_mask(nonce, mask_pos++));
			}
		}
	}

	if (expected_source_crc != 0u && sxer84321_checksum(output, out_pos) != expected_source_crc) {
		return SXER84321_STATUS_CORRUPTED_DATA;
	}

	*output_size = out_pos;
	return SXER84321_STATUS_OK;
}
