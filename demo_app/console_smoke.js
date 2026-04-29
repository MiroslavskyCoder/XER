console.log("console-log", { alpha: 1, nested: [1, 2, 3] });
console.info("console-info", Buffer.from("4142", "hex"));
console.warn("console-warn", { warning: true, code: 17 });
console.error("console-error", ["x", "y"]);
console.dir({ typed: Buffer.from("0011", "hex"), plain: "text" });
console.assert(false, "console-assert", { failed: true });

console.log(JSON.stringify({
	logOk: typeof console.log === "function",
	infoOk: typeof console.info === "function",
	warnOk: typeof console.warn === "function",
	errorOk: typeof console.error === "function",
	dirOk: typeof console.dir === "function",
	assertOk: typeof console.assert === "function",
	consoleClassOk: typeof Console === "function"
}));