# ya-dev never ships

Growser-ya-dev (growser#397). This branch carries an experiment. It is never
merged into `build-line`, never tagged, never built as a public release.

- The commit that added this file marks the branch. A tree that descends from
  it is refused by `scripts/check-never-ship.py` (control repo) in every public
  release build, and by the fork's pre-push hook on every ref but `ya-dev`.
  Deleting the file does not help: the history is checked, not the tree.
- Every commit here has `Growser-ya-dev` in its message, and every line it
  changes carries the same marker, so a cherry-pick or a squash merge is
  refused too. The hook refuses a push to `ya-dev` with an unmarked commit.
- `build/growser.gni` asserts `!growser_public_release`, so a public build of
  this branch fails at `gn gen` even without the scripts.
