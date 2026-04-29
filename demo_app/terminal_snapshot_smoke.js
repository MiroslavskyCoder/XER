console.clearSnapshot();

console.log("snapshot-stdout", { ok: true, side: "out" });
console.error("snapshot-stderr", { ok: false, side: "err" });

const stdoutSnapshot = console.snapshot("stdout");
const stderrSnapshot = console.snapshot("stderr");

const stdoutOk = stdoutSnapshot.indexOf("snapshot-stdout") !== -1 && stdoutSnapshot.indexOf("snapshot-stderr") === -1;
const stderrOk = stderrSnapshot.indexOf("snapshot-stderr") !== -1 && stderrSnapshot.indexOf("snapshot-stdout") === -1;

console.clearSnapshot();

const clearedStdoutOk = console.snapshot("stdout") === "";
const clearedStderrOk = console.snapshot("stderr") === "";

console.log(JSON.stringify({
	snapshotMethodOk: typeof console.snapshot === "function",
	clearSnapshotMethodOk: typeof console.clearSnapshot === "function",
	stdoutOk,
	stderrOk,
	clearedStdoutOk,
	clearedStderrOk
}));