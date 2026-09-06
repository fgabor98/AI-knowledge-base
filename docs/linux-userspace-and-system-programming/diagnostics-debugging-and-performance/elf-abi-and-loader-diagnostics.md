# ELF, ABI, And Loader Diagnostics

When a program fails before `main`, inspect the ELF and dynamic loader before debugging application logic.

```sh
file ./app
readelf -h ./app
readelf -l ./app | grep 'Requesting program interpreter'
readelf -d ./app
readelf --version-info ./app
```

Check architecture, endianness, ABI, ELF class, interpreter path, needed libraries, RPATH/RUNPATH, symbol versions, and executable stack/text-relocation properties. A binary built for the wrong target can produce `Exec format error`; a missing interpreter or library commonly produces `No such file or directory` even when the application path exists.

`ldd` may execute code for untrusted binaries on some systems; prefer `readelf` and the loader's diagnostic mode for unknown artifacts. Use `LD_DEBUG` only in a controlled environment because it is verbose and may expose paths or secrets.

Compare the target's loader and library set with the build sysroot. ABI compatibility includes calling convention, data-model widths, structure layout, symbol versions, thread-local storage, and kernel/libc assumptions—not just CPU architecture. Record the exact package provenance so a loader failure is reproducible.
