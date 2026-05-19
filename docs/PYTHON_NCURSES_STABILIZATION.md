# Python ncurses stabilization checklist

This checklist tracks work that should be completed before the Python ncurses edition is treated as the stable behavioral reference for the future C ncurses implementation.

## Environment and diagnostics

- [x] Provide `--doctor` command-line diagnostics.
- [x] Provide diagnostics screen inside the TUI.
- [x] Check config file visibility.
- [x] Check session directory writability.
- [x] Check attachment directory writability.
- [x] Check model-cache directory writability.
- [x] Check editor resolution.
- [x] Check DNS resolution.
- [ ] Add optional deeper online model-cache validation if users explicitly request it.

## UI stability

- [x] Global function-key navigation across feature screens.
- [x] Clickable command strip.
- [x] Low-resolution-safe command menu.
- [x] Scrollable help/error/credits pages.
- [ ] Field-test low-resolution terminals.
- [ ] Field-test Chromebook terminal behavior.
- [ ] Field-test mousewheel behavior in several terminal emulators.

## Model workflow

- [x] Cached model list.
- [x] Explicit model-list update.
- [x] Metadata-based filters.
- [x] Manual selected-model test.
- [x] No automatic model probing/autoscan.
- [ ] Add clearer model-cache age display in header or model menu if useful.

## Sessions and memory

- [x] Session browser.
- [x] Session rename.
- [x] Context snapshots in logs.
- [x] Memory view/edit/clear/rebuild.
- [ ] Stress-test session rename edge cases.
- [ ] Stress-test memory snapshot parsing with long logs.

## Attachments

- [x] Extract fenced code blocks.
- [x] Attach local text files.
- [x] Attach binary/media files through inlineData when supported.
- [x] Save returned inlineData parts when parseable.
- [ ] Validate MIME fallback behavior.
- [ ] Validate file size limits and error messages.
- [ ] Validate generated attachment naming collisions.

## Documentation

- [x] Keep source comments verbose.
- [x] Keep man page synchronized.
- [x] Keep changelog synchronized.
- [x] Keep README project posture current.
- [ ] Add screenshots or terminal captures if desired.

## C implementation readiness

- [x] Announce C ncurses version.
- [x] Identify Python ncurses as behavioral reference.
- [x] Add diagnostics workflow that the C version can mirror.
- [ ] Freeze Python ncurses feature behavior before starting C implementation.
