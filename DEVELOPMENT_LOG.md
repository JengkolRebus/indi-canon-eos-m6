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

## 2026-09-21 — CR2 to FITS Integration

### FITS Pipeline
- Added LibRaw support for reading Canon CR2 RAW data.
- Added CFITSIO support for generating FITS files.
- Capture pipeline now performs:
  - Capture CR2 through libgphoto2
  - Download CR2 to local storage
  - Verify downloaded file
  - Delete CR2 from camera
  - Convert CR2 to FITS
  - Load FITS into the INDI CCD BLOB
  - Complete the exposure through `ExposureComplete()`

### INDI FITS Transfer
- FITS transfer format is enabled by default.
- Native transfer format is disabled by default.
- FITS images are successfully delivered through the INDI CCD BLOB
  and displayed by KStars.

### RAW Geometry Investigation
LibRaw reports the following geometry for the Canon EOS M6 CR2:

- Full RAW: `6288 x 4056`
- Visible RAW area: `6024 x 4020`
- Crop offset: `264, 36`

The CR2-to-FITS conversion currently crops the LibRaw RAW buffer to
the reported visible RAW area.

The RAW row stride (`raw_pitch`) and CCD geometry are still under
investigation because the resulting FITS image currently shows a
horizontal band at the bottom.

### Current Status
- CR2 capture/download/delete pipeline remains functional.
- CR2 to FITS conversion is integrated.
- FITS BLOB delivery is functional.
- RAW crop geometry is identified.
- Final RAW stride handling and CCD geometry alignment are not yet
  finalized.

## Future Development

Add future development notes below this line.
