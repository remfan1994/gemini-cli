# gemini-cli

A terminal-based Gemini AI interface designed for Unix-like systems, focusing on simplicity, persistence, and artifact-aware workflows.

This project provides a lightweight way to interact with Gemini models directly from the terminal, with support for sessions, logging, and automatic code extraction.

---

## Overview

`gemini-cli` is a command-line chat interface for Gemini that emphasizes:

- Persistent chat sessions
- Named and resumable conversations
- Structured logs
- Automatic extraction of code artifacts
- Minimal dependencies
- Terminal-first interaction design

The project is currently implemented in Bash, with future interfaces planned for ncurses (Python) and potentially a native C version.

---

## Features

### Core

- Interactive terminal chat
- Session persistence
- Named sessions
- Session resume support
- Session listing
- Config file support
- API key via environment or config file

### Output handling

- Clear separation of:
  - user input
  - system status
  - model output
- Visual separators for readability
- “Generating response” status indicator

### Artifact system

- Automatic detection of markdown code blocks
- Extraction into filesystem attachments
- Language-aware file extensions
- Organized attachment storage per session

### Logging

- Timestamped session logs
- Full raw API response preserved
- Human-readable chat reconstruction

---

## Installation

### Requirements

- Bash
- curl
- python3

Optional:
- rlwrap (improves input editing in Bash mode)

---

## Configuration

Configuration file location:

~/.config/gemini-cli/config


Example:

MODEL=gemini-2.5-flash
HISTORY_LIMIT=12
GEMINI_API_KEY=your_api_key_here
SESSION_DIR=~/.gpt
ATTACHMENT_DIR=~/.gpt/attachments


### Environment override

export GEMINI_API_KEY="your_key"


Environment variables override config file values.

---

## Usage

### Start a new session

gemini-cli


### Named session

gemini-cli mysession


### Resume session

gemini-cli --resume mysession


### List sessions

gemini-cli --list


### Help

gemini-cli --help
gemini-cli -h


### Version

gemini-cli --version


---

## Session model

Sessions are stored as:

SESSION_DIR/<session-name>.log


Each log contains:

- timestamped user input
- raw model output
- full conversation history

---

## Attachment system

When Gemini returns code blocks, they are:

1. extracted automatically
2. saved to:

ATTACHMENT_DIR/


3. replaced in terminal output with a reference marker

Example:

[attachment saved]
file: mysession-python-01.py


---

## Architecture

Current implementation (Bash):

- CLI orchestration: Bash
- API calls: curl
- JSON parsing: Python
- Storage: filesystem logs
- Attachments: Python extraction layer

Planned interfaces:

### ncurses version
- Full terminal UI
- Scrollable chat history
- Input box with proper editing
- Status bar and session navigation

### C version (future)
- Native ncurses UI
- libcurl transport
- minimal runtime dependency
- high-performance terminal tool

---

## Philosophy

- Unix-first design
- filesystem persistence
- minimal dependencies
- transparent logs
- reproducible sessions
- artifact-oriented AI interaction

---

## Project structure

bash/
gemini-cli

ncurses/
planned implementation

README.md
LICENSE.txt


---

## License

See LICENSE.txt for details.

---

## Status

Early-stage terminal AI interface evolving toward full ncurses and native implementations.

