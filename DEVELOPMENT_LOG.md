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

## 2026-09-22 — Direct RAW to INDI FITS Pipeline

### RAW Pipeline
- LibRaw is used to read Canon EOS M6 CR2 RAW data.
- The RAW visible area is copied directly into the INDI CCD framebuffer.
- No intermediate FITS file is created by the driver.
- INDI handles the FITS BLOB encoding/transfer.
- Temporary local CR2 files are removed after successful RAW loading.

Capture pipeline:
- Capture CR2 through libgphoto2
- Download CR2 to local storage
- Verify downloaded file
- Delete CR2 from camera
- Load visible RAW data directly into the INDI CCD framebuffer
- Remove the temporary local CR2
- Complete the exposure through `ExposureComplete()`

### EOS M6 RAW Geometry
LibRaw reports:

- Full RAW: `6288 x 4056`
- Visible RAW area: `6024 x 4020`
- Crop offset: `264, 36`
- RAW pitch: `12576` bytes

The driver copies the visible RAW area using the LibRaw-reported
`raw_pitch` and crop offsets.

### FITS Transfer
- FITS transfer format is enabled by default.
- Native transfer format is disabled by default.
- FITS images are successfully delivered through the INDI CCD BLOB.
- KStars successfully displays the FITS images.
- Sequence capture tested successfully with 5 consecutive frames.

### Polaris Testing
- Polaris successfully captures through the installed INDI driver.
- The temporary CR2 is removed after successful processing.
- `/tmp/indi-eosm6/` remains empty after successful capture.
- Polaris required write permission to `/tmp/indi-eosm6/` because the
  Polaris systemd service runs as user `polaris`.

### Installation
The driver is installed as:

`/usr/local/bin/indi_canon_eos_m6`

Driver XML:

`/usr/share/indi/indi_canon_eos_m6.xml`

The installed binary is used by INDI through the standard driver XML.

## Future Development

Add future development notes below this line.
