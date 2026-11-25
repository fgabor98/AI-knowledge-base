/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/* Shared by the module and by userspace, like a real UAPI header. */
#ifndef LAB_IOCTL_H
#define LAB_IOCTL_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define LAB_IOCTL_ABI_VERSION	1
#define LAB_IOCTL_FLAG_FAST	(1U << 0)
#define LAB_IOCTL_FLAGS_ALL	LAB_IOCTL_FLAG_FAST

struct lab_ioctl_config {
	__u32 version;
	__u32 flags;
	__u32 rate_hz;		/* 1..1000 */
	__u32 reserved[5];	/* must be zero */
};

struct lab_ioctl_status {
	__u32 version;
	__u32 flags;
	__u32 rate_hz;
	__u32 reserved;
	__u64 set_count;
};

#define LAB_IOCTL_MAGIC		'K'
/* Userspace writes the structure to the kernel. */
#define LAB_IOCTL_SET_CONFIG	_IOW(LAB_IOCTL_MAGIC, 0x80, struct lab_ioctl_config)
/* Userspace reads the structure from the kernel. */
#define LAB_IOCTL_GET_STATUS	_IOR(LAB_IOCTL_MAGIC, 0x81, struct lab_ioctl_status)
#define LAB_IOCTL_RESET		_IO(LAB_IOCTL_MAGIC, 0x82)

#endif
