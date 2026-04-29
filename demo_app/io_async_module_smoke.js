ImportModule("IO/Async");
ImportModule("FileSystem");
ImportModule("System");

function ensure(condition, message) {
	if (!condition) {
		throw new Error(message);
	}
}

function expectThrows(action, message) {
	let didThrow = false;
	let errorText = "";
	try {
		action();
	} catch (error) {
		didThrow = true;
		errorText = String(error);
	}
	ensure(didThrow, message + ": expected throw");
	ensure(errorText.length > 0, message + ": error text should not be empty");
	return errorText;
}

const outDir = System.projectRoot() + "/out/io_async_module_smoke";
const nestedDir = outDir + "/nested";
const sourcePath = nestedDir + "/payload.txt";
const copyPath = outDir + "/payload_copy.txt";
const missingPath = outDir + "/missing.txt";

ensure(FileSystem.createDirectories(outDir), "failed to create io_async smoke directory");
ensure(IOAsync.writeText(sourcePath, "alpha", false), "IOAsync.writeText initial write failed");
ensure(IOAsync.writeText(sourcePath, "-beta", true), "IOAsync.writeText append failed");

const text = IOAsync.readText(sourcePath);
ensure(text === "alpha-beta", "IOAsync.readText content mismatch");
ensure(IOAsync.readHex(sourcePath) === "616c7068612d62657461", "IOAsync.readHex content mismatch");
ensure(IOAsync.fileSize(sourcePath) === 10, "IOAsync.fileSize mismatch after append");

ensure(IOAsync.copyBinary(sourcePath, copyPath), "IOAsync.copyBinary failed");
ensure(IOAsync.readText(copyPath) === text, "IOAsync.copyBinary content mismatch");
ensure(IOAsync.fileSize(copyPath) === 10, "IOAsync.fileSize mismatch for copied file");

expectThrows(function() {
	IOAsync.readText(missingPath);
}, "IOAsync.readText missing file");
expectThrows(function() {
	IOAsync.readHex(missingPath);
}, "IOAsync.readHex missing file");
expectThrows(function() {
	IOAsync.fileSize(missingPath);
}, "IOAsync.fileSize missing file");
expectThrows(function() {
	IOAsync.copyBinary(missingPath, outDir + "/missing_copy.txt");
}, "IOAsync.copyBinary missing source");