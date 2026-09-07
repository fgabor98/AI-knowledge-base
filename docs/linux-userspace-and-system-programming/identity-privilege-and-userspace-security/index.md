---
status: draft
reviewed: false
domain: linux-userspace
difficulty: advanced
last_reviewed: null
---

# Stage 12: Identity, Privilege, And Userspace Security

Userspace security is the set of boundaries that decide who a process is, what it may access, which kernel interfaces it may invoke, and how untrusted data becomes an action. Linux provides many mechanisms, but they are not interchangeable: a UID is an identity, a capability is a permission fragment, a namespace changes what an object means, a cgroup controls resources, and an LSM policy mediates security decisions.

## The model

Reason about a request in layers:

1. **Identity:** real/effective/saved IDs, supplementary groups, and service accounts.
2. **Discretionary access:** mode bits, ownership, umask, ACLs, and filesystem mount options.
3. **Privilege decomposition:** capabilities, bounding sets, ambient inheritance, and `no_new_privs`.
4. **Visibility and resources:** namespaces and cgroup v2.
5. **Mandatory policy:** SELinux, AppArmor, Landlock, seccomp, and audit policy.
6. **Application correctness:** validation, parsing, path resolution, secret handling, and safe failure.

The strongest design uses several independent layers. A sandbox does not repair a confused-deputy bug, and a capability does not make an unsafe parser safe.

## Practical study loop

On a disposable service or test account, inspect:

```sh
id
grep -E '^(Uid|Gid|Groups|Cap[A-Za-z]*|NoNewPrivs|Seccomp):' /proc/self/status
namei -l /path/to/a/file
getfacl /path/to/a/file
capsh --print 2>/dev/null || true
```

For a deployed unit, record the configured identity, namespace options, capability bounding set, syscall policy, writable paths, and cgroup limits. Then test both the intended operation and the denied operation; a security control is only useful when its failure mode is understood.

## Completion checklist

- [ ] Can you explain which credential is used for each access check?
- [ ] Can you drop privileges without retaining an unintended capability or open descriptor?
- [ ] Can you distinguish isolation from authorization and accounting?
- [ ] Can you identify which policy layer produced `EACCES`, `EPERM`, or `SIGSYS`?
- [ ] Can you accept hostile paths, environment variables, files, and protocol frames safely?

## Further reading

- [credentials(7)](https://man7.org/linux/man-pages/man7/credentials.7.html)
- [capabilities(7)](https://man7.org/linux/man-pages/man7/capabilities.7.html)
- [namespaces(7)](https://man7.org/linux/man-pages/man7/namespaces.7.html)
- [cgroups v2](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [seccomp user-space API](https://docs.kernel.org/userspace-api/seccomp_filter.html)

## Stage exercise: define one service's authority

Choose a device reader with a local control socket. List its required device
and state paths, client identities, permitted commands, startup privileges,
steady-state credentials, namespace view and resource limits. Then trace one
request from bytes to authorization to kernel effect.

Test the same packaged service under its configured identity: valid request,
unauthorized request, forbidden file, full queue, missing device and restart.
Capture the returned error and policy evidence. The result should explain
which layer denied each operation and whether any side effect occurred.

## Learning materials

1. [Linux Credentials, Permissions, And ACLs](linux-credentials-permissions-and-acls.md)
2. [Capabilities And Privilege Dropping](capabilities-and-privilege-dropping.md)
3. [Namespaces And Cgroups](namespaces-and-cgroups.md)
4. [Seccomp, LSM, And Service Isolation](seccomp-lsm-and-service-isolation.md)
5. [Secure Userspace Input And Files](secure-userspace-input-and-files.md)
