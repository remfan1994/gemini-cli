## 2.9

### Added
- Renamed Bash-with-Python3 executable identity to `gemini-terminal-python3`
- Matching `gemini-terminal-python3.1` man page
- Stronger maintainer commentary throughout the executable

### Changed
- Program identity now distinguishes this edition from the Bash-only edition
- Documentation now explicitly describes this implementation as Bash orchestration plus Python 3 helper snippets
- Man page now reflects renamed command and current feature set

### Documentation
- Updated NAME/SYNOPSIS/VERSION sections for the renamed executable
- Clarified the role of Python 3 helpers in JSON, URL quoting, attachment parsing, and session handling
- Clarified that this edition favors reliability while remaining lighter than the ncurses UI

### Notes
- This release is primarily a rename and commentary/documentation pass.
- The Bash-only edition remains separate as `gemini-terminal`.
