# crash-report.gdb - print a minimal, repeatable crash report.
#
# usage: gdb -q -batch -x crash-report.gdb -ex crash_report PROGRAM CORE
set pagination off
set print pretty off
set disassembly-flavor intel

define crash_report
  printf "== signal and location\n"
  frame 0
  printf "== backtrace\n"
  bt
  printf "== registers\n"
  info registers rip rsp rbp
  printf "== faulting instruction\n"
  x/i $pc
  printf "== locals, every frame\n"
  frame apply all -s info locals
end
document crash_report
Print the crash location, backtrace, key registers, faulting instruction,
and the locals of every frame. Works on a live crash or on a core file.
end
