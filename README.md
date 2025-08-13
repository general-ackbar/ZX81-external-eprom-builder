# ZX81 External EPROM Builder

A small utility to help build bootable images from `.P` files for the [ZX81 External EPROM interface](https://github.com/thomasheckmann/zx81-external-eprom) by [Thomas Heckmann](https://github.com/thomasheckmann).

---

## Overview

The **ZX81 External EPROM interface** is a clever interface that allows loading different ROMs for the ZX81 without having to open and reflash the system ROMs.  
It can contain **8 × 8 KB ROMs** but also supports organizing the EPROM into **4 × 16 KB banks**. This leaves room for both a system ROM and one or more `.P` files.

The [original repository](https://github.com/thomasheckmann/zx81-external-eprom) contains the code to achieve this (see the `autorun` folder).  
This repository contains a **small command-line utility** that makes the process easier:

- Combines a modified system ROM, a small loader, and a `.P` file.
- Compresses the `.P` file using **zx7**.
- Loader decompresses the file at load time.
- Makes it possible to include programs **larger than 8 KB**.

---

## Building

```bash
make
```

---

## Usage

```bash
./p2rom [-b base.bin] [-l loader.bin] [-o output.rom] input.p
```

### Examples

```bash
# Creates a ROM image (game.rom) containing and autorunning game.p
./p2rom game.p

# Creates a ROM image containing and autorunning game.p.zx7, output is xyz.rom
./p2rom -o xyz.rom game.p.zx7

# Creates a ROM image with game.p, using myloader.bin and customrom.bin
./p2rom -b customrom.bin -l myloader.bin game.p
```

---

## ROM Modifications

The ZX81 system ROM needs a few modifications to autorun the `.P` file.  
See [this description](https://quix.us/timex/rigter/AutoBasic.html) for details.

- The source code adjusted for the **sjasmplus** assembler is in the `asm` folder.
- Includes source for the loader and zx7 decompression code.
- Precompiled versions are in the `bin` folder.

---

## Modifying Loader or Base Image

If you want to modify the loader or base image, assemble them with:

```bash
sjasmplus file.asm --raw=output.bin
```

