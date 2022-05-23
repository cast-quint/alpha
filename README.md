# Alpha

This is the course project of my [Languages and Compilers](https://www.csd.uoc.gr/~hy340/) course, developed in the Spring of 2021.
The goal was to develop a **bytecode compiler** and **virtual machine** for a simple language, Alpha, from scratch.
The compiler and virtual machine are completely independent binaries.

## BUILDING:
```
cd alpha
make
```
This will produce the bytecode compiler and virtual machine, ``alc`` and ``avm``, respectively.

## USAGE:
```
    brief: './alc <source> | ./avm'

    alc: ./alc [--log] <source>
        (--log generates log files about the symbol table, the intermediate code and the final bytecode.)

   avm: ./avm <file.abc>
```

