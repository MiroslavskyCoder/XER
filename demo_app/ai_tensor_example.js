/**
 * demo_app/ai_tensor_example.js
 *
 * Demonstrates the full AI module JS API:
 *   - module info / runtime features
 *   - ML logging & profiler
 *   - Math utilities
 *   - Random number generation
 *   - Type conversion helpers
 *   - Config parsing
 *   - Tensor creation, validation, normalisation, augmentation
 *   - Feature math (dot, L2, add, subtract, scale, matmul, covariance)
 *   - Dataset loading & caching via handles
 *   - AIContext: per-session metadata, tensors, packet exchange
 *   - Memory profiler
 */

ImportModule("AI");

// ─── 1. Module info ───────────────────────────────────────────────────────────

console.log("=== AI module info ===");
const info = AI.info();
console.log("name            :", info.name);
console.log("version         :", info.version);
console.log("buildTag        :", info.buildTag);
console.log("suggestedThreads:", info.suggestedThreads);
console.log("runtimeBanner   :", info.runtimeBanner);

const ver = AI.version();
console.log("\nversion object  :", JSON.stringify(ver));

const features = AI.runtimeFeatures();
console.log("runtimeFeatures :", JSON.stringify(features));

console.log("suggestedThreads:", AI.suggestedThreads());
console.log("compressString  :", AI.compressString("hello world compression test"));

// ─── 2. ML logger ─────────────────────────────────────────────────────────────

console.log("\n=== ML logger ===");
AI.mlSetLogLevel("debug");
AI.mlLog("info",    "pipeline started");
AI.mlLog("debug",   "data batch size = 32");
AI.mlLog("warning", "gradient too small");
AI.mlLog("error",   "NaN detected in output");

const logs = AI.mlGetLogs();
console.log("log entries:", logs.length);
logs.forEach(e => console.log(" [" + e.level + "] " + e.message));

AI.mlClearLogs();
console.log("after clear:", AI.mlGetLogs().length, "entries");

// ─── 3. Reader logger ─────────────────────────────────────────────────────────

console.log("\n=== Reader logger ===");
AI.readerSetLogLevel("info");
AI.readerLog("info",  "loading ONNX model");
AI.readerLog("error", "unsupported opset 18");
const rLogs = AI.readerGetLogs();
console.log("reader log entries:", rLogs.length);
rLogs.forEach(e => console.log(" [" + e.level + "] " + e.message));
AI.readerClearLogs();

// ─── 4. Math utilities ────────────────────────────────────────────────────────

console.log("\n=== Math utils ===");
const vec = [1, 2, 3, 4, 5, 6];
console.log("dot([1,2,3],[4,5,6])  :", AI.mathDot([1, 2, 3], [4, 5, 6]));
console.log("mean(vec)             :", AI.mathMean(vec));
console.log("variance(vec)         :", AI.mathVariance(vec));

const raw = [0.5, 2.0, 0.1, 0.9];
console.log("normalize(raw)        :", AI.normalize(raw));
console.log("softmax([1,2,3])      :", AI.softmax([1, 2, 3]));
console.log("l2Normalize([3,4])    :", AI.l2Normalize([3, 4]));

// ─── 5. Random generator ──────────────────────────────────────────────────────

console.log("\n=== Random generator ===");
AI.setSeed(42);
console.log("randomFloat(0,1)      :", AI.randomFloat(0, 1));
console.log("randomInt(0,100)      :", AI.randomInt(0, 100));
console.log("bernoulli(0.7)        :", AI.bernoulli(0.7));

// ─── 6. Type converters ───────────────────────────────────────────────────────

console.log("\n=== Type converters ===");
const floats = [1.5, 2.75, -0.5];
const ints   = AI.floatToInt(floats);
console.log("floatToInt([1.5,2.75,-0.5]):", ints);
console.log("intToFloat(ints)           :", AI.intToFloat(ints));

const bytes = AI.floatToBytes([0.1, 0.2, 0.3, 0.4]);
console.log("floatToBytes (4 floats) -> bytes length:", bytes.length);
console.log("bytesToFloat              :", AI.bytesToFloat(bytes));

const fp16 = AI.fp16RoundTrip([1.0, 0.5, 0.125, 3.1416]);
console.log("fp16RoundTrip             :", fp16);

// ─── 7. Config parser ─────────────────────────────────────────────────────────

console.log("\n=== Config parser ===");
const cfg = AI.parseConfig(JSON.stringify({
    learning_rate: 0.001,
    batch_size: 32,
    epochs: 10,
    optimizer: "adam"
}));
console.log("parsed config:", JSON.stringify(cfg));

// ─── 8. Profiler ──────────────────────────────────────────────────────────────

console.log("\n=== Profiler ===");
AI.profilerBegin("training_loop");

// simulate some work
let sum = 0;
for (let i = 0; i < 1000; i++) sum += AI.randomFloat(0, 1);

AI.profilerEnd("training_loop");
const snap = AI.profilerSnapshot();
console.log("profiler snapshot:", JSON.stringify(snap));

// ─── 9. Tensors ───────────────────────────────────────────────────────────────

console.log("\n=== Tensors ===");

// float32 tensor [2, 3]
const t1 = AI.createTensor([2, 3], [0.1, 0.2, 0.3, 0.4, 0.5, 0.6], "float32");
console.log("createTensor [2,3]:", JSON.stringify(t1));

// zero tensor
const tz = AI.zeroTensor([4], "float32");
console.log("zeroTensor  [4]   :", JSON.stringify(tz));

// validation
const valid = AI.validateTensor([2, 3], [0.1, 0.2, 0.3, 0.4, 0.5, 0.6]);
console.log("validateTensor    :", JSON.stringify(valid));

// normalisation (mean=0, std=1)
const normed = AI.normalizeTensor([1, 4], [1.0, 2.0, 3.0, 4.0], "float32", 0.0, 1.0);
console.log("normalizeTensor   :", JSON.stringify(normed));

// augmentation modes
const augRotation = AI.augmentTensor("rotation", [1, 4], [0.1, 0.5, 0.2, 0.9], "float32", 15.0);
console.log("augmentTensor rotation:", JSON.stringify(augRotation));

const augFlip = AI.augmentTensor("flip", [1, 4], [0.1, 0.5, 0.2, 0.9], "float32", 0.5);
console.log("augmentTensor flip    :", JSON.stringify(augFlip));

const augCrop = AI.augmentTensor("crop", [1, 4], [0.1, 0.5, 0.2, 0.9], "float32", 0.8, 1.0);
console.log("augmentTensor crop    :", JSON.stringify(augCrop));

const augColor = AI.augmentTensor("colorjitter", [1, 4], [0.1, 0.5, 0.2, 0.9], "float32", 0.2, 0.2, 0.2);
console.log("augmentTensor color   :", JSON.stringify(augColor));

const augNoise = AI.augmentTensor("noise", [1, 4], [0.1, 0.5, 0.2, 0.9], "float32", 0.05);
console.log("augmentTensor noise   :", JSON.stringify(augNoise));

// ─── 10. Feature math ─────────────────────────────────────────────────────────

console.log("\n=== Feature math ===");
const a = [1.0, 2.0, 3.0];
const b = [4.0, 5.0, 6.0];

console.log("featureDotProduct :", AI.featureDotProduct(a, b));
console.log("featureL2Norm(a)  :", AI.featureL2Norm(a));
console.log("featureAdd        :", AI.featureAdd(a, b));
console.log("featureSubtract   :", AI.featureSubtract(b, a));
console.log("featureScale(a,2) :", AI.featureScale(a, 2.0));

// matmul: A[2x3] * B[3x2]
const matA = [1, 2, 3, 4, 5, 6];        // 2x3
const matB = [7, 8, 9, 10, 11, 12];     // 3x2
const matRes = AI.featureMatMul(matA, 2, 3, matB, 3, 2);
console.log("featureMatMul [2x3]*[3x2] -> [2x2]:", matRes);

// covariance of 3 samples with 4 features
const samples = [
    [1.0, 2.0, 3.0, 4.0],
    [2.0, 3.0, 4.0, 5.0],
    [3.0, 4.0, 5.0, 6.0]
].flat();
const cov = AI.featureCovariance(samples, 3, 4);
console.log("featureCovariance (3x4):", cov);

// ─── 11. Dataset loading & caching ───────────────────────────────────────────

console.log("\n=== Dataset loading & caching ===");

// loadDataset: the path may not exist in this demo — we check graceful handling
const dsResult = AI.loadDataset("./data/sample.csv", "csv", { delimiter: "," });
if (dsResult && dsResult.handle) {
    const handle = dsResult.handle;
    console.log("loaded dataset handle :", handle);
    console.log("  name              :", dsResult.name);
    console.log("  batchCount        :", dsResult.batchCount);
    console.log("  totalSamples      :", dsResult.totalSamples);
    console.log("  loaderType        :", dsResult.loaderType);

    // inspect via getDataset
    const got = AI.getDataset(handle);
    console.log("getDataset same name  :", got && got.name);

    // list all handles
    console.log("listDatasets          :", AI.listDatasets());

    // cache and retrieve (signature: key, handle)
    AI.cacheDataset("training_set", handle);
    console.log("isDatasetCached       :", AI.isDatasetCached("training_set"));
    console.log("getCachedDataset name :", AI.getCachedDataset("training_set")?.name);
    console.log("getDatasetCacheSize   :", AI.getDatasetCacheSize());

    AI.removeCachedDataset("training_set");
    console.log("after remove, cached  :", AI.isDatasetCached("training_set"));

    // save and drop (signature: path, handle)
    AI.saveDataset("./data/sample_out.csv", handle);
    AI.dropDataset(handle);
    console.log("listDatasets after drop:", AI.listDatasets());
} else {
    console.log("loadDataset: no file available (expected in demo) -> graceful null/false result");
}

// full cache-clear path
AI.clearDatasetCache();
console.log("cacheSize after clearDatasetCache:", AI.getDatasetCacheSize());

// ─── 12. Memory profiler ──────────────────────────────────────────────────────

console.log("\n=== Memory profiler ===");
const mem = AI.memoryCapture();
console.log("memoryCapture:", JSON.stringify(mem));
AI.memoryReset();
console.log("after memoryReset, next capture:", JSON.stringify(AI.memoryCapture()));

// ─── 13. AIContext ────────────────────────────────────────────────────────────

console.log("\n=== AIContext ===");
const ctx = AI.createContext();

// Metadata
ctx.setMeta("modelName",  "ResNet-50");
ctx.setMeta("inputShape", "3x224x224");
ctx.setMeta("precision",  "float32");
console.log("meta modelName  :", ctx.getMeta("modelName"));
console.log("meta inputShape :", ctx.getMeta("inputShape"));
console.log("metaKeys        :", ctx.metaKeys());

// Tensor storage
const inputData = new Array(3 * 224 * 224).fill(0).map((_, i) => (i % 256) / 255);
ctx.putTensor("input", [3, 224, 224], inputData, "float32");
ctx.createEmbedding("embed", [1, 128], new Array(128).fill(0).map(() => AI.randomFloat(-1, 1)));

const tInfo = ctx.getTensor("input");
console.log("getTensor 'input' shape :", tInfo && tInfo.shape);
console.log("listTensors             :", ctx.listTensors());

// Packet export/import
const packet = ctx.exportPacket();
console.log("exportPacket keys       :", packet && Object.keys(packet));

const ctx2 = AI.createContext();
ctx2.importPacket(packet);
console.log("importPacket meta keys  :", ctx2.metaKeys());

// Clear
ctx.clear();
console.log("after clear, listTensors:", ctx.listTensors());
console.log("after clear, metaKeys   :", ctx.metaKeys());

// ─── Done ─────────────────────────────────────────────────────────────────────

console.log("\n=== AI tensor example complete ===");
