## 0.3.0-bash-only

### Added
- Renamed Bash-only executable identity to `gemini-terminal`
- Matching `gemini-terminal.1` man page
- Stronger maintainer commentary throughout the Bash-only executable

### Changed
- Program identity now distinguishes this dependency-light edition from `gemini-terminal-python3`
- Documentation now emphasizes no-Python dependency posture without reducing the client to a curl example
- Man page now reflects renamed command and current feature set

### Documentation
- Updated NAME/SYNOPSIS/VERSION sections for the renamed executable
- Clarified dependency expectations and optional jq/file behavior
- Clarified best-effort JSON/media limitations without Python

### Notes
- This release is primarily a rename and commentary/documentation pass.
- The Bash-only edition remains an end-user chat client, not a developer example script.
