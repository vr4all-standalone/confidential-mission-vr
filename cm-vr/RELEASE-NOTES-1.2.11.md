# Confidential Mission1.2.11 — camera reference and grip leader correction

- Correct Game zoom to use CM's actual native40degree camera reset, with guarded instructions and literal, independently from first-room presentation calibration. Normal40degree views return to1x; native camera animation zooms in/out. The prior calibration wrongly added about1.63x at the normal lens.
- Aim and rendering still share the same per-frame factor; native HUD, held models, depth, head tracking and audio keep their existing behavior.
- Both How to Play grip leaders now terminate at a verified point on this bundled controller's actual squeeze surface. Updated metadata and a narrow bundled-model fallback cover existing cached controller metadata; explicit custom points remain in effect.
- Grip toggle, one-second purple feedback, menu row and session-only Off default remain.

Fresh Quest/PCVR/source candidates are local and unpublished. Physical acceptance of this correction is separate from software/native camera probes.
