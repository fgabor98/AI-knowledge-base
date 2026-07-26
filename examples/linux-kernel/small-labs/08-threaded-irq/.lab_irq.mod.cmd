savedcmd_lab_irq.mod := printf '%s\n'   lab_irq.o | awk '!x[$$0]++ { print("./"$$0) }' > lab_irq.mod
