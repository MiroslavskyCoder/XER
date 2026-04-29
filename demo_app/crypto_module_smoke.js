ImportModule("Crypto");
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

const outDir = System.projectRoot() + "/out/crypto_module_smoke";
ensure(FileSystem.createDirectories(outDir), "failed to create crypto smoke output directory");

const sampleText = "hello crypto\n";
const samplePath = outDir + "/payload.txt";
ensure(FileSystem.writeText(samplePath, sampleText), "failed to write crypto smoke payload");

ensure(
	Crypto.sha256("abc") === "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
	"Crypto.sha256 returned unexpected digest for abc");
ensure(Crypto.sha256File(samplePath) === Crypto.sha256(sampleText), "Crypto.sha256File did not match string digest");

const randomHex = Crypto.randomHex(16);
ensure(typeof randomHex === "string" && randomHex.length === 32, "Crypto.randomHex returned invalid hex length");
ensure(/^[0-9a-f]+$/.test(randomHex), "Crypto.randomHex returned non-hex characters");
ensure(Crypto.randomHex(0) === "", "Crypto.randomHex(0) should return empty string");

const encoded = Crypto.base64Encode("hello world");
ensure(encoded === "aGVsbG8gd29ybGQ=", "Crypto.base64Encode returned unexpected payload");
ensure(Crypto.base64Decode(encoded) === "hello world", "Crypto.base64Decode did not round-trip encoded text");
ensure(Crypto.base64Encode("") === "", "Crypto.base64Encode empty string mismatch");
ensure(Crypto.base64Decode("") === "", "Crypto.base64Decode empty string mismatch");

expectThrows(function() {
	Crypto.base64Decode("%%%invalid%%%");
}, "Crypto.base64Decode invalid input");
expectThrows(function() {
	Crypto.sha256File(outDir + "/missing.txt");
}, "Crypto.sha256File missing file");