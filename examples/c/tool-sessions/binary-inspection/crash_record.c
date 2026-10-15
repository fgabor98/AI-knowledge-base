/*
 * Minimal field crash record: on SIGILL/SIGSEGV, print the faulting address
 * relative to the executable's load base, plus its GNU build ID. Both are
 * looked up once at start-up, so the handler itself only formats numbers and
 * calls write() and _exit(). Symbolization happens offline.
 */
#define _GNU_SOURCE
#include <elf.h>
#include <link.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

static char build_id[41];
static uintptr_t load_base;

static void put(const char *text)
{
    (void)!write(STDERR_FILENO, text, strlen(text));
}

static void put_hex(uintptr_t value)
{
    char digits[2 + 2 * sizeof value + 1];
    size_t position = sizeof digits - 1;

    digits[position] = '\0';
    do {
        digits[--position] = "0123456789abcdef"[value & 0xfu];
        value >>= 4;
    } while (value != 0);
    digits[--position] = 'x';
    digits[--position] = '0';
    put(&digits[position]);
}

static int find_build_id(struct dl_phdr_info *info, size_t size, void *data)
{
    (void)size;
    (void)data;
    if (info->dlpi_name[0] != '\0') {       /* only the main executable */
        return 0;
    }
    load_base = (uintptr_t)info->dlpi_addr;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *phdr = &info->dlpi_phdr[i];
        const unsigned char *note = (const unsigned char *)(info->dlpi_addr + phdr->p_vaddr);
        size_t offset = 0;

        if (phdr->p_type != PT_NOTE) {
            continue;
        }
        while (offset + sizeof(ElfW(Nhdr)) <= phdr->p_memsz) {
            const ElfW(Nhdr) *header = (const ElfW(Nhdr) *)(note + offset);
            const unsigned char *name = note + offset + sizeof *header;
            const unsigned char *desc = name + ((header->n_namesz + 3u) & ~3u);

            if (header->n_type == NT_GNU_BUILD_ID && header->n_namesz == 4 &&
                memcmp(name, "GNU", 4) == 0 && header->n_descsz <= 20) {
                for (size_t b = 0; b < header->n_descsz; b++) {
                    build_id[2 * b] = "0123456789abcdef"[desc[b] >> 4];
                    build_id[2 * b + 1] = "0123456789abcdef"[desc[b] & 0xfu];
                }
                return 1;
            }
            offset += sizeof *header + ((header->n_namesz + 3u) & ~3u) +
                      ((header->n_descsz + 3u) & ~3u);
        }
    }
    return 0;
}

static void on_fatal_signal(int signal_number, siginfo_t *info, void *context)
{
    const ucontext_t *uc = context;
    uintptr_t pc = (uintptr_t)uc->uc_mcontext.gregs[REG_RIP];

    (void)info;
    put("crash: signal ");
    put_hex((uintptr_t)signal_number);
    put(" build-id ");
    put(build_id);
    put(" module-offset ");
    put_hex(pc - load_base);
    put("\n");
    _exit(128 + signal_number);
}

void crash_record_install(void)
{
    struct sigaction action;

    dl_iterate_phdr(find_build_id, NULL);   /* build ID and load base, once */
    memset(&action, 0, sizeof action);
    action.sa_sigaction = on_fatal_signal;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGILL, &action, NULL);
    sigaction(SIGSEGV, &action, NULL);
}
