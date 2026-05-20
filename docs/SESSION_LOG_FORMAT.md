# Session log format

Session logs are intended to be human-readable plain text. This is a core project design decision.

The session log is not meant to be a hidden database. Users should be able to inspect, copy, grep, back up, and recover sessions without special tools.

## Basic entry format

Standard entries use this shape:

```text
[HH:MM:SS] speaker: message
```

Examples:

```text
[12:00:00] alice: What is DNS?
[12:00:05] Gemini: DNS is the system that maps names to IP addresses.
```

Multi-line messages may continue after the first line. Parsers should treat a new bracketed timestamp line as the beginning of a new entry.

## Speaker names

Common speaker names:

```text
<local user>      user message, usually from whoami or environment
Gemini            model response
system            client-generated metadata or context snapshots
remfan1994        temporary UI-only startup notice, not logged
```

The `remfan1994` startup notice should not be written to the log.

## System entries

System entries are allowed, but they must remain readable.

Common system uses:

```text
CONTEXT_BEGIN / CONTEXT_END snapshots
model change notices
session metadata
```

System entries should not make logs unreadable or force users to treat logs as binary/state files.

## Context snapshots

Context snapshots are used so clients can resume conversation continuity without reparsing the entire log.

Suggested shape:

```text
[12:34:56] system: CONTEXT_BEGIN
User: earlier user message
Assistant: earlier assistant response
User: later user message
Assistant: later assistant response
[12:34:56] system: CONTEXT_END
```

Implementations may parse the latest snapshot first. If no snapshot exists, implementations may rebuild context from recent user/Gemini entries.

## Parsing guidance

Maintainers should be careful with this format. It is intentionally simple, but it is not perfect.

Risks:

```text
- user text may contain lines that resemble timestamps
- model text may contain code blocks with bracketed text
- old logs may lack context snapshots
- future logs may contain new system entries
```

Parsers should be forgiving. If a context snapshot cannot be parsed, fall back to recent normal entries rather than crashing.

## Attachment references

Terminal display may replace generated code blocks or inline data with attachment notices, but the raw Gemini response should be preserved in logs where practical.

Example display notice:

```text
[attachment saved]
file: session-python-01.py
location: ~/.gpt/attachments
```

## Stability rule

The log format should not be changed casually. The future C ncurses version should be able to read logs produced by the Bash and Python ncurses versions.
