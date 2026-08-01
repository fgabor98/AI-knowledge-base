---
status: draft
reviewed: false
domain: device-tree
difficulty: advanced
last_reviewed: null
---

# FIT Authenticated Selection And Key Policy

A digest detects changed bytes when compared with a trusted expected digest. A signature connects covered bytes to an authorized private key. A secure boot policy must additionally authenticate which components are selected together, require verification, protect the public key, and reject unsafe fallback paths.

## Understand FIT Roles

A Flattened Image Tree can contain:

- image nodes for kernels, DTBs, ramdisks, firmware, and other payloads
- hash nodes describing payload digests
- signature nodes
- configuration nodes that select a set of images
- a default or explicitly requested configuration

The FIT is itself encoded as an FDT, but its schema describes a boot container rather than hardware.

## Hashes Are Not Authorization

If an attacker can replace both a payload and its adjacent hash, a successful hash check proves only internal consistency. A trusted external digest or signature is required to establish authorization.

Likewise, signing each image independently may allow an attacker to combine an old but valid DTB with a new valid kernel or alter which configuration is selected. Authenticate relationships, not only parts.

## Prefer Signed Configurations

This schematic ITS binds a kernel, DTB, and ramdisk through one signed configuration:

```dts
/dts-v1/;

/ {
    description = "AXC200 release 42";

    images {
        kernel-1 {
            data = /incbin/("Image");
            type = "kernel";
            arch = "arm64";
            os = "linux";
            compression = "none";
            hash-1 { algo = "sha256"; };
        };

        fdt-1 {
            data = /incbin/("axc200-revb.dtb");
            type = "flat_dt";
            arch = "arm64";
            compression = "none";
            hash-1 { algo = "sha256"; };
        };

        ramdisk-1 {
            data = /incbin/("initramfs.cpio.gz");
            type = "ramdisk";
            arch = "arm64";
            os = "linux";
            compression = "gzip";
            hash-1 { algo = "sha256"; };
        };
    };

    configurations {
        default = "conf-axc200-revb-r42";

        conf-axc200-revb-r42 {
            kernel = "kernel-1";
            fdt = "fdt-1";
            ramdisk = "ramdisk-1";
            signature-1 {
                algo = "sha256,rsa2048";
                key-name-hint = "prod-2026";
                sign-images = "kernel", "fdt", "ramdisk";
            };
        };
    };
};
```

Exact supported properties and algorithms depend on the U-Boot release and configuration. Treat the target bootloader's documentation and tests as authoritative.

## Establish A Trusted Key

U-Boot FIT verification commonly stores public-key material under `/signature` in U-Boot's control FDT. That control FDT must itself be authenticated by an earlier stage or protected storage. Verify:

- the expected key node exists in the shipped verifier
- the key has the intended `required` policy (`"conf"` for required configuration verification when that design is used)
- `required-mode` semantics match key-rotation policy when multiple required keys exist
- unsupported algorithms fail closed
- debug commands cannot replace the key or weaken the policy in production

`key-name-hint` assists key lookup; a name is not a security identity.

## Sign And Inspect

An illustrative software-signing flow is:

```bash
mkimage -f release.its unsigned.itb
mkimage -F -k keys -K u-boot-control.dtb -r unsigned.itb
mv unsigned.itb release.itb

mkimage -l release.itb
sha256sum release.itb u-boot-control.dtb
```

Do not place production private keys in a normal build workspace. Use a signing service, hardware-backed key, controlled identity, audit log, and release authorization. Build workers should normally produce digests or unsigned candidates; the signer should verify policy and return a signature or signed artifact.

## Authenticate Selection Inputs

Review every selector:

- FIT default configuration
- explicit `bootm` configuration name
- board-compatible best-match behavior
- bootloader environment variables
- EEPROM/strap-derived board revision
- extra overlay/feature configurations
- slot and recovery choice

The selected configuration should be signed and constrained to the detected product identity. If an unauthenticated environment can select arbitrary separately signed subimages, valid cryptography can still produce an unauthorized combination.

## Close Bypass Paths

Test attempts to boot:

- an unsigned FIT
- a FIT signed by an unknown key
- a signed FIT with one payload byte changed
- a valid payload under a modified configuration
- a valid old DTB paired with a current kernel
- an unsigned legacy image
- a raw kernel plus external DTB outside the verified container
- a valid configuration for another board
- an overlay appended outside the signed configuration

Expected behavior must be rejection or entry into authenticated recovery—not a permissive alternate command.

## Signature Is Not Freshness

A correctly signed old release remains authentic. Anti-rollback needs protected monotonic state such as a fuse, secure element, TPM NV counter, replay-protected storage, or platform update framework. Bind a signed security version to the release and compare it against protected minimum state before activation or boot.

Never advance the minimum version until the new slot has proved bootable and the recovery design remains valid. The exact transaction is platform-specific.

## Key Policy

Document:

- offline root and online/intermediate release keys
- production, development, factory, and recovery trust separation
- algorithms and deprecation dates
- who may request and approve signatures
- artifact types and product families allowed per key
- revocation or minimum-key-version mechanism
- overlap window for rotation
- disaster recovery for signer loss or compromise
- field evidence that identifies the accepted key

## Verification Checklist

```text
[ ] verifier is authenticated
[ ] public key is anchored outside attacker-controlled image
[ ] verification is required, not optional
[ ] signed configuration covers every boot-critical payload
[ ] selected board/slot/configuration is policy-constrained
[ ] legacy/raw/interactive bypass paths are closed
[ ] old signed releases face protected rollback policy
[ ] failure enters authenticated recovery
[ ] negative tests run on the actual production configuration
```

## Further Reading

- [U-Boot FIT Signature Verification](https://docs.u-boot.org/en/stable/usage/fit/signature.html)
- [U-Boot Verified Boot](https://docs.u-boot.org/en/stable/usage/fit/verified-boot.html)
- [U-Boot FIT format](https://docs.u-boot.org/en/stable/usage/fit/source_file_format.html)
- [Mutation, Overlay, And Fixup Chain Of Custody](mutation-overlay-and-fixup-chain-of-custody.md)
