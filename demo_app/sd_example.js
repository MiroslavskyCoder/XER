/**
 * demo_app/sd_example.js
 *
 * Stable Diffusion (sd_base) JS example for XER.
 * Covers:
 *   - SD.info()
 *   - SD.generate(options)
 *   - SD.generateImage(options) -> RGBA object
 *   - SD.createSession()
 *   - new SD.SDSession()
 *   - session.attachPacket(), getPacket(), generate(), generateImage(), lastResult()
 *   - Image.save(path, image)
 */

ImportModule("SD");
ImportModule("Image");

const result = SD.generateImage({
  prompt: "cinematic shot, ultra detail",
  negativePrompt: "blurry, artifacts",
  width: 1024,
  height: 1024,
  steps: 30,
  guidanceScale: 7.5,
  seed: 42,

  enableSdxl: true,
  enableControlNet: true,
  enableVae: true,
  controlNetStrength: 0.07,
  depthStrength: 0.12,
  controlHint: "canny",
  backend: "cuda/cudnn" // или "cuda/cutlass", "openvino", "onnx", "xnnpack", "tensorflow", "eigen"
});

console.log("SD generation result:", {
  ok: result.ok,
  error: result.error,
  metadata: result.metadata, 
});

if (result.ok) {
  const saved = Image.save("sd_output.png", result.image);
  console.log("Image save status:", saved);
  if (!saved) {
    throw new Error("Failed to save sd_output.png");
  }
  console.log("Image saved to sd_output.png");
}