# INDI Canon EOS M6

USB/PTP Canon EOS M6 driver using a persistent libgphoto2 `Camera*` session.

Flow:

CONNECT
 -> gp_camera_new
 -> gp_camera_init
 -> read ISO/shutter once
 -> keep Camera* session open

CAPTURE
 -> gp_camera_capture
 -> gp_camera_file_get
 -> load CR2 into INDI CCD BLOB
 -> ExposureComplete

DISCONNECT
 -> gp_camera_exit
 -> gp_camera_free

No gphoto2 shell commands.
No list-files.
No polling.
No separate gphoto2 download.
No delete-file.
No camera setting changes.

The design follows libgphoto2's sample-photobooth pattern, where `gp_camera_init()` is called once and the same camera object is reused for multiple captures until `gp_camera_exit()`.

Important:
This version does NOT directly invoke Canon's internal PTP `SetRemoteMode` operation. That operation is defined inside libgphoto2's PTP implementation but is not part of the public high-level libgphoto2 API. This driver first tests the public persistent-session mechanism.
