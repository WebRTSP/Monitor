[![Get it from the Snap Store](https://snapcraft.io/en/dark/install.svg)](https://snapcraft.io/video-monitor)

## Remote video streams viewer

Shows video from configured WebRTSP source to default output.
Created as part of DIY project of doorbell cam monitor with optional motion detection.

### Requirements
1. Raspberry Pi 3 or newer (or maybe some other 64bit ARM boards with ability to run SNAP packages)
2. Raspberry Pi OS Lite 64bit (or mayber some other 64bit OS with ability to run SNAP packages and console only mode)
3. Some h264 video stream source:
    * IP Cam with RTSP protocol support
    * IP Cam with ONVIF protocol support
    * WebRTSP server like [WebRTSP/ReStreamer](https://github.com/WebRTSP/ReStreamer)
    * WebRTSP Record client like [WebRTSP/RecordStreamer](https://github.com/WebRTSP/RecordStreamer)
4. Monitor connected to default HDMI output

### How to install it as Snap package
1. Run: `sudo snap install video-monitor --edge`
2. To see application logs in realtime you can run `sudo snap logs video-monitor -f`

### How to edit config file
1. `sudoedit /var/snap/video-monitor/common/monitor.conf`
2. To load updated config it's required to restart Snap: `sudo snap restart video-monitor`

### How to use it with RTSP source
1. [Install app](#how-to-install-it-as-snap-package)
2. Open config file for edit ([as described above](#how-to-edit-config-file)) and replace content with something like
```
source: {
  url: "rtsp://your_cam_ip_or_dns:port/path"
}
```
3. Restart Snap: `sudo snap restart video-monitor`

### How to use it with ONVIF source (with optional motion detection)
1. [Install app](#how-to-install-it-as-snap-package)
2. Open config file for edit ([as described above](#how-to-edit-config-file)) and replace content with something like
```
source: {
  onvif: "http://ip.cam:8080/"
  track-motion: true // to show video preview when motion is detected by ONVIF camera
  motion-preview-time: 30 // minimum time to preview video after motion detected
}
```
3. Restart Snap: `sudo snap restart video-monitor`

### How to use it as WebRTSP client (with optional motion detection on server side)
1. [Install app](#how-to-install-it-as-snap-package)
2. Open config file for edit ([as described above](#how-to-edit-config-file)) and replace content with something like
```
source: {
  // "webrtsp://" for plain WebSocket connection (ws://)
  // "webrtsps://" for Secure WebSocket connection (wss://)
  url: "webrtsps://ipcam.stream/%C5%A0trbsk%C3%A9%20pleso"
}
```
3. Restart Snap: `sudo snap restart video-monitor`
4. Install [WebRTSP/ReStreamer](https://github.com/WebRTSP/ReStreamer/tree/master?tab=readme-ov-file#how-to-install-it-as-snap-package-and-try)
5. Configure `WebRTSP/RecordStreamer`:
    * [Without motion detection](https://github.com/WebRTSP/ReStreamer/tree/master?tab=readme-ov-file#how-to-configure-your-own-source)
    * [With motion detection](https://github.com/WebRTSP/ReStreamer/tree/master?tab=readme-ov-file#how-to-use-it-as-cloud-nvr-for-ip-cam-not-accessible-directly)
      and add `restream: true` to streamer configuration to make video stream accessable for Video Monitor when recording is active

### How to use it as WebRTSP record server (with optional motion detection)
1. [Install app](#how-to-install-it-as-snap-package)
2. Open config file for edit ([as described above](#how-to-edit-config-file)) and replace content with something like
```
record-server: {
  token: "some-random-string"
}
```
3. Restart Snap: `sudo snap restart video-monitor`
4. Install [WebRTSP/RecordStreamer](https://github.com/WebRTSP/RecordStreamer#how-to-install-it-as-snap-package)
5. Configure `WebRTSP/RecordStreamer`:
    * [Without motion detection](https://github.com/WebRTSP/RecordStreamer?tab=readme-ov-file#how-to-use-it-as-streamer-for-cloud-nvr)
    * [With motion detection](https://github.com/WebRTSP/RecordStreamer?tab=readme-ov-file#how-to-use-it-as-streamer-for-cloud-nvr-with-motion-detection)
