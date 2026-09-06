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
cat /proc/self/status | grep -E '^(Uid|Gid|Groups|Cap|NoNewPrivs|Seccomp):'
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
