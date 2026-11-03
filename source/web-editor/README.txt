V-STATION FM1 WEB EDITOR - RC1

1. Flash RC1 to the FM1.
2. Connect the FM1 USB-C port to the computer.
3. Windows: double-click start-editor.bat
   Linux:   run ./start-editor.sh
4. Chrome or Edge opens to http://127.0.0.1:8765/
5. Click CONNECT FM1 and allow MIDI / SysEx access.

RC1 keeps the TEST40/41 Web MIDI/SysEx editor transport and adds a persistent
User Preset Library stored in the browser:
- 170 active synth parameters; the removed legacy seven-field tail is not shown or sent
- 175 factory presets after removing all Motion presets; the factory bank remains alphabetical
- Save Current / Update Selected user presets
- search, category filter and A-Z / category / newest sorting
- import one or many individual preset JSON files
- export a selected preset as JSON
- export/import the entire user library as one JSON backup bundle
- legacy TEST40 177-value JSON preset import; removed trailing fields are ignored
- factory preset browser remains separate and read-only
- Init, Compare, Revert and Randomize
- FM1 Hardware page with pool, CPU mode, scan rate and live performance telemetry
- live hardware changes reflected back into the browser

The User Preset Library is local to the browser profile and localhost origin.
Use EXPORT LIBRARY periodically if the collection matters to you.
