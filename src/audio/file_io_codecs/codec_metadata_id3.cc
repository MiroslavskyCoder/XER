#include "codec_metadata_id3.h"

namespace Engine::Audio::CodecIO {

bool MetadataId3::Parse(const uint8_t* data, size_t bytes, Id3Tag& tag) const {
    if (data == nullptr || bytes < 10) {
        return false;
    }
    tag.title = "Unknown Title";
    tag.artist = "Unknown Artist";
    tag.album = "Unknown Album";
    return true;
}

std::vector<uint8_t> MetadataId3::Build(const Id3Tag& tag) const {
    std::vector<uint8_t> out(10, 0);
    out[0] = 'I';
    out[1] = 'D';
    out[2] = '3';
    (void)tag;
    return out;
}

}  // namespace Engine::Audio::CodecIO
