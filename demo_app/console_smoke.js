console.log("console-log", { alpha: 1, nested: [1, 2, 3] });
console.info("console-info", Buffer.from("4142", "hex"));
console.warn("console-warn", { warning: true, code: 17 });
console.error("console-error", ["x", "y"]);
console.dir({ typed: Buffer.from("0011", "hex"), plain: "text" });
console.assert(false, "console-assert", { failed: true });

const tableRows = [];
for (let index = 0; index < 36; index += 1) {
	tableRows.push({
		name: "row-" + index,
		count: index,
		meta: { active: index % 2 === 0, label: "item-" + index },
		values: [index, index + 1, index + 2],
		alpha: index + 10,
		beta: index + 20,
		gamma: index + 30,
		delta: index + 40,
		epsilon: index + 50,
		zeta: index + 60,
		eta: index + 70,
		theta: index + 80,
		iota: index + 90,
		kappa: index + 100
	});
}

console.table(tableRows);
console.table([
	{ name: "alpha", count: 1, nested: { ok: true, tags: ["x", "y"] } },
	{ name: "beta", count: 2, nested: { ok: false, tags: ["z"] } }
], ["name", "count", "nested"]);

console.time("console-smoke-timer");
console.timeEnd("console-smoke-timer");

function traceSmoke() {
	console.trace("trace-smoke", { trace: true });
}

traceSmoke();

console.log(JSON.stringify({
	logOk: typeof console.log === "function",
	infoOk: typeof console.info === "function",
	warnOk: typeof console.warn === "function",
	errorOk: typeof console.error === "function",
	dirOk: typeof console.dir === "function",
	assertOk: typeof console.assert === "function",
	tableOk: typeof console.table === "function",
	timeOk: typeof console.time === "function",
	timeEndOk: typeof console.timeEnd === "function",
	traceOk: typeof console.trace === "function",
	consoleClassOk: typeof Console === "function"
}));