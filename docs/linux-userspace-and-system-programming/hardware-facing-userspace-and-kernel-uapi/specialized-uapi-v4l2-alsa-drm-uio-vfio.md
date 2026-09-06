---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Specialized UAPI: V4L2, ALSA, DRM, UIO, And VFIO

## What problem does this solve?

Major subsystems have rich state machines and buffer protocols. A generic “open,
read, close” mental model is insufficient; use their capability queries, negotiated
formats, event queues, and ownership rules.

## V4L2

Video4Linux2 devices negotiate format, size, field, colorspace, and buffer method.
Streaming I/O commonly follows request/queue, stream-on, dequeue, validate, process,
requeue, and stream-off. Handle sequence gaps, timestamps, buffer starvation,
disconnect, and format changes. Do not assume a camera index is stable.

## ALSA

ALSA PCM devices expose hardware/software parameters, sample format, rate, channels,
period, buffer, and start/stop/xrun state. Configure explicitly and handle XRUN
recovery. A successful write can mean frames queued, not audible output. Mixer/control
names and availability are card/profile dependent.

## DRM/KMS

DRM/KMS uses modes, connectors, planes, framebuffers, atomic commits, and events.
Resource ownership and master/client rules matter. Validate formats/strides and use
atomic commit completion events rather than assuming a submit returned means scanout
completed.

## UIO and VFIO

UIO can expose device memory and interrupts to userspace but leaves significant driver
policy to the application. VFIO provides an IOMMU-mediated device assignment model
with container/group/device ownership and DMA restrictions. Neither justifies mapping
arbitrary physical memory. Review isolation, reset, interrupt, and device ownership
before deployment.

## Common mistakes

- Treating subsystem indices/names as stable hardware identity.
- Skipping format/capability negotiation.
- Reusing a V4L2/ALSA/DRM buffer before ownership returns.
- Ignoring xrun, sequence, event, mode, and disconnect errors.
- Using UIO/VFIO without IOMMU, reset, privilege, and DMA analysis.

## Debugging checklist

- Record subsystem/card/device identity, capabilities, negotiated format, buffers,
  events, and generation.
- Use subsystem tools (`v4l2-ctl`, `alsactl`, `modetest`) where available.
- Test buffer starvation, xrun, hotplug, mode change, reset, and peer/process death.
- Correlate events and timestamps with kernel logs and external output.
- Verify memory/cache/DMA ownership and service permissions.

## Related topics

- [Stage 10: Hardware-Facing Userspace And Kernel UAPI](index.md)
- [poll, mmap, And Device Events](poll-mmap-and-device-events.md)
- [Standard UAPI Operation Patterns](standard-uapi-operation-patterns.md)

## References

- [V4L2 userspace API](https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/v4l2.html)
- [ALSA kernel documentation](https://www.kernel.org/doc/html/latest/sound/index.html)
- [DRM userspace API](https://www.kernel.org/doc/html/latest/gpu/drm-uapi.html)
- [UIO HOWTO](https://www.kernel.org/doc/html/latest/driver-api/uio-howto.html)
- [VFIO documentation](https://www.kernel.org/doc/html/latest/driver-api/vfio.html)
