# Config reference

All current implementations share this config file:

```text
~/.config/gemini-cli/config
```

The config format is intentionally simple:

```text
KEY=VALUE
```

Blank lines and comment lines beginning with `#` may be ignored by implementations.

## Core keys

### GEMINI_API_KEY

Optional API key stored in the config file.

Environment variable precedence:

```text
GEMINI_API_KEY environment variable wins over config-file GEMINI_API_KEY.
```

Security note:

```text
Saving API keys in config is convenient but stores them in plaintext.
```

### MODEL

Gemini model identifier.

Examples:

```text
MODEL=gemini-2.5-flash
MODEL=models/gemini-2.5-flash
```

Implementations should normalize `models/name` to `name` where practical.

### CONTEXT_TURNS

Preferred key for conversation memory size.

Meaning:

```text
Number of recent user/assistant exchanges to send back to Gemini as context.
```

This does not mean shell command history, transcript length, or log size.

Default:

```text
CONTEXT_TURNS=12
```

### HISTORY_LIMIT

Legacy alias for `CONTEXT_TURNS`.

This remains accepted for old configs, but new docs and UI should prefer `CONTEXT_TURNS`.

## Storage keys

### SESSION_DIR

Directory where session logs are stored.

Typical default:

```text
SESSION_DIR=~/.gpt
```

### ATTACHMENT_DIR

Directory where extracted code blocks, generated inline data, and local artifacts are stored.

Typical default:

```text
ATTACHMENT_DIR=~/.gpt/attachments
```

## Model-list and filter keys

These keys control model-list filtering and display where supported.

### MODEL_FILTER_GENERATE_CONTENT

If enabled, show only models reporting `generateContent` support.

### MODEL_FILTER_REQUIRE_TOKENS

If enabled, require nonzero or present input/output token limits where the model metadata provides them.

### MODEL_FILTER_HIDE_PREVIEW

If enabled, hide models with names or descriptions indicating preview/experimental status where practical.

### MODEL_MIN_INPUT_TOKENS

Minimum input token limit required for a model to appear in filtered lists.

### MODEL_MIN_OUTPUT_TOKENS

Minimum output token limit required for a model to appear in filtered lists.

### MODEL_SORT_ORDER

Suggested supported values:

```text
name
input-tokens
output-tokens
```

Implementations may support additional sort modes.

## Editor and UI keys

### EDITOR

Preferred external editor command.

If unset, implementations should try:

```text
VISUAL environment variable
EDITOR environment variable
common editor fallback such as nano or vi
```

### INPUT_PLACEHOLDER

Input placeholder text for full-screen UI clients.

Example:

```text
INPUT_PLACEHOLDER=Type your chat message here...
```

### MONOCHROME

Disables reliance on dim/grey styling where supported.

Suggested truthy values:

```text
1 yes y true on
```

### STARTUP_NOTICE

Controls the temporary Project Lead startup notice.

Suggested truthy values:

```text
1 yes y true on
```

Suggested false values:

```text
0 no n false off
```

## Model-test keys

### MODEL_TEST_PROMPT

Prompt used by one-model test workflows.

The current project philosophy favors user-operated testing over automatic probing.

## Attachment keys

### MAX_ATTACHMENT_BYTES

Maximum local file size to allow for prompt attachments.

Implementations should refuse files above this threshold rather than risk building huge request bodies.

## Compatibility guidance

When adding config keys:

```text
- document the key in the man page
- document it in this file
- add comments near the code that reads the key
- update README if it changes user-visible behavior
- avoid renaming existing keys unless a legacy alias is kept
```
