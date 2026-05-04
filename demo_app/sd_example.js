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

function printSection(title) {
    console.log("\n=== " + title + " ===");
}

function printResult(prefix, result) {
    if (!result || typeof result !== "object") {
        console.log(prefix + ": <no result object>");
        return;
    }

    console.log(prefix + " ok       :", result.ok);
    console.log(prefix + " error    :", result.error || "");
    console.log(prefix + " imageShape:", JSON.stringify(result.imageShape || []));
    console.log(prefix + " metadata :", JSON.stringify(result.metadata || {}));
}

function printImageResult(prefix, result) {
    if (!result || typeof result !== "object") {
        console.log(prefix + ": <no image result object>");
        return;
    }
    console.log(prefix + " ok        :", result.ok);
    console.log(prefix + " converted :", result.converted);
    console.log(prefix + " error     :", result.error || "");
    console.log(prefix + " size      :", (result.width || 0) + "x" + (result.height || 0));
    console.log(prefix + " data bytes:", Array.isArray(result.data) ? result.data.length : 0);
}

console.log("=== SD + Image example ===");
console.log("SD.info()    :", JSON.stringify(SD.info()));
console.log("Image.info() :", JSON.stringify(Image.info()));

const request = {
    prompt: "futuristic city, dusk, ultra detailed",
    negativePrompt: "blurry, noisy, artifacts",
    width: 256,
    height: 256,
    steps: 8,
    guidanceScale: 5.5,
    seed: 77
};

printSection("Top-level SD.generateImage() and Image.save()");
let imageResult = null;
try {
    imageResult = SD.generateImage(request);
    printImageResult("generateImage", imageResult);
    if (imageResult && imageResult.ok && imageResult.image) {
        const saved = Image.save("./demo_app/data/sd_generated_top.png", imageResult.image);
        console.log("saved top-level image:", saved);
    }
} catch (err) {
    console.log("generateImage threw:", String(err));
}

printSection("Session flow with packet fallback");
const session = SD.createSession();
session.attachPacket({
    prompt: "concept art, neon alley, detailed reflections"
});

let sessionResult = null;
try {
    sessionResult = session.generate({
        width: 320,
        height: 320,
        steps: 10,
        guidanceScale: 6.0,
        seed: 2026
    });
    printResult("session.generate", sessionResult);
} catch (err) {
    console.log("session.generate threw:", String(err));
}

let sessionImageResult = null;
try {
    sessionImageResult = session.generateImage({
        width: 320,
        height: 320,
        steps: 10,
        guidanceScale: 6.0,
        seed: 2027
    });
    printImageResult("session.generateImage", sessionImageResult);
    if (sessionImageResult && sessionImageResult.ok && sessionImageResult.image) {
        const savedSession = Image.save("./demo_app/data/sd_generated_session.png", sessionImageResult.image);
        console.log("saved session image:", savedSession);
    }
} catch (err) {
    console.log("session.generateImage threw:", String(err));
}

try {
    const savedLast = session.saveLastImage("./demo_app/data/sd_generated_last.png");
    console.log("session.saveLastImage:", savedLast);
} catch (err) {
    console.log("session.saveLastImage threw:", String(err));
}

printSection("Constructed session path");
const session2 = new SD.SDSession();
session2.attachPacket({
    prompt: "studio portrait, dramatic light"
});
let session2Image = null;
try {
    session2Image = session2.generateImage({
        width: 192,
        height: 192,
        steps: 6,
        guidanceScale: 5.0,
        seed: 88
    });
    printImageResult("session2.generateImage", session2Image);
} catch (err) {
    console.log("session2.generateImage threw:", String(err));
}

console.log("\nSD + Image example complete");