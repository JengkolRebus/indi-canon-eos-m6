# INDI Canon EOS M6

USB/PTP INDI CCD driver for the Canon EOS M6 using libgphoto2.

## Design

### CONNECT

```text
CONNECT
 -> gp_camera_new
 -> gp_camera_init
 -> read ISO/shutter once
 -> keep Camera* session open
```

### CAPTURE

```text
CAPTURE
 -> gp_camera_capture
 -> gp_camera_file_get
 -> verify downloaded CR2
 -> delete downloaded CR2 from camera
 -> load CR2 into INDI CCD BLOB
 -> ExposureComplete
```

### DISCONNECT

```text
DISCONNECT
 -> gp_camera_exit
 -> gp_camera_free
```

## Characteristics
- Canon EOS M6
- USB/PTP connection
- libgphoto2
- Native CR2 transfer
- INDI CCD BLOB
- Multiple captures without reconnecting
- Camera-side deletion after successful download
- No gphoto2 shell commands
- No file listing during the connected session
- No polling
- No camera setting changes by the driver
- ISO and shutter are read once when connecting

## Requirements
- INDI
- libgphoto2
- Canon EOS M6 connected through USB

## Build
```bash
cmake -S . -B build
cmake --build build -j$(nproc)
```

The resulting executable is:
```text
build/indi_canon_eos_m6
```

## Install
```bash
sudo cp build/indi_canon_eos_m6 /usr/local/bin/
sudo cp indi_canon_eos_m6.xml /usr/share/indi/
```

Start the driver:
```bash
indiserver -vv /usr/local/bin/indi_canon_eos_m6
```

In KStars/Ekos, select:

```text
Canon EOS M6
```

## Credits

Developed with assistance from ChatGPT by OpenAI.
