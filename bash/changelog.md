# Changelog
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
