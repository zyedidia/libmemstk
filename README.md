This library can use three different backend implementations: `mprotect`,
`userfaultfd`, and `dune`. The backend is selected in the Makefile with the
`MPROT` variable.

# mprotect

Uses the `mprotect` system call and a sigsegv signal handler to handle faults. This version
should work on any version of Linux.

# userfaultfd

This implementation uses the modern `userfaultfd` Linux API to handle page
faults. The `userfaultfd` system call requires sudo by default. Alternatively,
you may enable unprivileged use of userfaultfd with

```
sysctl -w vm.unprivileged_userfaultfd=1
```

The userfaultfd version requires Linux kernel 5.10+. If you do not have a modern
kernel, you can use the Vagrant virtual machine to run one. You must have vagrant
and VirtualBox installed.

```
$ vagrant up
...
$ vagrant ssh
```

# dune

This implementation uses [Dune](https://github.com/project-dune/dune) to
virtualize the process and allow it to handle its own page faults.
