# Release hygiene notes for 0.9.x

The project is in a stabilization phase before any native C ncurses implementation begins.

## Current executable names

    gemini/bash/bash-only/ttychatter-gemini-bash
    gemini/bash/python3/ttychatter-gemini-python3
    gemini/ncurses/python/ttychatter-gemini-ncurses-python
    gemini/ncurses/c/ttychatter-gemini-ncurses        planned

## Current man page names

    ttychatter-gemini-bash.1
    ttychatter-gemini-python3.1
    ttychatter-gemini-ncurses-python.1
    ttychatter-gemini-ncurses.1                planned

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

    ~/.config/ttychatter/gemini/config

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
