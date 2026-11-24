# Lab 15: Virtual Input Device

This checkpoint registers an input device without hardware. Writing to the
`press` module parameter reports key presses; the parameter uses
`module_param_cb()`, so every write runs a validating callback.

`KEY_F24` is used because desktops normally ignore it.

```sh
make
sudo insmod ./lab_input.ko
grep -A4 lab-virtual-key /proc/bus/input/devices
sudo evtest   # choose lab-virtual-key, leave it running
```

In another terminal:

```sh
echo 2 | sudo tee /sys/module/lab_input/parameters/press
echo 99 | sudo tee /sys/module/lab_input/parameters/press   # ERANGE
sudo rmmod lab_input
```

`evtest` must show press, `SYN_REPORT`, release, `SYN_REPORT` for each count,
and end when the device disappears. Delete one `input_sync()` and compare.
