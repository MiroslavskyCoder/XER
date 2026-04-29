ImportModule("Container");
ImportModule("System");
ImportModule("RuntimeLive");
ImportModule("Doctor");
ImportModule("Audio");
ImportModule("Git");
ImportModule("OpenCV");
ImportModule("CUDA");
ImportModule("CUDNN");
ImportModule("ANGLE");
ImportModule("VTK");
ImportModule("Skia");
ImportModule("FFmpeg");

console.log(JSON.stringify({
	containerOk: typeof Container === "object" && Container !== null,
	systemOk: typeof System === "object" && System !== null,
	runtimeLiveOk: typeof RuntimeLive === "object" && RuntimeLive !== null,
	doctorOk: typeof Doctor === "object" && Doctor !== null,
	audioOk: typeof Audio === "object" && Audio !== null,
	gitOk: typeof Git === "object" && Git !== null,
	openCvOk: typeof OpenCV === "object" && OpenCV !== null,
	cudaOk: typeof CUDA === "object" && CUDA !== null,
	cudnnOk: typeof CUDNN === "object" && CUDNN !== null,
	angleOk: typeof ANGLE === "object" && ANGLE !== null,
	vtkOk: typeof VTK === "object" && VTK !== null,
	skiaOk: typeof Skia === "object" && Skia !== null,
	ffmpegOk: typeof FFmpeg === "object" && FFmpeg !== null,
	hasAudioInspect: Audio && typeof Audio.inspect === "function",
	hasAudioEffects: Audio && typeof Audio.applyEffect === "function" && typeof Audio.availableEffects === "function",
	hasAudioBatch: Audio && typeof Audio.applyBatch === "function",
	hasGitDescribe: Git && typeof Git.describe === "function",
	hasGitStatus: Git && typeof Git.status === "function",
	hasCompileAndRun: RuntimeLive && typeof RuntimeLive.compileAndRun === "function",
	hasFfmpegProbe: FFmpeg && typeof FFmpeg.probeMedia === "function",
	hasSkiaExports: Skia && typeof Skia.exportedFunctions === "object"
}));