ImportModule("Audio");
ImportModule("FileSystem");
ImportModule("System");

function ensure(condition, message) {
	if (!condition) {
		throw new Error(message);
	}
}

const outDir = System.projectRoot() + "/out/audio_module_smoke";
ensure(FileSystem.createDirectories(outDir), "failed to create audio smoke directory");

const frames = 128;
const samples = [];
for (let index = 0; index < frames; ++index) {
	const phase = (index / frames) * Math.PI * 4.0;
	samples.push(Math.sin(phase) * 0.25);
}

const input = {
	samples,
	sampleRate: 8000,
	channels: 1
};

const wavPath = outDir + "/roundtrip.wav";
const aliasPath = outDir + "/roundtrip_alias.wav";

const saveResult = Audio.saveWav(wavPath, input);
ensure(saveResult && saveResult.frameCount === frames, "saveWav returned invalid frame count");
ensure(FileSystem.exists(wavPath), "saveWav did not create output file");

const inspect = Audio.inspect(wavPath, {
	targetSampleRate: 8000,
	targetChannels: 1
});
ensure(inspect && inspect.source && inspect.normalized, "inspect did not return metadata");
ensure(inspect.source.channels === 1, "inspect source channel count mismatch");
ensure(inspect.normalized.sampleRate === 8000, "inspect normalized sample rate mismatch");
ensure(inspect.normalized.frames === frames, "inspect normalized frame count mismatch");

const loaded = Audio.load(wavPath, {
	targetSampleRate: 8000,
	targetChannels: 1
});
ensure(loaded && loaded.channels === 1, "load returned invalid channel count");
ensure(loaded.sampleRate === 8000, "load returned invalid sample rate");
ensure(loaded.frameCount === frames, "load returned invalid frame count");
ensure(loaded.samples.length === frames, "load returned invalid sample buffer length");
ensure(Math.abs(loaded.samples[16] - samples[16]) < 0.05, "load did not preserve waveform shape");

const encoded = Audio.encodeWav(loaded);
ensure(encoded && encoded.length > 44, "encodeWav returned invalid wav bytes");

const decoded = Audio.decodeWav(encoded);
ensure(decoded && decoded.sampleRate === 8000, "decodeWav returned invalid sample rate");
ensure(decoded.channels === 1, "decodeWav returned invalid channel count");
ensure(decoded.frameCount === frames, "decodeWav returned invalid frame count");
ensure(Math.abs(decoded.samples[16] - samples[16]) < 0.05, "decodeWav did not preserve waveform shape");

const aliasResult = Audio.save(aliasPath, decoded);
ensure(aliasResult && aliasResult.frameCount === frames, "save alias returned invalid frame count");
ensure(FileSystem.exists(aliasPath), "save alias did not create output file");
