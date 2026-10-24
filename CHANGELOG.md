# Changelog

## [Unreleased]

### Security

- Replaced disabled Discord TLS verification and a hardcoded IP with hostname/CA
  verification, clock checks and bounded response waiting.
- Required a private GPS key on POST updates, validated coordinates, rejected
  browser requests and expired stale phone positions.
- Moved local credentials into ignored configuration, added a fail-closed example,
  and documented the authenticated iOS shortcut and private-hotspot HTTP limitation.
- Firmware must be reflashed and its shortcut updated together; precise positions
  and webhook response bodies are no longer written to the serial log.
