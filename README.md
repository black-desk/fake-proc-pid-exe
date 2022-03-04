# 一种伪造 `/proc/[pid]/exe` 的方法

论为什么不应该在 Dbus 服务或者是通过 local socket 进行进程间通信的服务中仅使用
`/proc/[pid]/exe` 来鉴定方法的调用者身份.

本项目是一个伪造 `/proc/[pid]/exe` 的 demo. 以下会简单解释一下这种伪造是如何做到
的.

## 构建

依赖 libfmt 和 dbus-1, 请自行安装.

```bash
meson setup build
cd build
meson compile
```

## 运行

在终端 1 中:

```bash
./attack server # 运行一个 dbus 服务.
```

在另一个终端 2 里:

```bash
./attack run dde-control-center
```

你会看到类似这样的输出:

```
# 终端 1
server: get caller exe="/usr/bin/dde-control-center"

# 终端 2
Do a clone syscall to create new user namespace and mnt namespace
start waiting cloned procress to exit [pid=298114]
cloned: Now I have CAP_SYS_ADMIN in this new user namespace. So I can manipulate the mount namespace I just created
cloned: copy /proc/self/exe to "....../build/dde-control-center", then chmod+x
cloned: mount pwd[....../build] to /usr/bin
cloned: set uid mapping to make dbus-daemon happy
cloned: now I exec /usr/bin/dde-control-center
dbus call start
dbus call end
```

## 发生了什么

结合源码, 可以看到现象是 DBus 服务中接到 method_call 消息后, 向
`org.freedesktop.DBus` 询问调用者的 PID, 通过 PID 去 procfs 中查询该进程的二进制
文件具体在什么地方. 按理来说, 如果这个程序在 `/usr/bin` 下面, 这个目录又只有
root 才可以写入, 那么这个程序就是可以被信任的.

但是实际上这里发出消息的并不是 `/usr/bin/dde-control-center`, 而是本项目中自己构
建出来的二进制. 如果你完全按照上述步骤进行操作, 可以看到在 `build` 目录下, 有一
个名叫 `dde-control-center` 的程序, 它是 `attack` 的拷贝.

在运行 `attack run` 命令时, `attack` 进行了一次 `clone`, 创建了新的 user
namespace 以及 mount namespace. 在一个 user namespace 中拥有 `CAP_SYS_ADMIN` 就
可以操作属于这个 user namespace 的 mount namespace, 也就是进行 mount 操作.

我们在 `src/cmd/run.cpp:prepare` 函数中, 将 cwd `mount` 到了 `/usr/bin`, 之后
`exec` 了 `/usr/bin/dde-control-center`. 但是这里实际上运行的程序是宿主机上的
`build/dde-control-center`. 此时宿主机上的 `/proc/` 中可以看到这个在子 user
namespace 中运行着的进程, 并且它的 `exe` 是被 "篡改" 过的状态.
