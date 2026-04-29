ImportModule("Git");
ImportModule("FileSystem");
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

function makeTempRepoPath() {
	return "/tmp/xer_git_smoke_" + String(Date.now()) + "_" + String(Math.floor(Math.random() * 1000000));
}

function findStatusEntry(entries, path) {
	for (let index = 0; index < entries.length; ++index) {
		if (entries[index].path === path) {
			return entries[index];
		}
	}
	return null;
}

function findRenameEntry(entries, oldPath, path) {
	for (let index = 0; index < entries.length; ++index) {
		if (entries[index].oldPath === oldPath && entries[index].path === path) {
			return entries[index];
		}
	}
	return null;
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

const tempRepoPath = makeTempRepoPath();
ensure(Git.init(tempRepoPath), "Git.init did not initialize temporary repo");
ensure(FileSystem.exists(tempRepoPath + "/.git"), "Git.init did not create .git directory");
ensure(Git.isRepository(tempRepoPath), "Git.init result is not recognized as a repository");
ensure(Git.setConfig(tempRepoPath, "user.name", "XER Smoke"), "Git.setConfig user.name failed");
ensure(Git.setConfig(tempRepoPath, "user.email", "xer-smoke@example.com"), "Git.setConfig user.email failed");

ensure(FileSystem.writeText(tempRepoPath + "/alpha.txt", "alpha\n"), "failed to write alpha.txt");
ensure(FileSystem.writeText(tempRepoPath + "/stable.txt", "stable\n"), "failed to write stable.txt");
ensure(Git.add(tempRepoPath, "alpha.txt"), "Git.add alpha.txt failed");
ensure(Git.add(tempRepoPath, "stable.txt"), "Git.add stable.txt failed");
ensure(Git.commit(tempRepoPath, "initial commit"), "Git.commit initial commit failed");

const tempHead = Git.head(tempRepoPath);
ensure(typeof tempHead === "string" && tempHead.length >= 7, "temporary repo HEAD is invalid after initial commit");
ensure(typeof Git.currentBranch(tempRepoPath) === "string" && Git.currentBranch(tempRepoPath).length > 0, "temporary repo branch is empty");

ensure(Git.move(tempRepoPath, "alpha.txt", "beta.txt"), "Git.move alpha.txt -> beta.txt failed");
ensure(FileSystem.writeText(tempRepoPath + "/beta.txt", "alpha renamed and modified\n"), "failed to modify beta.txt");
ensure(FileSystem.writeText(tempRepoPath + "/stable.txt", "stable modified\n"), "failed to modify stable.txt");
ensure(FileSystem.writeText(tempRepoPath + "/staged_only.txt", "staged only\n"), "failed to write staged_only.txt");
ensure(Git.add(tempRepoPath, "staged_only.txt"), "Git.add staged_only.txt failed");

const tempStatus = Git.status(tempRepoPath);
ensureShape(tempStatus, ["dirty", "entries", "entryCount", "repoPath", "statusPorcelain"], "Git.status temp repo result shape mismatch");
ensure(tempStatus.repoPath === tempRepoPath, "Git.status temp repoPath mismatch");
ensure(tempStatus.dirty === true, "Git.status temp repo should be dirty");
ensure(tempStatus.entryCount === 3, "Git.status temp repo should have exactly three entries");
ensure(tempStatus.entries.length === 3, "Git.status temp repo entries length mismatch");
ensure(tempStatus.statusPorcelain === Git.statusPorcelain(tempRepoPath), "Git.statusPorcelain temp repo mismatch");

const renameEntry = findRenameEntry(tempStatus.entries, "alpha.txt", "beta.txt");
ensure(renameEntry !== null, "Git.status temp repo is missing rename entry");
ensureShape(renameEntry, ["indexStatus", "oldPath", "path", "raw", "workTreeStatus"], "Git.status rename entry shape mismatch");
ensure(renameEntry.indexStatus === "R", "Git.status rename entry should be staged as rename");
ensure(renameEntry.workTreeStatus === "M", "Git.status rename entry should also carry unstaged modification");
ensure(renameEntry.raw.indexOf("alpha.txt -> beta.txt") >= 0, "Git.status rename raw entry mismatch");

const stagedOnlyEntry = findStatusEntry(tempStatus.entries, "staged_only.txt");
ensure(stagedOnlyEntry !== null, "Git.status temp repo is missing staged-only entry");
ensureShape(stagedOnlyEntry, ["indexStatus", "oldPath", "path", "raw", "workTreeStatus"], "Git.status staged-only entry shape mismatch");
ensure(stagedOnlyEntry.indexStatus === "A", "Git.status staged-only entry should have staged add flag");
ensure(stagedOnlyEntry.workTreeStatus === " ", "Git.status staged-only entry should not have unstaged flag");

const unstagedOnlyEntry = findStatusEntry(tempStatus.entries, "stable.txt");
ensure(unstagedOnlyEntry !== null, "Git.status temp repo is missing unstaged-only entry");
ensureShape(unstagedOnlyEntry, ["indexStatus", "oldPath", "path", "raw", "workTreeStatus"], "Git.status unstaged-only entry shape mismatch");
ensure(unstagedOnlyEntry.indexStatus === " ", "Git.status unstaged-only entry should not have staged flag");
ensure(unstagedOnlyEntry.workTreeStatus === "M", "Git.status unstaged-only entry should have modified work tree flag");

const tempDescribe = Git.describe(tempRepoPath);
ensure(tempDescribe && tempDescribe.isRepository === true, "Git.describe temp repo should report repository");
ensure(tempDescribe.error === "", "Git.describe temp repo should not report error");
ensure(tempDescribe.dirty === true, "Git.describe temp repo should be dirty");
ensure(tempDescribe.statusEntryCount === tempStatus.entryCount, "Git.describe temp repo entry count mismatch");
ensure(tempDescribe.statusPorcelain === tempStatus.statusPorcelain, "Git.describe temp repo porcelain mismatch");