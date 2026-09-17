# Development Log — INDI Canon EOS M6

## 2026-09-18 — Version 1.0 Final

### Project
- Driver: `indi-canon-eos-m6`
- Device: Canon EOS M6
- Connection: USB/PTP
- Backend: libgphoto2
- INDI class: CCD
- Version: 1.0

### Architecture

The driver keeps one libgphoto2 `Camera*` session open for the
duration of the INDI connection.

CONNECT:
- `gp_camera_new()`
- `gp_camera_init()`
- Read ISO and shutter once
- Keep the camera session open

CAPTURE:
- `gp_camera_capture()`
- `gp_camera_file_get()`
- Verify downloaded CR2
- Delete the downloaded CR2 from the camera
- Load CR2 into INDI CCD BLOB
- `ExposureComplete()`

DISCONNECT:
- `gp_camera_exit()`
- `gp_camera_free()`

### Design Decisions
- Native CR2 transfer is used.
- No `gphoto2` shell commands are used by the driver.
- No file listing is performed during the connected session.
- No polling is used.
- The driver does not change camera settings.
- ISO and shutter are read once during connection.
- Multiple captures work without disconnecting/reconnecting.
- Camera-side CR2 deletion occurs after successful download and
  verification.

### Testing
- Camera connection tested successfully.
- ISO and shutter were read successfully.
- CR2 capture and download tested successfully.
- CR2 displayed successfully through the INDI CCD BLOB.
- Multiple captures without reconnecting tested successfully.
- Camera-side deletion verified successfully after disconnect.

### Build
Final executable:

`indi_canon_eos_m6`

Installed to:

`/usr/local/bin/indi_canon_eos_m6`

Driver XML installed to:

`/usr/share/indi/indi_canon_eos_m6.xml`

### Repository
GitHub:

`https://github.com/JengkolRebus/indi-canon-eos-m6`

Final README includes project credit for development assistance from
ChatGPT by OpenAI.

### Cleanup
Old EOS M6 development builds and temporary test files were removed.
The final development source is kept in:

`~/indi-eosm6-build/indi-canon-eos-m6`

## Future Development

Add future development notes below this line.
