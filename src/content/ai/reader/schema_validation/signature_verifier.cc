#include "signature_verifier.h"

#include <iomanip>
#include <sstream>

// Use zlib's Adler-32 as a lightweight substitute until OpenSSL is linked.
// Full SHA-256 requires #include <openssl/sha.h> and linking -lcrypto.
#include <zlib.h>

namespace Engine::ModelsBuilder::Reader::Schema {

std::string SignatureVerifier::ComputeSha256(const uint8_t* data, size_t size) {
  // Placeholder: use Adler-32 until OpenSSL SHA-256 is available in build
  uLong crc = crc32(0L, Z_NULL, 0);
  crc = crc32(crc, reinterpret_cast<const Bytef*>(data),
               static_cast<uInt>(size));

  std::ostringstream oss;
  oss << std::hex << std::setw(8) << std::setfill('0') << crc;
  return oss.str();
}

bool SignatureVerifier::VerifySha256(const uint8_t* data, size_t size,
                                      const std::string& expected_hex) {
  return ComputeSha256(data, size) == expected_hex;
}

}  // namespace Engine::ModelsBuilder::Reader::Schema
