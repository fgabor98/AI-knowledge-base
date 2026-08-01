---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# Graph Validation And Runtime Diagnosis

Graph bugs often survive compilation because every phandle exists. Effective validation checks structure, reciprocity, interface compatibility, enabled ancestry, driver availability, and negotiated runtime state.

## Validation Layers

| Layer | Proves | Does not prove |
|---|---|---|
| DTS compilation | syntax and resolvable labels | schema or topology correctness |
| `dtbs_check` | conformance to selected schemas | physical wiring or driver support |
| final DTB inspection | actual merged nodes/properties | runtime binding |
| driver/framework logs | registration and assembly behavior | valid electrical signaling |
| subsystem topology | runtime objects, links, formats/modes | signal integrity |
| measurement/capture | clocks, lanes, timing, data | software ownership/lifetime |

Use all necessary layers rather than treating the first success as completion.

## Static Graph Audit

Extract all endpoints from the final source or decompiled DTB. For each endpoint record:

- absolute path and label/source origin
- local device and port number/direction
- endpoint number
- remote endpoint path
- reverse remote endpoint
- bus type, lane map, polarity, timing, and frequency properties
- ancestor availability
- device schema and expected driver

Then detect:

- missing reverse edges
- self-links
- one endpoint referenced by multiple unrelated peers
- enabled-to-disabled connections
- wrong port numbers
- duplicate lane use
- endpoint properties forbidden by the device schema

Graph schema validates generic shape; device schemas constrain actual port counts and properties. Both are necessary.

## Build Validation

Run the repository/kernel workflow appropriate to the board, commonly including:

```sh
make dtbs
make dt_binding_check
make dtbs_check
```

Narrow schema checks during development, then run the full relevant architecture set before submission. Warnings about endpoint cardinality or `reg` length often identify a misplaced cell count rather than the line highlighted.

Do not silence `unevaluatedProperties` failures by moving endpoint properties to arbitrary parent nodes. Determine which schema owns them.

## Runtime Tree First

Firmware, bootloaders, overlays, and fixups can alter the graph. Inspect `/sys/firmware/devicetree/base` or a captured FDT and compare it with the built artifact. Confirm every graph participant is available and its control-bus device exists.

Resolve deferred probe by walking from the missing component to supplies, clocks, PHYs, and its graph neighbor. A remote endpoint can exist while the remote driver's mandatory regulator is absent.

## Subsystem-Specific Evidence

For display:

```sh
modetest -c -p
cat /sys/kernel/debug/dri/0/state
```

For media:

```sh
media-ctl -p
v4l2-ctl --list-devices
```

For audio:

```sh
aplay -l
cat /proc/asound/cards
find /sys/kernel/debug/asoc -maxdepth 3 -type f
```

Runtime object names and indices are not stable identities. Correlate devices through sysfs OF-node links, driver logs, and component names.

## A Diagnostic Ladder

1. identify the missing user-visible object or failed stream
2. inspect the subsystem topology and incomplete link
3. map both runtime objects to DT endpoints
4. verify reciprocal graph edges in the live tree
5. verify both component devices and drivers
6. inspect supplier resources and PM state
7. negotiate the simplest supported mode/format
8. inspect framework error counters and traces
9. measure clocks, lanes, sync, and data at the physical interface
10. increase complexity: rate, resolution, channels, concurrency, suspend

## Schema-Valid But Wrong

Examples include:

- reciprocal endpoints connect two outputs
- lane numbers are valid but do not match PCB routing
- both sides support CSI-2 but have no common link frequency
- a display mode exceeds a bridge PLL limit
- an audio card registers with two clock masters
- all nodes bind but a shared PHY makes two routes mutually exclusive
- an endpoint points through a disabled product-variant component

The senior review question is not “does it compile?” but “can every advertised path operate safely through its full lifecycle?”

## Authoritative References

- [Linux Devicetree schema validation guide](https://docs.kernel.org/devicetree/bindings/writing-schema.html)
- [Linux generic graph binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/graph.yaml)
- [Linux video interface binding schema](https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/media/video-interfaces.yaml)
- [Linux dynamic debug guide](https://docs.kernel.org/admin-guide/dynamic-debug-howto.html)

## Continue

Proceed to the [Complex Pipeline Integration Lab](complex-pipeline-integration-lab.md).
