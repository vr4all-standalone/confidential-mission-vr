# Game zoom regression checks

Run with Python3 and an existing MinGW64 toolchain at D:/msys64/mingw64/bin. Use the source root and an output directory as the two positional arguments.

python -B tests/game-zoom-test.py ROOT OUTPUT/policy
python -B tests/game-zoom-gl-test.py ROOT OUTPUT/gl
python -B tests/game-zoom-integration-test.py ROOT OUTPUT/integration
python -B tests/game-zoom-ui-test.py ROOT OUTPUT/ui

The native camera, gun-handoff, PCVR pad, XR lifecycle and CPU raycast fixtures compile actual production blocks with controlled runtime inputs. GLSL uses a hidden WGL context and actual production shader raster/transform feedback. Menu and feedback tests execute actual menu cases and drawing policy with texture/draw boundaries stubbed; text legibility and physical controller reach still require headset testing.

Native reset reference regression: python -B tests/native-camera-reference-test.py ROOT OUTPUT/native-reference [OWNED_RAM_DUMP]
The optional owned dump verifies guards against actual code; no game data is included in source. CM uses40degrees from the guarded native reset, not vr.Focal.
