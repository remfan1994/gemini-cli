## 0.9.0

### Added
- Renamed Python ncurses executable identity to `gemini-ncurses-python`
- Matching `gemini-ncurses-python.1` man page
- Release-candidate oriented documentation pass
- Stronger maintainer commentary throughout the Python ncurses executable
- Explicit man-page documentation for global function-key navigation
- Updated diagnostics documentation for `--doctor`

### Changed
- Program identity now matches the new repository naming structure
- Python ncurses edition is now documented as the behavioral reference for the future C ncurses version
- Documentation posture now emphasizes end-user terminal chat clients rather than developer curl examples
- Man page now reflects renamed command, current config keys, model cache workflow, memory snapshots, diagnostics, and limitations

### Documentation
- Updated NAME/SYNOPSIS/VERSION sections for the renamed executable
- Added maintainer-focused commentary emphasis
- Updated model discovery and model testing explanations
- Updated diagnostics and memory model sections
- Clarified that terminal drag selection belongs to the terminal emulator

### Notes
- This release is primarily a rename, commentary, documentation, and stabilization pass.
- No new major feature area is introduced in this release.
- The goal is to move the Python ncurses implementation closer to serving as a stable reference for the future native C version.
