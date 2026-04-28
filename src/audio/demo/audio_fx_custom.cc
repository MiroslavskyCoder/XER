#include "audio/demo/audio_fx_custom.h"

#include "audio/audio_core/audio_source_loader.h"
#include "audio/effects_rack/custom_effect_struct.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"
#include "audio/fx_customs/fx_customs_presets.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace Engine::Audio::Demo {

namespace {

std::string NormalizeName(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	text.erase(std::remove(text.begin(), text.end(), ' '), text.end());
	return text;
}

std::string SlugifyName(std::string text) {
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
		if (std::isalnum(ch)) {
			return static_cast<char>(std::tolower(ch));
		}
		return static_cast<char>('_');
	});
	text.erase(std::unique(text.begin(), text.end(), [](char lhs, char rhs) {
		return lhs == '_' && rhs == '_';
	}), text.end());
	while (!text.empty() && text.front() == '_') {
		text.erase(text.begin());
	}
	while (!text.empty() && text.back() == '_') {
		text.pop_back();
	}
	return text.empty() ? std::string("fx_custom") : text;
}

bool WantsStereoByDefault(const std::string& effect_name) {
	const std::string normalized = NormalizeName(effect_name);
	return normalized == "stereoconverter"
		|| normalized == "superreverb"
		|| normalized == "roomreverb"
		|| normalized == "studioreverb"
		|| normalized == "eqreverb"
		|| normalized == "delayreverb"
		|| normalized == "delay"
		|| normalized == "airvoice";
}

bool WriteBinaryFile(
	const std::filesystem::path& path,
	const std::vector<std::uint8_t>& bytes,
	std::string* error_out) {
	std::ofstream output(path, std::ios::binary);
	if (!output.is_open()) {
		if (error_out != nullptr) {
			*error_out = "failed to open binary output: " + path.string();
		}
		return false;
	}
	output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
	if (!output) {
		if (error_out != nullptr) {
			*error_out = "failed to write binary output: " + path.string();
		}
		return false;
	}
	return true;
}

bool WriteTextFile(
	const std::filesystem::path& path,
	const std::string& text,
	std::string* error_out) {
	std::ofstream output(path);
	if (!output.is_open()) {
		if (error_out != nullptr) {
			*error_out = "failed to open text output: " + path.string();
		}
		return false;
	}
	output << text;
	if (!output) {
		if (error_out != nullptr) {
			*error_out = "failed to write text output: " + path.string();
		}
		return false;
	}
	return true;
}

bool WriteWaveFile(
	const std::filesystem::path& path,
	const std::vector<float>& samples,
	int sample_rate,
	int channels,
	std::string* error_out) {
	Engine::Audio::CodecIO::WavPcmCodec codec;
	std::vector<std::uint8_t> encoded;
	const size_t frame_count = channels > 0 ? samples.size() / static_cast<size_t>(channels) : 0u;
	if (!codec.Encode16(samples.data(), frame_count, encoded, sample_rate, channels)) {
		if (error_out != nullptr) {
			*error_out = "failed to encode custom fx wave output";
		}
		return false;
	}
	return WriteBinaryFile(path, encoded, error_out);
}

}  // namespace

bool RunAudioFxCustom(
	const AudioFxCustomOptions& options,
	std::string* report_out,
	std::string* error_out) {
	if (report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx custom report target is null";
		}
		return false;
	}
	if (options.input_path.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio fx custom requires an input audio path";
		}
		return false;
	}
	if (options.effect_name.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio fx custom requires an effect name";
		}
		return false;
	}
	if (!Engine::Audio::FX::Customs::IsFxCustomPresetSupported(options.effect_name)) {
		if (error_out != nullptr) {
			*error_out = "unsupported custom effect preset: " + options.effect_name;
		}
		return false;
	}

	std::error_code fs_error;
	std::filesystem::create_directories(options.output_dir, fs_error);
	if (fs_error) {
		if (error_out != nullptr) {
			*error_out = "failed to create audio fx custom output directory";
		}
		return false;
	}

	Engine::Audio::Core::AudioSourceLoadOptions load_options;
	load_options.input_path = options.input_path;
	load_options.raw_sample_rate = options.raw_sample_rate;
	load_options.target_sample_rate = options.target_sample_rate;
	load_options.target_channels = options.target_channels > 0
		? options.target_channels
		: (WantsStereoByDefault(options.effect_name) ? 2 : 0);

	Engine::Audio::Core::AudioSourceBuffer input_audio;
	if (!Engine::Audio::Core::AudioSourceLoader::Load(load_options, &input_audio, error_out)) {
		return false;
	}

	std::vector<float> processed_audio;
	Engine::Audio::FX::CustomEffectReport report;
	if (!Engine::Audio::FX::Customs::RenderFxCustomPresetInterleaved(
			options.effect_name,
			static_cast<float>(input_audio.sample_rate),
			input_audio.samples,
			input_audio.channels,
			&processed_audio,
			&report,
			"builtin://gain",
			error_out)) {
		return false;
	}

	const std::string effect_slug = SlugifyName(options.effect_name);
	const std::filesystem::path input_wav_path = options.output_dir / "normalized_input.wav";
	const std::filesystem::path output_wav_path = options.output_dir / (effect_slug + ".wav");
	const std::filesystem::path report_path = options.output_dir / (effect_slug + "_report.txt");
	if (!WriteWaveFile(input_wav_path, input_audio.samples, input_audio.sample_rate, input_audio.channels, error_out)
		|| !WriteWaveFile(output_wav_path, processed_audio, input_audio.sample_rate, input_audio.channels, error_out)) {
		return false;
	}

	std::ostringstream output;
	output << "Audio FX Custom CLI\n";
	output << "effect_name=" << options.effect_name << "\n";
	output << "input_path=" << options.input_path.string() << "\n";
	output << "source_format=" << input_audio.source_format << "\n";
	output << "codec=" << input_audio.codec_name << "\n";
	output << "decode_backend=" << input_audio.decode_backend << "\n";
	output << "original_sample_rate=" << input_audio.original_sample_rate << "\n";
	output << "original_channels=" << input_audio.original_channels << "\n";
	output << "sample_rate=" << input_audio.sample_rate << "\n";
	output << "render_channels=" << input_audio.channels << "\n";
	output << "frame_count=" << input_audio.frame_count << "\n";
	output << "normalized_input_wav=" << input_wav_path.string() << "\n";
	output << "processed_wav=" << output_wav_path.string() << "\n";
	output << Engine::Audio::FX::BuildCustomEffectReportText(report);

	const std::string report_text = output.str();
	if (!WriteTextFile(report_path, report_text, error_out)) {
		return false;
	}
	*report_out = report_text;
	return true;
}

}  // namespace Engine::Audio::Demo