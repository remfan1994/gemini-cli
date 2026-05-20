## 0.9.1 repository integration pass

### Added
- Documentation for Bash runtime colon-command mode
- Release candidate audit checklist
- Updated README section describing the renamed executable family
- Updated README section describing command-mode parity between Bash and ncurses

### Changed
- Integrated latest Bash runtime-command-mode files into the repo package
- Integrated latest renamed Python ncurses reference files into the repo package
- Refreshed repository structure documentation around `bash/bash-only`, `bash/python3`, `ncurses/python`, and future `ncurses/c`
- Clarified that `~/.config/gemini-cli/config` remains the compatibility config path despite executable renames

### Notes
- No new ncurses feature work was introduced in this pass.
- No new Bash runtime-command feature work was introduced in this pass.
- This pass is intended to reduce drift before further stabilization and eventual C implementation.
