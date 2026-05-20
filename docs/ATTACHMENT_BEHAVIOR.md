# Attachment behavior

Attachments are one of the project features that most needs clear maintenance guidance. This document describes intended behavior rather than a single implementation's internals.

## Purpose

The attachment system exists because terminal chat becomes hard to use when large code blocks, generated files, or local file contents flood the screen.

The project should separate:

```text
conversation text
large code/data artifacts
local files attached to prompts
generated inline data returned by Gemini
```

## Code block extraction

When Gemini returns fenced markdown code blocks, clients may save them to files and replace the display with a notice.

Example:

```text
```python
print("hello")
```
```

May become:

```text
[attachment saved]
file: session-python-01.py
location: ~/.gpt/attachments
```

The original raw Gemini response should remain preserved in the session log where practical.

## File naming

Recommended naming pattern:

```text
<session>-<type>-NN.<extension>
```

Examples:

```text
android-backup-python-01.py
linux-help-bash-02.sh
notes-json-01.json
```

Implementations should avoid overwriting existing attachments. Numbering should continue from the highest existing number where practical.

## Extension mapping

Typical markdown language to extension mapping:

```text
bash/sh/shell    .sh
python/py        .py
json             .json
yaml/yml         .yml
javascript/js    .js
html             .html
css              .css
sql              .sql
c                .c
cpp/c++          .cpp
unknown/text     .txt
```

## Local file attachments

Text-like files may be attached as text parts.

Binary/media files require base64 and Gemini API support. Clients may attempt inlineData-style attachments where supported, but this behavior depends on model capability, API limits, MIME type, and file size.

## Generated media or inline data

If Gemini returns inline data parts, clients may save them into ATTACHMENT_DIR.

This is conditional. Not every model returns files, images, audio, or video. A client that is prepared to save inline data may still never receive it from a particular model.

## Safety rules

Implementations should enforce or document:

```text
MAX_ATTACHMENT_BYTES
filename sanitization
MIME fallback behavior
base64 handling limits
clear errors for unsupported files
```

## Bash-only limitations

The Bash-only client should attempt useful attachment behavior without Python, but it should document that JSON and binary handling are best-effort without Python or jq.

This does not make the Bash-only client a lesser project track. It is the dependency-light track, not the featureless track.
