const utf = Buffer.from("hello");
const hex = Buffer.from("0011223344556677", "hex");
const alloc = Buffer.alloc(6, "ab");
const copied = Buffer.alloc(4);

utf.copy(copied, 0, 1, 4);

const slice = utf.slice(1, 4);
slice[0] = 0x61;

const swap16 = Buffer.from("00112233", "hex");
swap16.swap16();

const swap32 = Buffer.from("00112233", "hex");
swap32.swap32();

const swap64 = Buffer.from("0011223344556677", "hex");
swap64.swap64();

const filled = Buffer.alloc(5);
filled.fill("xy", 1, 5);

console.log(JSON.stringify({
	globalOk: typeof Buffer === "function",
	constructorOk: Buffer.isBuffer(utf),
	utfText: utf.toString(),
	copyText: copied.toString(),
	sliceShared: utf.toString() === "hallo",
	indexOfLl: utf.indexOf("ll"),
	lastIndexOfL: utf.lastIndexOf("l"),
	hexRoundtrip: hex.toString("hex"),
	allocPattern: alloc.toString(),
	filledPattern: filled.toString(),
	swap16: swap16.toString("hex"),
	swap32: swap32.toString("hex"),
	swap64: swap64.toString("hex"),
	byteLengthHex: Buffer.byteLength("0011", "hex"),
	concatText: Buffer.concat([Buffer.from("A"), Buffer.from("B")]).toString()
}));