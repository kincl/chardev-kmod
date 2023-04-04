# chardev-kmod

Incredibly simple kernel module that creates a character device at `/dev/hello`
and can be interacted with by using `cat` or `echo`

## Build and install kernel module

```
dnf install kernel-devel kernel-headers
```

```
cd chardev-kmod
make
insmod chardev.ko
```