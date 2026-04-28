#include "audio/demo/audio_fx_custom.h"

#include "audio/audio_core/audio_source_loader.h"
#include "audio/effects_rack/custom_effect_struct.h"
#include "audio/file_io_codecs/codec_wav_pcm.h"
#include "audio/fx_customs/fx_customs_presets.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
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

struct EffectArtifact {
	std::string effect_name;
	std::string effect_slug;
	std::filesystem::path processed_wav_path;
	std::filesystem::path report_path;
	Engine::Audio::FX::CustomEffectReport report;
	std::string report_text;
};

bool ContainsStereoPreferredEffect(const std::vector<std::string>& effect_names) {
	return std::any_of(effect_names.begin(), effect_names.end(), [](const std::string& effect_name) {
		return WantsStereoByDefault(effect_name);
	});
}

bool ResolveEffectList(
	const AudioFxCustomOptions& options,
	std::vector<std::string>* effect_names_out,
	std::string* error_out) {
	if (effect_names_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx effect list target is null";
		}
		return false;
	}
	*effect_names_out = options.effect_names;
	if (effect_names_out->empty() && !options.effect_name.empty()) {
		effect_names_out->push_back(options.effect_name);
	}
	if (effect_names_out->empty()) {
		if (error_out != nullptr) {
			*error_out = "audio fx command requires at least one effect name";
		}
		return false;
	}
	for (const auto& effect_name : *effect_names_out) {
		if (!Engine::Audio::FX::Customs::IsFxCustomPresetSupported(effect_name)) {
			if (error_out != nullptr) {
				*error_out = "unsupported custom effect preset: " + effect_name;
			}
			return false;
		}
	}
	return true;
}

bool LoadInputAudio(
	const AudioFxCustomOptions& options,
	const std::vector<std::string>& effect_names,
	Engine::Audio::Core::AudioSourceBuffer* input_audio_out,
	std::string* error_out) {
	if (input_audio_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx input buffer target is null";
		}
		return false;
	}

	Engine::Audio::Core::AudioSourceLoadOptions load_options;
	load_options.input_path = options.input_path;
	load_options.raw_sample_rate = options.raw_sample_rate;
	load_options.target_sample_rate = options.target_sample_rate;
	load_options.target_channels = options.target_channels > 0
		? options.target_channels
		: (ContainsStereoPreferredEffect(effect_names) ? 2 : 0);
	return Engine::Audio::Core::AudioSourceLoader::Load(load_options, input_audio_out, error_out);
}

std::string BuildEffectReportText(
	const std::string& effect_name,
	const Engine::Audio::Core::AudioSourceBuffer& input_audio,
	const std::filesystem::path& input_wav_path,
	const std::filesystem::path& output_wav_path,
	const Engine::Audio::FX::CustomEffectReport& report) {
	std::ostringstream output;
	output << "Audio FX Custom CLI\n";
	output << "effect_name=" << effect_name << "\n";
	output << "input_path=" << input_audio.decode_backend.empty() ? std::string() : input_audio.decode_backend << "\n";
	output.seekp(0, std::ios::end);
	return output.str();
}

std::string BuildEffectArtifactText(
	const std::string& effect_name,
	const std::filesystem::path& input_path,
	const Engine::Audio::Core::AudioSourceBuffer& input_audio,
	const std::filesystem::path& input_wav_path,
	const std::filesystem::path& output_wav_path,
	const Engine::Audio::FX::CustomEffectReport& report) {
	std::ostringstream output;
	output << "Audio FX Custom CLI\n";
	output << "effect_name=" << effect_name << "\n";
	output << "input_path=" << input_path.string() << "\n";
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
	return output.str();
}

bool RenderEffectArtifact(
	const AudioFxCustomOptions& options,
	const Engine::Audio::Core::AudioSourceBuffer& input_audio,
	const std::filesystem::path& input_wav_path,
	const std::string& effect_name,
	EffectArtifact* artifact_out,
	std::string* error_out) {
	if (artifact_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx artifact target is null";
		}
		return false;
	}

	std::vector<float> processed_audio;
	artifact_out->effect_name = effect_name;
	artifact_out->effect_slug = SlugifyName(effect_name);
	artifact_out->processed_wav_path = options.output_dir / (artifact_out->effect_slug + ".wav");
	artifact_out->report_path = options.output_dir / (artifact_out->effect_slug + "_report.txt");
	if (!Engine::Audio::FX::Customs::RenderFxCustomPresetInterleaved(
			effect_name,
			static_cast<float>(input_audio.sample_rate),
			input_audio.samples,
			input_audio.channels,
			&processed_audio,
			&artifact_out->report,
			"builtin://gain",
			error_out)) {
		return false;
	}
	if (!WriteWaveFile(artifact_out->processed_wav_path, processed_audio, input_audio.sample_rate, input_audio.channels, error_out)) {
		return false;
	}
	artifact_out->report_text = BuildEffectArtifactText(
		artifact_out->effect_name,
		options.input_path,
		input_audio,
		input_wav_path,
		artifact_out->processed_wav_path,
		artifact_out->report);
	return WriteTextFile(artifact_out->report_path, artifact_out->report_text, error_out);
}

std::string BuildBatchSummaryText(
	const AudioFxCustomOptions& options,
	const Engine::Audio::Core::AudioSourceBuffer& input_audio,
	const std::filesystem::path& input_wav_path,
	const std::filesystem::path& summary_path,
	const std::vector<EffectArtifact>& artifacts) {
	std::ostringstream output;
	output << std::fixed << std::setprecision(6);
	output << "Audio FX Batch CLI\n";
	output << "input_path=" << options.input_path.string() << "\n";
	output << "source_format=" << input_audio.source_format << "\n";
	output << "codec=" << input_audio.codec_name << "\n";
	output << "decode_backend=" << input_audio.decode_backend << "\n";
	output << "sample_rate=" << input_audio.sample_rate << "\n";
	output << "render_channels=" << input_audio.channels << "\n";
	output << "frame_count=" << input_audio.frame_count << "\n";
	output << "effect_count=" << artifacts.size() << "\n";
	output << "normalized_input_wav=" << input_wav_path.string() << "\n";
	output << "summary_path=" << summary_path.string() << "\n";
	for (size_t index = 0; index < artifacts.size(); ++index) {
		const auto& artifact = artifacts[index];
		output << "effect_" << index << "_name=" << artifact.effect_name << "\n";
		output << "effect_" << index << "_wav=" << artifact.processed_wav_path.string() << "\n";
		output << "effect_" << index << "_report=" << artifact.report_path.string() << "\n";
		output << "effect_" << index << "_peak=" << artifact.report.peak << "\n";
		output << "effect_" << index << "_rms=" << artifact.report.rms << "\n";
		output << "effect_" << index << "_channels=" << artifact.report.channel_count << "\n";
		output << "effect_" << index << "_workers=" << artifact.report.worker_count_used << "\n";
	}
	return output.str();
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
	std::vector<std::string> effect_names;
	if (!ResolveEffectList(options, &effect_names, error_out)) {
		return false;
	}
	if (effect_names.size() != 1u) {
		if (error_out != nullptr) {
			*error_out = "audio fx custom expects exactly one effect name";
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

	Engine::Audio::Core::AudioSourceBuffer input_audio;
	if (!LoadInputAudio(options, effect_names, &input_audio, error_out)) {
		return false;
	}

	const std::filesystem::path input_wav_path = options.output_dir / "normalized_input.wav";
	if (!WriteWaveFile(input_wav_path, input_audio.samples, input_audio.sample_rate, input_audio.channels, error_out)) {
		return false;
	}
	EffectArtifact artifact;
	if (!RenderEffectArtifact(options, input_audio, input_wav_path, effect_names.front(), &artifact, error_out)) {
		return false;
	}
	*report_out = artifact.report_text;
	return true;
}

bool RunAudioFxBatch(
	const AudioFxCustomOptions& options,
	std::string* report_out,
	std::string* error_out) {
	if (report_out == nullptr) {
		if (error_out != nullptr) {
			*error_out = "audio fx batch report target is null";
		}
		return false;
	}
	if (options.input_path.empty()) {
		if (error_out != nullptr) {
			*error_out = "audio fx batch requires an input audio path";
		}
		return false;
	}

	std::vector<std::string> effect_names;
	if (!ResolveEffectList(options, &effect_names, error_out)) {
		return false;
	}
	if (effect_names.size() < 2u) {
		if (error_out != nullptr) {
			*error_out = "audio fx batch expects at least two effects";
		}
		return false;
	}

	std::error_code fs_error;
	std::filesystem::create_directories(options.output_dir, fs_error);
	if (fs_error) {
		if (error_out != nullptr) {
			*error_out = "failed to create audio fx batch output directory";
		}
		return false;
	}

	Engine::Audio::Core::AudioSourceBuffer input_audio;
	if (!LoadInputAudio(options, effect_names, &input_audio, error_out)) {
		return false;
	}

	const std::filesystem::path input_wav_path = options.output_dir / "normalized_input.wav";
	if (!WriteWaveFile(input_wav_path, input_audio.samples, input_audio.sample_rate, input_audio.channels, error_out)) {
		return false;
	}

	std::vector<EffectArtifact> artifacts;
	artifacts.reserve(effect_names.size());
	for (const auto& effect_name : effect_names) {
		EffectArtifact artifact;
		if (!RenderEffectArtifact(options, input_audio, input_wav_path, effect_name, &artifact, error_out)) {
			return false;
		}
		artifacts.push_back(std::move(artifact));
	}

	const std::filesystem::path summary_path = options.output_dir / "batch_summary.txt";
	const std::string summary_text = BuildBatchSummaryText(options, input_audio, input_wav_path, summary_path, artifacts);
	if (!WriteTextFile(summary_path, summary_text, error_out)) {
		return false;
	}
	*report_out = summary_text;
	return true;
}

}  // namespace Engine::Audio::Demo