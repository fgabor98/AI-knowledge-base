# Lab 6: GPIO Consumer

This checkpoint requests a GPIO by its role, `reset`, through the descriptor
API. The supplied overlay deliberately omits the board-specific GPIO phandle
so it can be compiled on any target and still demonstrate the optional-resource
path.

Build it with a Device Tree compiler installed:

```sh
make
```

Apply `gpio-demo-overlay.dtbo`, then load the module:

```sh
sudo insmod ./gpio_demo.ko
dmesg | tail -n 30
```

For a real GPIO test, add a property appropriate for the target's binding and
controller, for example:

```dts
reset-gpios = <&gpio1 3 GPIO_ACTIVE_LOW>;
```

Do not copy that phandle or GPIO number blindly to another board. After
applying the board-specific overlay, inspect the request with:

```sh
cat /sys/kernel/debug/gpio
```

The driver uses `devm_gpiod_get_optional()` and
`gpiod_set_value_cansleep()`, so active-low interpretation stays in the GPIO
descriptor rather than being manually inverted in the driver.

