# Lab 4: Software Platform Driver

This checkpoint separates a platform device from its platform driver. The
helper module creates a software-only device, so `probe()` and `remove()` can
be observed without real hardware. The driver keeps the character-device
interface from Lab 3 to make the new device-model boundary visible.

Build and load the driver first or second; the platform bus will bind them when
both are present:

```sh
make
sudo insmod ./platform_demo.ko
sudo insmod ./platform_demo_device.ko
find /sys/bus/platform/devices -maxdepth 1 -name 'lab_platform_demo*'
readlink /sys/bus/platform/devices/lab_platform_demo.0/driver
printf 'platform hello\n' | sudo tee /dev/platform_demo0 >/dev/null
cat /dev/platform_demo0
```

Unload the device helper before unloading the driver:

```sh
sudo rmmod platform_demo_device
sudo rmmod platform_demo
```

Expected evidence includes `probe`/`remove` messages and a driver symlink in
sysfs. The helper is only a lab device; real platform devices normally come
from firmware or board code.

