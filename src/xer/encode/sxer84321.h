#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SXER84321_MAGIC 0x31525853u
#define SXER84321_VERSION 0x00010021u

typedef enum Sxer84321Status {
	SXER84321_STATUS_OK = 0,
	SXER84321_STATUS_INVALID_ARGUMENT = 1,
	SXER84321_STATUS_BUFFER_TOO_SMALL = 2,
	SXER84321_STATUS_CORRUPTED_DATA = 3
} Sxer84321Status;

size_t sxer84321_max_encoded_size(size_t input_size);
uint32_t sxer84321_checksum(const uint8_t* data, size_t size);
const char* sxer84321_status_string(Sxer84321Status status);

Sxer84321Status sxer84321_encode(
	const uint8_t* input,
	size_t input_size,
	uint64_t nonce,
	uint8_t* output,
	size_t output_capacity,
	size_t* output_size,
	uint32_t* out_source_crc,
	uint32_t* out_payload_crc);

Sxer84321Status sxer84321_decode(
	const uint8_t* input,
	size_t input_size,
	uint64_t nonce,
	uint8_t* output,
	size_t output_capacity,
	size_t* output_size,
	uint32_t expected_source_crc,
	uint32_t expected_payload_crc);

#ifdef __cplusplus
}
#endif
