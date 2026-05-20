# Release hygiene notes for 0.9.x

The project is in a stabilization phase before any native C ncurses implementation begins.

## Current executable names

    bash/bash-only/gemini-terminal
    bash/python3/gemini-terminal-python3
    ncurses/python/gemini-ncurses-python
    ncurses/c/gemini-ncurses        planned

## Current man page names

    gemini-terminal.1
    gemini-terminal-python3.1
    gemini-ncurses-python.1
    gemini-ncurses.1                planned

## Items to keep synchronized

- executable name
- PROGRAM constant
- --help output
- --version output
- man page NAME/SYNOPSIS/USAGE
- README examples
- changelog entries
- comments that mention implementation identity

## Intentional legacy naming

The config path remains:

    ~/.config/gemini-cli/config

This is intentional for backward compatibility with earlier releases.

## Generated files to exclude

Do not commit:

    __pycache__/
    *.pyc
    temporary editor files
    local session logs
    local attachment output

## Pre-C stabilization focus

- Python ncurses global navigation consistency
- low-resolution behavior
- model-cache and model-selector UX
- diagnostics clarity
- file attachment edge cases
- documentation/commentary completeness
