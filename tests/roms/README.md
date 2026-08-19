# Validation ROMs

External Z80 test ROMs used by the optional compliance suite. These are CP/M `.com` / `.cim` binaries: load them at memory address `0x0100`, set `PC = 0x0100`, set `SP` somewhere safe (e.g. `0xF000`), and trap BDOS calls at `0x0005` (and the warm-boot vector at `0x0000`).

## Files

| File          | Size    | Source                                                | Purpose                                                    |
|---------------|---------|-------------------------------------------------------|------------------------------------------------------------|
| `prelim.com`  | 1280 B  | superzazu/z80 (`roms/prelim.com`)                     | Quick sanity test. Prints `Preliminary tests complete` on success. |
| `zexdoc.cim`  | 8588 B  | superzazu/z80 (`roms/zexdoc.cim`)                     | Documented-behavior Z80 instruction exerciser. ~46 tests, ~30 s on a real Z80. |
| `zexdoc.com`  | 8588 B  | identical bytes to `zexdoc.cim`                       | Same file, alternate extension some harnesses expect.       |
| `zexall.cim`  | 8588 B  | superzazu/z80 (`roms/zexall.cim`)                     | Same exerciser with undocumented flag bits 3/5 included in the CRCs. |

The `.cim` extension is what the superzazu/z80 repository ships; the duplicate `.com` lets test code load either path without caring which extension upstream chose.

## BDOS harness expectations

ZEXDOC and ZEXALL use only two BDOS calls. Implement them in the test host's `read_memory` (or by intercepting calls to `0x0005`):

- **Function 2 (C-write):** print the byte in `E` as ASCII. Used for progress and pass/fail messages.
- **Function 9 (string-print):** print the `$`-terminated string at `DE`.

Warm-boot via `RST 0` (jump to `0x0000`) means the program is done. A "success" line ends with `OK` for each sub-test; failures print the expected vs actual CRC.

## License / provenance

ZEXDOC was authored by Frank D. Cringle and is in the public domain. The superzazu/z80 repository (MIT-licensed) re-distributes the same binaries used widely across the Z80 emulation community.

## Update

```bash
curl -fsSL https://github.com/superzazu/z80/raw/master/roms/zexdoc.cim   -o tests/roms/zexdoc.cim
curl -fsSL https://github.com/superzazu/z80/raw/master/roms/zexall.cim   -o tests/roms/zexall.cim
curl -fsSL https://github.com/superzazu/z80/raw/master/roms/prelim.com   -o tests/roms/prelim.com
cp tests/roms/zexdoc.cim tests/roms/zexdoc.com
```

Hashes (SHA-256) recorded at download time:

- `prelim.com` : `3b3578f19030a4df7e25ce852f763af26053b12582a576c4dffb014aa7c590d1`
- `zexdoc.cim` : `10b7c3972ff6765712ed160e5bd8750e4a13642f62b75711e062ef06a7f2f7b5`
- `zexall.cim` : `af7e5d86146d390a68440fb85668648f14a648602da29a1816d2ef11459411ae`
