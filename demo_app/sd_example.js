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
  scheduler: "ddim", // "euler" | "ddim"
  guidanceScale: 7.5,
  seed: 42,

  enableSdxl: true,
  enableControlNet: true,
  enableVae: true,
  strictModelLoading: false,
  controlNetStrength: 0.07,
  depthStrength: 0.12,
  controlHint: "canny",
  backend: "cuda/cudnn", // или "cuda/cutlass", "openvino", "onnx", "xnnpack", "tensorflow", "eigen"
  modelWeights: {
    textEncoder: "models/sd_base/text_encoder.onnx",
    unet: "models/sd_base/unet.onnx",
    vaeDecoder: "models/sd_base/vae_decoder.onnx",
    controlNet: "models/sd_base/controlnet.onnx",
    sdxlTextEncoder2: "models/sd_base/sdxl_text_encoder_2.onnx",
    sdxlRefinerUnet: "models/sd_base/sdxl_refiner_unet.onnx"
  }
});

console.log("SD metadata:", result.metadata);

if (result.ok) {
  const saved = Image.save("sd_output.png", result.image);
  console.log("Image save status:", saved);
  if (!saved) {
    throw new Error("Failed to save sd_output.png");
  }
  console.log("Image saved to sd_output.png");
}