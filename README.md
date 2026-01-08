# Stream

`stream` is a module used in internal projects.
It provides an **Arduino-style** API for **Serial/WebSocket** so the same `Serial`-based code can run on multiple platforms **without code changes**.
The repository also includes **standards and utilities** for low-level byte processing.

## Features

* Unified Arduino-style interface for `Serial` and `WebSocket`.
* Run the same code across platforms without rewriting.
* Byte-level utilities/standards (parsing, packing, conversions, etc.).

## Tests:

### Stream
``` bash
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -I. utils/sbu.c serial.cpp test/serial.cpp -o test/serial_test
```

``` bash 
./test/serial_test /dev/ttyUSB0
```

``` bash 
./test/serial_test /dev/cu.usbmodemflip_Ivorzam1 > test/serial_test.log
```

### Stream buffer utils (sbu)

``` bash 
gcc -std=c99 -Wall -Wextra -O0 ./test/sbu.c utils/sbu.c -I. -o ./test/test_sbu
```

``` bash 
./test/test_sbu  
```

## Branches

This repository uses two main branches:

* **`main`** — stable code.
* **`dev`** — active development.

## Contributors

<a href="https://github.com/apfxtech/Meshtastic/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=apfxtech/Meshtastic" />
</a>
