# demo_app

## Bridge multimedia demo

Run bridge status check for multimedia/GPU related modules:

```bash
./build/EngineBuilder run demo_app/bridge_multimedia.js
```

Run the broader module-registry smoke check used after refactors:

```bash
./out/build/default/EngineBuilder run demo_app/modules_smoke.js
```

This script checks these modules:

- OpenCV
- CUDA
- CUDNN
- Skia
- FFmpeg
- ANGLE

The module smoke script also verifies:

- Container
- System
- RuntimeLive
- Doctor
- VTK
