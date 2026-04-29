ImportModule("Git");
ImportModule("System");

function ensure(condition, message) {
	if (!condition) {
		throw new Error(message);
	}
}

function sortedKeys(value) {
	return Object.keys(value).sort().join(",");
}

function ensureShape(value, expectedKeys, message) {
	const expected = expectedKeys.slice().sort().join(",");
	const actual = sortedKeys(value);
	ensure(actual === expected, message + ": expected [" + expected + "], got [" + actual + "]");
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

const repoPath = System.projectRoot();
const nonRepoPath = "/tmp";

ensure(Git.isRepository(repoPath), "Git.isRepository did not detect workspace repo");
ensure(!Git.isRepository(nonRepoPath), "Git.isRepository incorrectly accepted /tmp");

const describe = Git.describe(repoPath);
ensure(describe && describe.isRepository === true, "Git.describe did not report repository");
ensureShape(
	describe,
	["branch", "dirty", "error", "head", "headShort", "isRepository", "repoPath", "statusEntries", "statusEntryCount", "statusPorcelain"],
	"Git.describe result shape mismatch");
ensure(typeof describe.branch === "string", "Git.describe branch is not a string");
ensure(typeof describe.head === "string" && describe.head.length >= 7, "Git.describe head is invalid");
ensure(describe.headShort === describe.head.slice(0, 7), "Git.describe headShort mismatch");
ensure(Array.isArray(describe.statusEntries), "Git.describe statusEntries is not an array");
ensure(describe.statusEntryCount === describe.statusEntries.length, "Git.describe statusEntryCount mismatch");
ensure(typeof describe.statusPorcelain === "string", "Git.describe statusPorcelain is not a string");
ensure(typeof describe.dirty === "boolean", "Git.describe dirty is not a boolean");
ensure(describe.error === "", "Git.describe should not report error for repo path");
if (describe.statusEntries.length > 0) {
	ensureShape(
		describe.statusEntries[0],
		["indexStatus", "oldPath", "path", "raw", "workTreeStatus"],
		"Git.describe status entry shape mismatch");
}

ensure(Git.currentBranch(repoPath) === describe.branch, "Git.currentBranch mismatch against describe.branch");
ensure(Git.head(repoPath) === describe.head, "Git.head mismatch against describe.head");

const status = Git.status(repoPath);
ensureShape(status, ["dirty", "entries", "entryCount", "repoPath", "statusPorcelain"], "Git.status result shape mismatch");
ensure(status.repoPath === repoPath, "Git.status repoPath mismatch");
ensure(status.entryCount === status.entries.length, "Git.status entryCount mismatch");
ensure(status.statusPorcelain === Git.statusPorcelain(repoPath), "Git.statusPorcelain mismatch against Git.status");
if (status.entries.length > 0) {
	ensureShape(status.entries[0], ["indexStatus", "oldPath", "path", "raw", "workTreeStatus"], "Git.status entry shape mismatch");
}

const nonRepoDescribe = Git.describe(nonRepoPath);
ensure(nonRepoDescribe && nonRepoDescribe.isRepository === false, "Git.describe non-repo path should report false");
ensureShape(
	nonRepoDescribe,
	["branch", "dirty", "error", "head", "headShort", "isRepository", "repoPath", "statusEntries", "statusEntryCount", "statusPorcelain"],
	"Git.describe non-repo result shape mismatch");
ensure(nonRepoDescribe.branch === "", "Git.describe non-repo branch should be empty");
ensure(nonRepoDescribe.head === "", "Git.describe non-repo head should be empty");
ensure(nonRepoDescribe.statusPorcelain === "", "Git.describe non-repo statusPorcelain should be empty");
ensure(nonRepoDescribe.statusEntryCount === 0, "Git.describe non-repo statusEntryCount should be zero");
ensure(nonRepoDescribe.statusEntries.length === 0, "Git.describe non-repo statusEntries should be empty");
ensure(nonRepoDescribe.error.length > 0, "Git.describe non-repo should report error text");

expectThrows(function() {
	Git.head(nonRepoPath);
}, "Git.head non-repo path");
expectThrows(function() {
	Git.currentBranch(nonRepoPath);
}, "Git.currentBranch non-repo path");
expectThrows(function() {
	Git.statusPorcelain(nonRepoPath);
}, "Git.statusPorcelain non-repo path");
expectThrows(function() {
	Git.status(nonRepoPath);
}, "Git.status non-repo path");