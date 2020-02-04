# J-Link debugger

* Install [J-Link software](https://www.segger.com/products/debug-probes/j-link/tools/j-link-gdb-server/about-j-link-gdb-server/)
* Connect 4 SWD pins
* Run J-Link
```bash
$ JLinkExe
```
* Configure J-Link
```bash
si 1
speed 4000
device Cortex-M0
r
```
