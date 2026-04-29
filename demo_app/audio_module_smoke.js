ImportModule("Audio");
ImportModule("FileSystem");
ImportModule("System");

function ensure(condition, message) {
	if (!condition) {
		throw new Error(message);
	}
}

function sortedKeys(value) {
	return Object.keys(value).sort().join(",");
}

function ensureShape(value, expectedKeys, message) {
	const expected = expectedKeys.slice().sort().join(",");
	const actual = sortedKeys(value);
	ensure(actual === expected, message + ": expected [" + expected + "], got [" + actual + "]");
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

const stats = Audio.stats(decoded);
ensure(stats && stats.peak > 0.2, "stats peak is too small");
ensure(stats.rms > 0.05, "stats rms is too small");

const effects = Audio.availableEffects();
ensure(Array.isArray(effects) && effects.includes("Compressor"), "availableEffects missing Compressor");
ensure(Audio.hasEffect("Compressor"), "hasEffect did not recognize Compressor");

const effectResult = Audio.applyEffect("Compressor", decoded);
ensure(effectResult && effectResult.audio && effectResult.report, "applyEffect did not return structured result");
ensureShape(effectResult, ["audio", "effectName", "report", "reportText"], "applyEffect result shape mismatch");
ensure(effectResult.audio.frameCount === frames, "applyEffect changed frame count unexpectedly");
ensure(effectResult.report.nodeCount >= 1, "applyEffect report is missing nodes");

const parallelBatchDir = outDir + "/batch_parallel";
ensure(FileSystem.createDirectories(parallelBatchDir), "failed to create parallel batch dir");
const parallelBatch = Audio.applyBatch(["Compressor", "Limiter"], decoded, {
	batchMode: "parallel",
	outputDir: parallelBatchDir
});
ensure(parallelBatch && parallelBatch.batchMode === "parallel", "applyBatch parallel did not return batch mode");
ensureShape(
	parallelBatch,
	["audio", "batchMode", "effectNames", "finalOutputWav", "inputAudio", "normalizedInputWav", "outputDir", "stageCount", "stages"],
	"applyBatch parallel result shape mismatch");
ensure(parallelBatch.stageCount === 2, "applyBatch parallel stageCount mismatch");
ensure(parallelBatch.stages.length === 2, "applyBatch parallel did not return both stages");
ensureShape(
	parallelBatch.stages[0],
	["audio", "effectName", "effectSlug", "frameCount", "processedWav", "renderMode", "report", "reportPath", "reportText", "stageIndex", "stageInputDurationSeconds", "stageInputFrameCount", "stageInputWav"],
	"applyBatch stage result shape mismatch");
ensure(FileSystem.exists(parallelBatch.normalizedInputWav), "applyBatch parallel did not write normalized input wav");
ensure(FileSystem.exists(parallelBatch.stages[0].processedWav), "applyBatch parallel did not write first stage wav");
ensure(FileSystem.exists(parallelBatch.stages[0].reportPath), "applyBatch parallel did not write first stage report");
ensure(parallelBatch.stages[1].stageInputWav === parallelBatch.normalizedInputWav, "parallel batch stage input should stay on normalized input");
ensure(parallelBatch.audio.frameCount === frames, "applyBatch parallel changed final frame count unexpectedly");

const chainResult = Audio.applyChain(["Compressor", "Limiter"], decoded);
ensure(chainResult && chainResult.audio && chainResult.stages, "applyChain did not return structured result");
ensureShape(chainResult, ["audio", "effectNames", "stageCount", "stages"], "applyChain result shape mismatch");
ensure(chainResult.stageCount === 2, "applyChain stageCount mismatch");
ensure(chainResult.stages.length === 2, "applyChain did not return both stages");
ensureShape(chainResult.stages[0], ["audio", "effectName", "report", "reportText"], "applyChain stage result shape mismatch");
ensure(chainResult.audio.frameCount === frames, "applyChain changed frame count unexpectedly");

const chainBatchDir = outDir + "/batch_chain";
ensure(FileSystem.createDirectories(chainBatchDir), "failed to create chain batch dir");
const chainBatch = Audio.applyBatch(["Compressor", "Limiter"], decoded, {
	batchMode: "chain",
	outputDir: chainBatchDir
});
ensure(chainBatch && chainBatch.batchMode === "chain", "applyBatch chain did not return batch mode");
ensureShape(
	chainBatch,
	["audio", "batchMode", "effectNames", "finalOutputWav", "inputAudio", "normalizedInputWav", "outputDir", "stageCount", "stages"],
	"applyBatch chain result shape mismatch");
ensure(chainBatch.stageCount === 2, "applyBatch chain stageCount mismatch");
ensureShape(
	chainBatch.stages[1],
	["audio", "effectName", "effectSlug", "frameCount", "processedWav", "renderMode", "report", "reportPath", "reportText", "stageIndex", "stageInputDurationSeconds", "stageInputFrameCount", "stageInputWav"],
	"applyBatch chain stage result shape mismatch");
ensure(chainBatch.stages[1].stageInputWav === chainBatch.stages[0].processedWav, "applyBatch chain did not chain stage input artifact");
ensure(chainBatch.finalOutputWav === chainBatch.stages[1].processedWav, "applyBatch chain final output wav mismatch");
ensure(chainBatch.audio.frameCount === frames, "applyBatch chain changed final frame count unexpectedly");

const aliasResult = Audio.save(aliasPath, decoded);
ensure(aliasResult && aliasResult.frameCount === frames, "save alias returned invalid frame count");
ensure(FileSystem.exists(aliasPath), "save alias did not create output file");
