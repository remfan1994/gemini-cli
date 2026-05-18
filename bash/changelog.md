# Changelog
## 2.5

### Added
- Temporary remfan1994 Project Lead startup notice for new blank sessions
- `STARTUP_NOTICE` config key
- `MODEL_TEST_PROMPT` config key
- `--test-model <model_id>` command
- `--test-model <model_id> --save` command
- User-operated model testing workflow
- Context snapshots saved into session logs
- Context snapshot preference during resume-time memory rebuild
- Removed-autoscan explanation message for users who still run `--autoscan-model`

### Removed
- Active `--autoscan-model` behavior
- Automatic probing of all available models

### Changed
- Bash model workflow now mirrors the ncurses philosophy: list models, choose one, test it, then save it if desired
- `--select-model` now offers to test the selected model before saving
- Startup notice is displayed as a transcript-style message from `remfan1994`
- Startup notice is not logged, not sent to Gemini, and not stored in memory
- Startup notice is removed from the visible terminal when the first message is sent
- Resume memory rebuild now prefers the latest saved context snapshot
- Help output now documents `--test-model`, `MODEL_TEST_PROMPT`, and `STARTUP_NOTICE`

### Documentation
- Updated man page synopsis with `--test-model`
- Removed autoscan from normal documented workflow
- Added STARTUP NOTICE section
- Added MODEL TESTING section
- Updated MODEL SELECTION section
- Updated CONFIGURATION section
- Updated MEMORY MODEL section
- Updated LIMITATIONS section

### Notes
- `--autoscan-model` is intentionally no longer an active workflow.
- The Bash version remains line-oriented and does not attempt to clone ncurses menus.
- The Project Lead notice is temporary UI text only, not conversation context.
## 2.4

### Added
- `CONTEXT_TURNS` config key for clearer conversation-memory terminology
- Backward compatibility for legacy `HISTORY_LIMIT` config key
- Resume-time context rebuild from session logs
- Context rebuild for existing named sessions
- Autoscan challenge validation
- Default autoscan prompt: `What is the capital of Texas?`
- Default autoscan expected substring: `austin`
- `AUTOSCAN_PROMPT` config key
- `AUTOSCAN_EXPECTED` config key
- Autoscan failure reporting when a model returns text but fails the expected-answer check
- Startup display of active context-turn count
- Startup display of loaded context-entry count when an existing session log is used

### Changed
- Autoscan no longer accepts any non-empty text response as success
- Autoscan now requires the response to contain the expected answer substring
- Internal memory variable terminology shifted from history-style wording to context-buffer wording
- `CONTEXT_TURNS` now represents user/assistant exchanges rather than individual speaker entries
- Resume memory rebuild keeps up to `CONTEXT_TURNS * 2` speaker entries from the session log
- Existing named sessions now regain recent conversation context instead of only continuing the log
- Help output now documents `CONTEXT_TURNS`, `AUTOSCAN_PROMPT`, and `AUTOSCAN_EXPECTED`
- Man page now describes `HISTORY_LIMIT` as a legacy alias instead of the preferred setting

### Documentation
- Updated CONFIGURATION section with `CONTEXT_TURNS`
- Documented legacy `HISTORY_LIMIT`
- Added autoscan challenge details
- Documented `AUTOSCAN_PROMPT`
- Documented `AUTOSCAN_EXPECTED`
- Updated MEMORY MODEL section
- Updated MODEL AUTOSCAN section
- Updated SESSION MANAGEMENT section
- Updated LIMITATIONS section to clarify autoscan verifies only the configured challenge

### Notes
- This brings the Bash edition into closer memory parity with the ncurses edition.
- The Bash version remains intentionally line-oriented and does not attempt to imitate ncurses UI behavior.
- The autoscan challenge is empirical and configurable.
- The default Texas/Austin challenge reduces false positives from models that merely return any non-empty text.

## 2.3

### Added
- `--models` command for listing Gemini generateContent-compatible models
- `--autoscan-model` command for testing available models until one works
- `--autoscan-model --save` for saving the first working model to config
- `--select-model` numbered model selection menu
- `--select-model --save` for saving selected model without confirmation
- `--set-api-key` helper for saving GEMINI_API_KEY to config
- Model discovery documentation in script comments and man page
- Model autoscan documentation in script comments and man page
- Fallback model labeling when MODEL is not configured
- API-key setup documentation
- Safer JSON payload construction using Python json.dumps
- Better Gemini API error reporting
- Session-name sanitization
- Attachment numbering that avoids overwriting previous attachment files

### Changed
- MODEL may now be unset in config; gemini-cli uses a clearly labeled fallback model
- `gemini-2.5-flash` is now documented as a fallback, not as a permanent recommendation
- Prompts are now encoded through Python JSON generation instead of shell string interpolation
- Model selection is now future-resistant through live model listing and autoscan
- Bash edition remains line-oriented rather than imitating ncurses UI behavior

### Documentation
- Updated man page synopsis with model-management commands
- Added MODEL DISCOVERY section
- Added MODEL AUTOSCAN section
- Added MODEL SELECTION section
- Added MODEL FALLBACK section
- Added API KEY SETUP section
- Updated CONFIGURATION section
- Updated LIMITATIONS section

### Notes
- Autoscan finds a functional model, not necessarily the best model.
- Autoscan may consume multiple API calls.
- The ncurses edition remains the preferred interface for full-screen interactive behavior.
- The Bash edition remains the lightweight fallback and scriptable utility version.


## 2.2

### Added
- Built-in help system
- -h alias
- --help alias
- help command alias

### Changed
- CLI became self-documenting
- Added internal usage reference output

### Documentation
- Updated man page synopsis
- Added help system documentation

## 2.1

### Added
- Configurable SESSION_DIR support
- Configurable ATTACHMENT_DIR support

### Changed
- Directory creation now occurs after config parsing
- Storage paths are no longer hardcoded

### Documentation
- Updated man page configuration section
- Synchronized script, man page, and changelog


## 2.0

### Added
- Automatic attachment extraction system
- Markdown fenced code block detection
- Automatic artifact saving into attachment directory
- Attachment filename numbering system
- Language-aware file extension mapping
- Attachment placeholder notices in terminal output

### Supported Attachment Types
- bash/sh -> .sh
- python -> .py
- json -> .json
- yaml/yml -> .yml
- javascript -> .js
- html -> .html
- css -> .css
- sql -> .sql
- fallback unknown types -> .txt

### Changed
- Terminal output now omits large code blocks
- Session logs preserve full original Gemini output
- Tool behavior shifted toward artifact-oriented workflow

### Documentation
- Added attachment system documentation to man page
- Added attachment directory documentation


## 1.9

### Added
- Fully annotated script source
- Extensive architectural comments
- Publish-oriented inline documentation
- Expanded maintenance explanations throughout script

### Documentation
- Synchronized man page with documented architecture


## 1.8

### Added
- Config file support
- GEMINI_API_KEY loading from config file
- Environment variable override precedence
- Version flag (--version)

### Changed
- Runtime configuration became externalized
- API key handling formalized

### Documentation
- Added configuration section to man page
- Added environment precedence documentation


## 1.7

### Added
- Named chat sessions
- Session naming via:
    gemini-cli <name>
- Session resume support:
    gemini-cli --resume <name>
- Session listing:
    gemini-cli --list

### Changed
- Timestamp-only sessions became optional
- Session handling became stateful and reusable

### Documentation
- Added session management section to man page


## 1.6

### Added
- Lightweight rolling memory system
- Sliding history context window
- HISTORY_LIMIT configuration
- Context trimming system

### Changed
- API calls now include conversational memory
- Reduced uncontrolled token growth

### Documentation
- Added memory model documentation


## 1.5

### Added
- Timestamped session logging
- Persistent chat history files
- Username-based prompt display
- Timestamped UI messages

### Changed
- Conversations became persistently archivable

### Documentation
- Added session file documentation


## 1.4

### Added
- Visual separator system
- END INPUT / BEGIN OUTPUT markers
- "Generating response..." status line

### Changed
- Improved readability of long conversations
- Improved distinction between prompts and responses


## 1.3

### Added
- Cleaner terminal formatting
- Structured startup banner
- Model display at startup

### Changed
- Improved terminal readability
- Reduced ambiguous terminal state


## 1.2

### Added
- Gemini API integration via curl
- Dynamic model selection
- Python-based JSON parsing

### Changed
- Removed dependency on browser interface
- Reduced dependency footprint versus larger AI clients


## 1.1

### Added
- Basic interactive Gemini terminal chat loop
- Persistent stdin prompt
- Ctrl+C exit workflow
- Multi-line input support

### Changed
- Replaced manual curl pasting workflow


## 1.0

### Initial Release

### Features
- Minimal Bash-based Gemini API client
- ChromeOS/container-friendly design
- Lightweight dependency model
- Shell-oriented conversational interface
