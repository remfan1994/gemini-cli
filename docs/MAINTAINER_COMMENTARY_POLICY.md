# Maintainer commentary policy

This project intentionally favors verbose source commentary.

The purpose is not only education. The purpose is project survival.

The code should be understandable to future maintainers, bug fixers, forkers, packagers, and users who want to adapt the project to their own terminal environment.

## Commenting posture

Comments should explain:

```text
what the code does
why the feature exists
why the implementation was chosen
what tradeoffs were accepted
what would break if someone simplified it carelessly
how this implementation relates to the other project tracks
```

Short comments are acceptable for obvious local details, but major subsystems should have longer explanatory comments.

## Areas requiring especially strong comments

```text
JSON generation and parsing
model cache logic
model filtering
runtime colon commands
function-key routing
mouse handling
session log parsing
context snapshots
attachment extraction
inlineData/file handling
external editor invocation
API-key storage behavior
Bash-only fallbacks
```

## Cross-implementation comments

Because this project has multiple implementations, comments should often say whether a feature is:

```text
shared behavior
Bash-only behavior
Bash/Python3 behavior
ncurses/Python reference behavior
future C behavior to preserve
```

## Anti-goal

Do not write terse clever code that only the current author understands.

This is open source terminal software intended to outlive a single maintainer.
