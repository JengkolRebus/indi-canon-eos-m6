# Canon EOS M6 INDI Driver

Custom INDI CCD driver for Canon EOS M6 using USB/PTP through libgphoto2.

## Overview

This driver provides Canon EOS M6 image capture through INDI using the camera's native USB/PTP connection.

The driver uses a persistent libgphoto2 camera session. The camera is connected once and the USB/PTP session remains open while the driver is connected.

## Architecture

### CONNECT

1. Create the libgphoto2 camera object.
2. Initialize the Canon EOS M6.
3. Read the current ISO and shutter information.
4. Keep the camera session open.

### CAPTURE

1. Trigger image capture through libgphoto2.
2. Download the resulting CR2 file through the existing USB/PTP session.
3. Verify the downloaded CR2.
4. Delete the CR2 from the camera after successful download.
5. Process the CR2 with LibRaw.
6. Load the processed RAW data directly into the INDI CCD framebuffer.
7. Deliver the image to the INDI client as a FITS BLOB.
8. Remove the temporary local CR2 after successful processing.

### DISCONNECT

1. Close the USB/PTP libgphoto2 camera session.
2. Free the libgphoto2 camera object.

## Capture Mode

The driver uses the camera's own capture mechanism through libgphoto2.

Camera settings such as ISO and shutter speed are controlled by the camera. The INDI exposure duration is not used to change the camera's shutter setting.

The downloaded image is processed as RAW data and delivered through the standard INDI CCD BLOB mechanism.

## Data Pipeline

Canon EOS M6 -> USB/PTP -> libgphoto2 -> CR2 download -> CR2 verification -> delete CR2 from camera -> LibRaw -> RAW processing -> INDI CCD framebuffer -> FITS BLOB -> INDI client / Ekos / Polaris

No intermediate FITS file is created on disk by the final capture pipeline.

## Characteristics

- Canon EOS M6 USB/PTP connection.
- Persistent libgphoto2 camera session while connected.
- Native CR2 capture and download.
- CR2 verification before further processing.
- Camera-side CR2 deletion after successful download.
- RAW processing with LibRaw.
- Direct loading into the INDI CCD framebuffer.
- FITS BLOB delivery through INDI.
- No gphoto2 shell commands.
- No camera file listing or polling loop implemented by the driver.
- Temporary local CR2 files are removed after successful processing.

## Requirements

Build dependencies:

- CMake
- C++17 compiler
- INDI development libraries
- libgphoto2 development libraries
- LibRaw development libraries
- CFITSIO development libraries
- pkg-config

On Ubuntu/Debian:

sudo apt install cmake g++ pkg-config libindi-dev libgphoto2-dev libraw-dev libcfitsio-dev

A Canon EOS M6 with USB connection is required for camera testing.

## Build

Clone the repository:

git clone https://github.com/JengkolRebus/indi-canon-eos-m6.git
cd indi-canon-eos-m6

Build:

cmake -S . -B build
cmake --build build -j$(nproc)

The resulting driver binary is:

build/indi_canon_eos_m6

## Install

Install the driver binary:

sudo install -m 755 build/indi_canon_eos_m6 /usr/local/bin/indi_canon_eos_m6

Install the INDI driver definition:

sudo install -m 644 indi_canon_eos_m6.xml /usr/share/indi/indi_canon_eos_m6.xml

Installed files:

/usr/local/bin/indi_canon_eos_m6
/usr/share/indi/indi_canon_eos_m6.xml

## Running

### Polaris / INDI Web Manager

After installation, select:

Canon EOS M6

as the camera driver.

### KStars / Ekos

Select:

Canon EOS M6

as the camera driver in the Ekos equipment profile.

### Manual

The driver can also be started directly with:

indiserver -vv /usr/local/bin/indi_canon_eos_m6

## Limitations

- Camera exposure settings are controlled on the camera.
- The INDI exposure duration does not change the camera's shutter setting.
- The driver depends on libgphoto2 support for the Canon EOS M6.
- libgphoto2 does not provide a generic public abort operation for the capture operation used by this driver, so an abort request cannot guarantee an immediate physical capture stop.
- The temporary local CR2 is removed after successful processing.

## Testing Status

### KStars / Ekos

Tested:

- Canon EOS M6 connection.
- USB/PTP camera session.
- CR2 capture and download.
- CR2 verification.
- Camera-side CR2 deletion.
- RAW processing.
- FITS BLOB delivery through INDI.
- Multiple image captures.

### Polaris

Tested:

- INDI connection.
- Canon EOS M6 driver through Polaris INDI Web Manager.
- FITS image delivery.
- Multiple-frame capture sequence.

## Credits

Built using:

- INDI
- libgphoto2
- LibRaw
- CFITSIO

Canon EOS M6 driver developed for use with INDI-compatible astronomy software.

Developed with assistance from ChatGPT by OpenAI.
