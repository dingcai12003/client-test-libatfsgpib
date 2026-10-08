# client-test

`client-test` 是一个与仓库现有 `client/` 完全独立的 ATFS GPIB 验证程序，用于在 Tester 机台上确认 ATFS 自带 `libatfsgpib.so` 能否加载、打开配置设备、访问目标 GPIB 设备并完成安全命令收发。

它不依赖 `client/`、`proto/`、`server/` 的任何源码、配置或构建系统。程序采用纯 C/POSIX + `dlopen` 动态加载现场配置的共享库，运行时解析 `UTSC_Gpib_*` API，避免 Qt 和现代 C++ 运行时依赖。

## 目录内容

- `src/`
  - 独立应用源码，按 CLI、配置、GPIB runtime、命令发送和交互菜单拆分
- `include/`
  - 公共类型、常量和模块函数声明
- `Makefile`
  - Linux 构建文件
- `config/`
  - 独立配置文件
- `atfs-env.sh`
  - ATFS 运行环境默认值和运行前检查函数
- `run.sh`
  - 便捷启动脚本

## 适用边界

- 目标机台：`SUSE 9.2`
- 目标用途：验证 ATFS GPIB 库和现场 `gpib.conf` 设备配置
- 交互方式：文本菜单，菜单项等价于几个测试按钮
- 固定预置命令：仅保留安全查询，不包含可能驱动机台动作的固定命令

## 构建说明

当前 Windows 开发主机不是最终 Linux 产物构建环境。  
请在与 `SUSE 9.2` 足够接近的 Linux 构建环境中编译，再将二进制和配置拷贝到机台运行。

构建命令：

```sh
make
```

默认构建面向 ATFS i386 现场环境，会使用 `-m32`，并把 `/opt/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfshn.a` 链入主程序，用于向 `dlopen(libatfsgpib.so)` 导出 `UTHN_Handle_FindArea` 等内部依赖符号。

如果 ATFS 安装路径不同，可覆盖 Makefile 变量：

```sh
make ATFSROOT=/custom/ATFS ATFSSYS=ATFSsys-4.05F1
```

If the ATFS headers are installed separately, override `ATFS_INCLUDE_DIR=/path/to/ATFSsys-4.05F1/include`.

构建后必须验证主程序已导出 `UTHN_Handle_FindArea`：

```sh
make verify-atfs-symbols
```

标准重建流程：

```sh
make clean
make
make verify-atfs-symbols
```

程序仍会链接 `libdl` 和 `librt`。在老 SUSE/glibc 环境中，ATFS 库可能需要通过 `librt.so.1` 解析 `clock_gettime`。

清理命令：

```sh
make clean
```

构建产物：

- `gpib-test`

## 部署说明

建议把以下文件拷贝到 Tester 机台上的独立目录，例如 `/opt/gpib-test/`：

- `gpib-test`
- `config/`
- `atfs-env.sh`
- `run.sh`
- `README.md`

机台无开发环境时，只负责运行，不负责现场编译。

## 配置说明

配置文件格式为 `key=value`，支持注释行 `# ...`。

默认样例：

```ini
library_bind_mode=now
preload_libraries=
device_name=PH1
read_buffer_size=2048
wait_srq_timeout_us=5000000
use_rm_lock=0
```

字段说明：

- `library_path`
  - 可选覆盖项；未配置时优先使用 `GPIB_LIBRARY_PATH`，再由 `ATFSSYSTEM/ATFSSYS` 或 `ATFSROOT/ATFSARCH/ATFSOS/ATFSSYS` 拼出 `libatfsgpib.so`
- `library_bind_mode`
  - `lazy` 表示延迟解析 ATFS 库符号，用于验证 `UTHN_Handle_FindArea` 是否只是非当前路径依赖；`now` 表示加载时立即解析全部符号，适合依赖库补齐后的严格回归验证；未配置时程序默认 `now`
- `preload_libraries`
  - 在 `library_path` 之前预加载的 ATFS 依赖库列表，多个库用英文逗号分隔，空值表示不预加载额外库
- `device_name`
  - ATFS `gpib.conf` 中 `[device]` 段的设备名，当前现场已确认 `PH1 0 1 TMO=5`
- `read_buffer_size`
  - `UTSC_Gpib_RecvData` 读取缓冲区大小
- `wait_srq_timeout_us`
  - 诊断菜单 `12` 调用 `UTSC_Gpib_WaitSrq` 的超时时间，单位微秒，默认 `5000000`
- `use_rm_lock`
  - 是否在 `UTSC_Gpib_Open` 前后调用 `UTSC_Rm_Lock/UTSC_Rm_Unlock`，默认 `0`；需要对齐 ATFS 官方示例时设置为 `1`

## 运行方式

`run.sh` 会先加载 `atfs-env.sh`，默认导出以下 ATFS 运行环境：

```sh
ATFSROOT=/opt/ATFS
ATFSARCH=i386
ATFSOS=linux
ATFSSYS=ATFSsys-4.05F1
ATFSSYSTEM=/opt/ATFS/i386/linux
ATFSVAROPT=/var/opt
ATFS_LIB_DIR=/opt/ATFS/i386/linux/ATFSsys-4.05F1/lib
GPIB_LIBRARY_PATH=/opt/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfsgpib.so
LD_LIBRARY_PATH=/opt/ATFS/i386/linux/ATFSsys-4.05F1/lib:$LD_LIBRARY_PATH
```

如现场安装路径不同，可在运行前覆盖对应变量：

```sh
ATFSROOT=/custom/ATFS ATFSVAROPT=/custom/var/opt ./run.sh probe
```

默认库路径由 `atfs-env.sh` 拼出；如需覆盖单个共享库路径，可设置 `GPIB_LIBRARY_PATH`。也可以复制一份配置后通过 `GPIB_TEST_CONFIG` 指定显式 `library_path`：

```sh
GPIB_LIBRARY_PATH=/custom/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfsgpib.so ./run.sh probe
```

```sh
GPIB_TEST_CONFIG=./config/gpib-test.custom.conf ./run.sh probe
```

默认会检查 `libatfsgpib.so`、`gpib_system.conf` 和现场 `gpib.conf` 是否存在。只想跳过路径检查时可临时设置：

```sh
ATFS_SKIP_CHECKS=1 ./run.sh probe
```

如果系统存在 `nm`，`run.sh` 还会检查当前 `gpib-test` 是否导出 `UTHN_Handle_FindArea`。检查失败时，说明需要重新执行 `make clean && make && make verify-atfs-symbols`。只想临时跳过符号检查时可设置：

```sh
ATFS_SKIP_SYMBOL_CHECK=1 ./run.sh probe
```

需要由脚本先尝试启动 GPIB 服务时，使用：

```sh
ATFS_START_GPIB=1 ./run.sh probe
```

排查环境变量时，可打印当前生效的 ATFS 环境：

```sh
ATFS_PRINT_ENV=1 ./run.sh probe
```

交互模式：

```sh
./gpib-test --config ./config/gpib-test.conf
```

或者：

```sh
./run.sh
```

Explicit old-path usage such as `./gpib-test --config ./gpib-test.conf probe` remains compatible when the old file is absent; the program falls back to `./config/gpib-test.conf`.

交互菜单：

1. `Probe Library And Device - 探测库加载和设备打开`
2. `Send ms - 查询 Prober 总状态`
3. `Send B - 查询 Prober ID`
4. `Send V - 查询 Lot Number`
5. `Send b - 查询 Wafer ID`
6. `Send f - 查询 Chuck Temperature`
7. `Send w - 查询 Wafer Status`
8. `Send x - 查询 Cassette Status`
9. `Send r - 查询 Hot-chuck Status`
10. `Send Custom Command - 自定义命令，仅读取 STB`
11. `Send Custom Command And Read Payload - 自定义命令并读取返回内容`
12. `Send Custom Command - Wait SRQ, Read STB All And Payload - 自定义命令，按 WaitSrq -> RecvStbAll -> RecvData 诊断流程读取`
13. `Clear Device (UTSC_Gpib_ClearDevice) - 清设备`
14. `Show Current Config - 显示当前配置`
15. `Try RM Lock Only (UTSC_Rm_Trylock) - 只尝试资源锁并立即释放`
16. `Exit - 退出`

非交互模式：

```sh
./gpib-test --config ./config/gpib-test.conf probe
./gpib-test --config ./config/gpib-test.conf ms
./gpib-test --config ./config/gpib-test.conf trylock
./gpib-test --config ./config/gpib-test.conf send --command "ms" --read-payload
./gpib-test --config ./config/gpib-test.conf send --command "V"
./gpib-test --config ./config/gpib-test.conf send --command "X" --payload "123"
```

## 输出内容

程序统一输出：

- 当前连接配置
- 发送内容的 ASCII
- 发送内容的 HEX
- 返回的 `STB`
- 返回的 `payload`
- 失败阶段说明和 ATFS 错误信息

固定查询菜单 `2-9` 会在发送命令后直接读取 payload，不再先读取 STB。自定义菜单 `10` 仍用于只读 STB；自定义菜单 `11` 用于直接读取 payload；自定义菜单 `12` 用于发送后按 `WaitSrq -> RecvStbAll -> RecvData` 流程诊断。

非交互 `send --read-payload` 同样表示发送后直接读取 payload；未指定 `--read-payload` 时只读取 STB。

## 推荐验证步骤

1. 先确认现场共享库存在：`find / -iname '*gpib*.so*' 2>/dev/null`
2. 再确认库里导出了必需的 ATFS GPIB 符号：
   `nm -D /opt/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfsgpib.so 2>/dev/null | grep 'UTSC_Gpib_Open\|UTSC_Gpib_SendData\|UTSC_Gpib_RecvData\|UTSC_Gpib_RecvStb\|UTSC_Gpib_Close'`
   如果配置 `use_rm_lock=1`，还需要确认：
   `nm -D /opt/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfsgpib.so 2>/dev/null | grep 'UTSC_Rm_Lock\|UTSC_Rm_Unlock\|UTSC_Rm_GetErrorMessage'`
   如果需要资源锁诊断，还需要确认：
   `nm -D /opt/ATFS/i386/linux/ATFSsys-4.05F1/lib/libatfsgpib.so 2>/dev/null | grep 'UTSC_Rm_TryLock\|UTSC_Rm_Unlock\|UTSC_Rm_GetErrorMessage'`
3. 确认现场设备名：`cat /var/opt/ATFS/ATFSsys-4.05F1/gpib.conf`
4. 如需排查资源锁占用，先执行 `trylock`
5. 执行 `probe`
6. 再执行 `ms`
7. 再执行 `B`、`b`、`f`、`w`、`x` 等安全查询命令
8. 如需 SRQ 诊断，执行菜单 `12. Send Custom Command - Wait SRQ, Read STB All And Payload`
9. 如需显式清设备，再执行菜单 `13. Clear Device (UTSC_Gpib_ClearDevice)`
10. 最后再用自定义命令补充验证

示例：

```sh
./gpib-test --config ./config/gpib-test.conf probe
./gpib-test --config ./config/gpib-test.conf trylock
./gpib-test --config ./config/gpib-test.conf ms
./gpib-test --config ./config/gpib-test.conf send --command "V" --read-payload
```

## 常见失败点

- `library_path` 指向的共享库不存在或路径错误
- 共享库缺少 `UTSC_Gpib_Open`、`UTSC_Gpib_SendData`、`UTSC_Gpib_RecvData`、`UTSC_Gpib_RecvStb`、`UTSC_Gpib_Close`
- 配置 `use_rm_lock=1` 时，共享库缺少 `UTSC_Rm_Lock`、`UTSC_Rm_Unlock` 或 `UTSC_Rm_GetErrorMessage`
- 执行 `trylock` 时，共享库缺少 `UTSC_Rm_TryLock`、`UTSC_Rm_Unlock` 或 `UTSC_Rm_GetErrorMessage`
- ATFS 内部依赖库未预加载，导致 `undefined symbol: ...`
- `device_name` 与 `/var/opt/ATFS/ATFSsys-4.05F1/gpib.conf` 中 `[device]` 名称不一致
- ATFS GPIB 服务或驱动未运行
- 写命令失败
- 读 `STB` 失败
- 读 `payload` 失败

如果固定查询命令已经成功写出，但读 payload 超时，优先用菜单 `12` 验证是否能收到 SRQ、是否能通过 `RecvStbAll` 取到状态队列，再判断问题停在 SRQ/STB 阶段还是 `RecvData` 阶段。

## Probe 崩溃排查

`probe` 会输出阶段日志并立即刷新到终端。如果仍然出现 `Segmentation fault (core dumped)`，记录崩溃前最后一行：

- 停在 `load-library: before dlopen librt.so.1`：可能是 `librt` 预加载或动态链接器问题
- 出现 `load-library: bind mode=lazy`：当前使用延迟符号解析；如果后续在 `UTSC_Gpib_Open` 或收发阶段仍报 `undefined symbol`，说明该路径实际需要对应依赖库
- 出现 `load-library: dlopen librt.so.1 failed: ...`：程序会继续尝试加载 ATFS 库；如果随后报 `clock_gettime`，需要检查系统 `librt`
- 停在 `preload-library: before dlopen <path>`：可能是配置的预加载库路径或依赖问题
- 出现 `[preload-library] FAILED`：按输出里的 `dlerror()` 修正 `preload_libraries`
- 停在 `load-library: before dlopen`：可能是 ATFS 库加载路径或动态链接器问题
- 停在 `load-library: after dlopen`：ATFS 库已加载，下一步是符号解析
- 停在 `resolve-symbols: after dlsym`：符号解析完成，下一步是 ATFS 程序名初始化
- 停在 `program-name: before UTSC_ProgramName_Set`：崩溃可能发生在 ATFS 程序名初始化
- 停在 `open-device: before UTSC_Gpib_Open(PH1)`：崩溃可能发生在 `UTSC_Gpib_Open`
- 出现 `open-device: after UTSC_Gpib_Open status=<code>`：`Open` 已返回，按返回码继续排查

如生成 core 文件，可执行：

```sh
gdb ./gpib-test core
bt
```

遇到 `undefined symbol` 时，先查哪个 ATFS 库导出该符号：

```sh
find /opt/ATFS -name '*.so*' -exec sh -c '
  nm -D "$1" 2>/dev/null | grep -q " T UTHN_Handle_FindArea" && echo "$1"
' _ {} \;
```

如果只想先判断是否存在相关 Handle 区域函数，可以放宽为 `grep "Handle_FindArea"`；最终填入 `preload_libraries` 前，仍应尽量用 `UTHN_Handle_FindArea` 精确匹配。

将找到的库填入 `config/gpib-test.conf`：

```ini
preload_libraries=/opt/ATFS/实际路径/libxxx.so
```

## 关于清设备

清设备已经从 `probe` 中拆出，变成单独菜单项 `13. Clear Device (UTSC_Gpib_ClearDevice)`。

这意味着：

- `probe` 现在只做库加载、符号检查和 `UTSC_Gpib_Open` 打开设备；只有配置 `use_rm_lock=1` 时才会先调用 `UTSC_Rm_Lock`
- 只有你明确选择清设备菜单项时，程序才会发送设备清除命令

这样可以降低把初始化检查误当成无影响操作的风险。

## 关于资源锁诊断

菜单 `15. Try RM Lock Only (UTSC_Rm_Trylock)` 和非交互命令 `trylock` 只用于诊断资源锁，不会调用 `UTSC_Gpib_Open`。

- `UTSC_Rm_TryLock(device_name, resource)` 返回成功时，程序会立即调用 `UTSC_Rm_Unlock(resource)` 释放锁。
- `UTSC_Rm_TryLock` 返回失败时，程序会打印返回码和 `UTSC_Rm_GetErrorMessage()`，用于判断资源是否被其他进程占用。
- 如果目标 ATFS 库没有导出 `UTSC_Rm_TryLock`，该诊断命令会直接失败，但不影响 `probe`、`ms`、`send` 等原有功能。

## 退出和连接释放

- 菜单 `16. Exit - 退出` 会调用 `UTSC_Gpib_Close(handle)` 关闭已打开的 GPIB 设备连接；只有配置 `use_rm_lock=1` 且已成功加锁时，才会调用 `UTSC_Rm_Unlock(resource)`，随后卸载共享库。
- 交互模式下按 `Ctrl+C`，或进程收到 `SIGTERM` 时，程序会尽量先走同一套关闭逻辑再退出。
- `kill -9`、断电、进程崩溃等强制终止场景无法保证用户态清理函数被执行。

## 说明

本工具是独立验证器，不承载现有业务流程，也不与当前 `client` 的任务编排、Qt UI、JSON 配置和日志体系做任何复用或集成。
