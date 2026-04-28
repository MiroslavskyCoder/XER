#include "audio_modules_smoke.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "audio/file_io_codecs/codec_file_streamer.h"
#include "audio/file_io_codecs/codec_aac_adts.h"
#include "audio/file_io_codecs/codec_metadata_id3.h"
#include "audio/file_io_codecs/codec_mp3_lame.h"
#include "audio/file_io_codecs/codec_ogg_vorbis.h"
#include "audio/file_io_codecs/codec_wav_float.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"
#include "audio/midi_sequencing/midi_clock_generator.h"
#include "audio/midi_sequencing/midi_controller_mapping.h"
#include "audio/midi_sequencing/midi_event_dispatcher.h"
#include "audio/midi_sequencing/midi_parser.h"
#include "audio/midi_sequencing/midi_pattern_sequencer.h"
#include "audio/midi_sequencing/midi_sysex_handler.h"
#include "audio/plugin_wrappers/au_host_interface.h"
#include "audio/plugin_wrappers/clap_host_interface.h"
#include "audio/plugin_wrappers/vst3_host_interface.h"
#include "wrapper/ffmpeg/ffmpeg_engine_bridge.h"
#include <cmath>

namespace Engine::Audio::Demo {

namespace {

struct SignalStats {
	float peak = 0.0f;
	double rms = 0.0;
};

SignalStats ComputeSignalStats(const std::vector<float>& samples) {
	SignalStats stats;
	if (samples.empty()) {
		return stats;
	}
	double sum_squared = 0.0;
	for (float sample : samples) {
		const float absolute = std::abs(sample);
		stats.peak = std::max(stats.peak, absolute);
		sum_squared += static_cast<double>(sample) * static_cast<double>(sample);
	}
	stats.rms = std::sqrt(sum_squared / static_cast<double>(samples.size()));
	return stats;
}

bool ReadInputBytes(const std::filesystem::path& path, std::vector<uint8_t>* bytes_out, std::string* error_out) {
	if (bytes_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "byte output target is null";
		}
		return false;
	}
	std::error_code size_error;
	const std::uintmax_t file_size = std::filesystem::file_size(path, size_error);
	if (size_error) {
		if (error_out != nullptr) {
			*error_out = "failed to determine input size: " + path.string();
		}
		return false;
	}

	Engine::Audio::CodecIO::CodecFileStreamer streamer;
	if (!streamer.OpenRead(path.string())) {
		if (error_out != nullptr) {
			*error_out = "failed to open input through CodecFileStreamer: " + path.string();
		}
		return false;
	}

	bytes_out->assign(static_cast<size_t>(file_size), 0);
	const size_t bytes_read = streamer.ReadBytes(bytes_out->data(), bytes_out->size());
	streamer.Close();
	if (bytes_read != bytes_out->size()) {
		if (error_out != nullptr) {
			*error_out = "failed to read complete input through CodecFileStreamer";
		}
		return false;
	}
	return true;
}

int DetectSampleRate(const std::filesystem::path& path) {
	engine::bridge::ffmpeg::MediaInfo media_info;
	if (!engine::bridge::ffmpeg::ProbeMedia(path.string(), &media_info, nullptr)) {
		return 44100;
	}
	for (const auto& stream : media_info.streams) {
		if (stream.media_type == "audio" && stream.sample_rate > 0) {
			return stream.sample_rate;
		}
	}
	return 44100;
}

template <typename Wrapper>
bool ProcessInBlocks(Wrapper* wrapper, const std::vector<float>& input, std::vector<float>* output, uint32_t block_size) {
	if (wrapper == nullptr || output == nullptr || block_size == 0) {
		return false;
	}
	output->assign(input.size(), 0.0f);
	std::vector<float> scratch(block_size, 0.0f);
	for (size_t cursor = 0; cursor < input.size(); cursor += block_size) {
		const uint32_t frames = static_cast<uint32_t>(std::min<size_t>(block_size, input.size() - cursor));
		if (!wrapper->Process(input.data() + static_cast<std::ptrdiff_t>(cursor), scratch.data(), frames)) {
			return false;
		}
		std::copy(scratch.begin(), scratch.begin() + frames, output->begin() + static_cast<std::ptrdiff_t>(cursor));
	}
	return true;
}

bool WriteWaveArtifact(const std::filesystem::path& path, const std::vector<float>& samples, int sample_rate) {
	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<uint8_t> bytes;
	if (!codec.Encode16(samples.data(), samples.size(), bytes, sample_rate)) {
		return false;
	}
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open()) {
		return false;
	}
	output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	return static_cast<bool>(output);
}

std::filesystem::path ResolveExternalClapSmokePluginPath() {
	std::error_code path_error;
	const std::filesystem::path executable_path = std::filesystem::read_symlink("/proc/self/exe", path_error);
	if (path_error) {
		return {};
	}
	const std::filesystem::path candidate = executable_path.parent_path() / "EngineClapSmokePlugin.clap";
	return std::filesystem::exists(candidate) ? candidate : std::filesystem::path{};
}

}  // namespace

bool RunAudioModulesSmoke(
	const AudioModulesSmokeOptions& options,
	std::string* report,
	std::string* error_out) {
	if (report == nullptr) {
		if (error_out != nullptr) {
			*error_out = "report target is null";
		}
		return false;
	}
	if (options.input_path.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio_modules_smoke requires an input file";
		}
		return false;
	}

	std::vector<uint8_t> input_bytes;
	if (!ReadInputBytes(options.input_path, &input_bytes, error_out)) {
		return false;
	}

	Engine::Audio::CodecIO::Mp3LameCodec mp3_codec;
	std::vector<float> decoded_mono;
	if (!mp3_codec.Decode(input_bytes.data(), input_bytes.size(), decoded_mono)) {
		if (error_out != nullptr) {
			*error_out = "direct MP3 decode failed for smoke input";
		}
		return false;
	}
	const int sample_rate = DetectSampleRate(options.input_path);
	const size_t smoke_frames = std::min<size_t>(decoded_mono.size(), 32768u);
	std::vector<float> smoke_input(decoded_mono.begin(), decoded_mono.begin() + static_cast<std::ptrdiff_t>(smoke_frames));
	if (smoke_input.empty()) {
		if (error_out != nullptr) {
			*error_out = "decoded MP3 smoke buffer is empty";
		}
		return false;
	}

	Engine::Audio::MIDI::MidiParser midi_parser;
	const std::vector<uint8_t> midi_bytes = {
		0x90, 60, 100,
		64, 110,
		0xB0, 74, 127,
		0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7,
		0xFA,
	};
	std::vector<Engine::Audio::MIDI::MidiEvent> midi_events;
	if (!midi_parser.ParseStream(midi_bytes.data(), midi_bytes.size(), midi_events)) {
		if (error_out != nullptr) {
			*error_out = "MIDI parser smoke sequence failed";
		}
		return false;
	}

	Engine::Audio::MIDI::MidiEventDispatcher dispatcher;
	size_t dispatched_events = 0;
	dispatcher.AddListener([&dispatched_events](const Engine::Audio::MIDI::MidiEvent&) {
		++dispatched_events;
	});
	dispatcher.DispatchBatch(midi_events);

	Engine::Audio::MIDI::MidiControllerMapping controller_mapping;
	controller_mapping.Bind(74, 0, -12.0f, 12.0f, false, 0, "gain_db");
	uint32_t mapped_param_id = 0;
	float mapped_gain_db = 0.0f;
	bool have_mapped_value = false;
	for (const auto& event : midi_events) {
		if (controller_mapping.MapEvent(event, &mapped_param_id, &mapped_gain_db)) {
			have_mapped_value = true;
			break;
		}
	}
	if (!have_mapped_value) {
		if (error_out != nullptr) {
			*error_out = "failed to map MIDI CC event to plugin parameter";
		}
		return false;
	}

	Engine::Audio::MIDI::MidiSysexHandler sysex_handler;
	Engine::Audio::MIDI::MidiSysexMessage sysex_message;
	bool parsed_sysex = false;
	for (const auto& event : midi_events) {
		if (event.type == Engine::Audio::MIDI::MidiMessageType::kSysEx) {
			parsed_sysex = sysex_handler.Parse(event.message, &sysex_message);
			break;
		}
	}
	if (!parsed_sysex) {
		if (error_out != nullptr) {
			*error_out = "failed to parse MIDI SysEx smoke event";
		}
		return false;
	}

	Engine::Audio::MIDI::MidiPatternSequencer sequencer;
	sequencer.SetPattern(midi_events);
	sequencer.SetLoopLength(4);
	const std::vector<Engine::Audio::MIDI::MidiEvent> looped_events = sequencer.CollectRange(0, 8);

	Engine::Audio::MIDI::MidiClockGenerator clock_generator;
	clock_generator.SetTempoBpm(120.0);
	const uint32_t clock_pulses = clock_generator.AdvanceSamples(static_cast<uint32_t>(sample_rate), static_cast<uint32_t>(sample_rate));

	Engine::Audio::Plugin::Vst3HostInterface vst3_host;
	if (!vst3_host.Initialize(static_cast<double>(sample_rate), 1024u)
		|| !vst3_host.LoadPlugin("builtin://gain")
		|| !vst3_host.SetParameter(mapped_param_id, mapped_gain_db)) {
		if (error_out != nullptr) {
			*error_out = "failed to initialize built-in VST3 host smoke processor";
		}
		return false;
	}
	std::vector<float> vst3_output;
	if (!ProcessInBlocks(&vst3_host, smoke_input, &vst3_output, 1024u)) {
		if (error_out != nullptr) {
			*error_out = "VST3 built-in smoke processing failed";
		}
		return false;
	}

	Engine::Audio::Plugin::ClapHostInterface clap_host;
	const std::filesystem::path external_clap_plugin = ResolveExternalClapSmokePluginPath();
	const bool use_external_clap = !external_clap_plugin.empty();
	if (!clap_host.Initialize(static_cast<double>(sample_rate), 1024u)
		|| !clap_host.LoadPlugin(use_external_clap ? external_clap_plugin.string() : "builtin://chorus")
		|| !clap_host.SetParameter(0u, use_external_clap ? mapped_gain_db : 0.9f)
		|| (!use_external_clap && !clap_host.SetParameter(2u, 0.45f))) {
		if (error_out != nullptr) {
			*error_out = "failed to initialize CLAP host smoke processor";
		}
		return false;
	}
	std::vector<float> clap_output;
	if (!ProcessInBlocks(&clap_host, smoke_input, &clap_output, 1024u)) {
		if (error_out != nullptr) {
			*error_out = "CLAP built-in smoke processing failed";
		}
		return false;
	}

	Engine::Audio::Plugin::AuHostInterface au_host;
	if (!au_host.Initialize(static_cast<double>(sample_rate), 1024u)
		|| !au_host.LoadComponent("builtin://parametric_eq")
		|| !au_host.SetParameter(0u, 1800.0f)
		|| !au_host.SetParameter(1u, 0.8f)
		|| !au_host.SetParameter(2u, 6.0f)) {
		if (error_out != nullptr) {
			*error_out = "failed to initialize built-in AU host smoke processor";
		}
		return false;
	}
	std::vector<float> au_output;
	if (!ProcessInBlocks(&au_host, smoke_input, &au_output, 1024u)) {
		if (error_out != nullptr) {
			*error_out = "AU built-in smoke processing failed";
		}
		return false;
	}

	Engine::Audio::CodecIO::WavFloatCodec wav_float_codec;
	std::vector<uint8_t> wav_float_bytes;
	if (!wav_float_codec.Encode32(smoke_input.data(), std::min<size_t>(smoke_input.size(), 4096u), wav_float_bytes)) {
		if (error_out != nullptr) {
			*error_out = "WAV float encode smoke failed";
		}
		return false;
	}
	std::vector<float> wav_float_roundtrip;
	if (!wav_float_codec.Decode32(wav_float_bytes.data(), wav_float_bytes.size(), wav_float_roundtrip)) {
		if (error_out != nullptr) {
			*error_out = "WAV float decode smoke failed";
		}
		return false;
	}

	Engine::Audio::CodecIO::MetadataId3 id3_codec;
	const Engine::Audio::CodecIO::Id3Tag synthetic_tag{"hoodak", "smoke", "modules"};
	const std::vector<uint8_t> id3_bytes = id3_codec.Build(synthetic_tag);
	Engine::Audio::CodecIO::Id3Tag parsed_tag;
	if (!id3_codec.Parse(id3_bytes.data(), id3_bytes.size(), parsed_tag)) {
		if (error_out != nullptr) {
			*error_out = "ID3 parse/build smoke failed";
		}
		return false;
	}

	Engine::Audio::CodecIO::AacAdtsCodec aac_codec;
	std::vector<uint8_t> aac_bytes;
	if (!aac_codec.Encode(smoke_input.data(), std::min<size_t>(smoke_input.size(), 8192u), aac_bytes)) {
		if (error_out != nullptr) {
			*error_out = "AAC encode smoke failed";
		}
		return false;
	}
	std::vector<float> aac_roundtrip;
	if (!aac_codec.Decode(aac_bytes.data(), aac_bytes.size(), aac_roundtrip)) {
		if (error_out != nullptr) {
			*error_out = "AAC decode smoke failed after encode";
		}
		return false;
	}

	Engine::Audio::CodecIO::OggVorbisCodec ogg_codec;
	std::vector<uint8_t> ogg_bytes;
	if (!ogg_codec.Encode(smoke_input.data(), std::min<size_t>(smoke_input.size(), 8192u), ogg_bytes)) {
		if (error_out != nullptr) {
			*error_out = "Ogg/Vorbis encode smoke failed";
		}
		return false;
	}
	std::vector<float> ogg_roundtrip;
	if (!ogg_codec.Decode(ogg_bytes.data(), ogg_bytes.size(), ogg_roundtrip)) {
		if (error_out != nullptr) {
			*error_out = "Ogg/Vorbis decode smoke failed after encode";
		}
		return false;
	}

	if (!options.output_dir.empty()) {
		std::error_code fs_error;
		std::filesystem::create_directories(options.output_dir, fs_error);
		if (fs_error) {
			if (error_out != nullptr) {
				*error_out = "failed to create smoke output directory";
			}
			return false;
		}
		if (!WriteWaveArtifact(options.output_dir / "vst3_gain.wav", vst3_output, sample_rate)
			|| !WriteWaveArtifact(options.output_dir / "clap_chorus.wav", clap_output, sample_rate)
			|| !WriteWaveArtifact(options.output_dir / "au_parametric_eq.wav", au_output, sample_rate)) {
			if (error_out != nullptr) {
				*error_out = "failed to write smoke wave artifacts";
			}
			return false;
		}
	}

	const SignalStats input_stats = ComputeSignalStats(smoke_input);
	const SignalStats vst3_stats = ComputeSignalStats(vst3_output);
	const SignalStats clap_stats = ComputeSignalStats(clap_output);
	const SignalStats au_stats = ComputeSignalStats(au_output);

	std::ostringstream output;
	output << std::fixed << std::setprecision(6);
	output << "Audio Modules Smoke\n";
	output << "input_path=" << options.input_path.string() << "\n";
	output << "sample_rate=" << sample_rate << "\n";
	output << "decoded_frames=" << decoded_mono.size() << "\n";
	output << "smoke_frames=" << smoke_input.size() << "\n";
	output << "input_peak=" << input_stats.peak << "\n";
	output << "input_rms=" << input_stats.rms << "\n";
	output << "midi_event_count=" << midi_events.size() << "\n";
	output << "midi_dispatched_events=" << dispatched_events << "\n";
	output << "midi_looped_events=" << looped_events.size() << "\n";
	output << "midi_clock_pulses=" << clock_pulses << "\n";
	output << "midi_sysex_manufacturer_id=" << sysex_message.manufacturer_id << "\n";
	output << "mapped_param_id=" << mapped_param_id << "\n";
	output << "mapped_gain_db=" << mapped_gain_db << "\n";
	output << "vst3_plugin=" << vst3_host.GetLoadedPluginId() << "\n";
	output << "vst3_peak=" << vst3_stats.peak << "\n";
	output << "vst3_rms=" << vst3_stats.rms << "\n";
	output << "clap_plugin=" << clap_host.GetLoadedPluginId() << "\n";
	output << "clap_mode=" << (use_external_clap ? "external" : "builtin") << "\n";
	output << "clap_peak=" << clap_stats.peak << "\n";
	output << "clap_rms=" << clap_stats.rms << "\n";
	output << "au_plugin=" << au_host.GetLoadedComponentId() << "\n";
	output << "au_peak=" << au_stats.peak << "\n";
	output << "au_rms=" << au_stats.rms << "\n";
	output << "wav_float_roundtrip_frames=" << wav_float_roundtrip.size() << "\n";
	output << "aac_encoded_bytes=" << aac_bytes.size() << "\n";
	output << "aac_roundtrip_frames=" << aac_roundtrip.size() << "\n";
	output << "ogg_encoded_bytes=" << ogg_bytes.size() << "\n";
	output << "ogg_roundtrip_frames=" << ogg_roundtrip.size() << "\n";
	output << "id3_title=" << parsed_tag.title << "\n";
	output << "id3_artist=" << parsed_tag.artist << "\n";
	output << "id3_album=" << parsed_tag.album << "\n";
	output << "smoke_status=pass\n";

	if (!options.output_dir.empty()) {
		output << "output_dir=" << options.output_dir.string() << "\n";
	}

	*report = output.str();
	return true;
}

}  // namespace Engine::Audio::Demo