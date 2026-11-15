"""Checks a release archive before it is published. A published immutable release cannot be fixed.

Usage: python -I check-archive.py <zip> <windows-x64|linux-x86_64>

The archive must hold exactly the expected files. The plugins must be found the way Audio Plugin Soko-kun
scans archives: the VST 3 binary sits in the bundle layout and exports GetPluginFactory, the CLAP binary
exports clap_entry, and no binary exports the VST 2 entry points VSTPluginMain or main_macho, or main
from a shared library. Only the standard library is used, and the archive is read in memory.
"""

import struct
import sys
import zipfile

NAME = "simple-whitenoise"
COMMON = [
    "LICENSE",
    "README.txt",
    f"{NAME}.clap",
    "THIRD_PARTY_LICENSES/clap.txt",
    "THIRD_PARTY_LICENSES/clap-wrapper.txt",
    "THIRD_PARTY_LICENSES/fmt.txt",
    "THIRD_PARTY_LICENSES/vst3sdk.txt",
]
VST3_BINARY = {
    "windows-x64": f"{NAME}.vst3/Contents/x86_64-win/{NAME}.vst3",
    "linux-x86_64": f"{NAME}.vst3/Contents/x86_64-linux/{NAME}.so",
}
EXPECTED_FORMAT = {"windows-x64": "pe", "linux-x86_64": "elf"}
VST2_EXPORTS = {"VSTPluginMain", "main_macho"}


def parse_pe(d):
    lfanew = struct.unpack_from("<I", d, 0x3C)[0]
    if d[lfanew : lfanew + 4] != b"PE\0\0":
        raise ValueError("no PE signature")
    coff = lfanew + 4
    machine, nsec = struct.unpack_from("<HH", d, coff)
    opt_size, chars = struct.unpack_from("<HH", d, coff + 16)
    opt = coff + 20
    if struct.unpack_from("<H", d, opt)[0] != 0x20B:
        raise ValueError("not a PE32+ image")
    export_rva = struct.unpack_from("<I", d, opt + 112)[0]
    sections = []
    for i in range(nsec):
        s = opt + opt_size + 40 * i
        vsize, va, rawsize, rawptr = struct.unpack_from("<IIII", d, s + 8)
        sections.append((va, max(vsize, rawsize), rawptr, rawsize))

    def offset(rva):
        for va, size, rawptr, rawsize in sections:
            if va <= rva < va + size and rva - va < rawsize:
                return rawptr + rva - va
        raise ValueError(f"RVA {rva:#x} is not mapped")

    exports = set()
    if export_rva:
        eo = offset(export_rva)
        count, names_rva = struct.unpack_from("<I", d, eo + 24)[0], struct.unpack_from("<I", d, eo + 32)[0]
        for i in range(count):
            o = offset(struct.unpack_from("<I", d, offset(names_rva) + 4 * i)[0])
            exports.add(d[o : d.index(b"\0", o)].decode("latin-1"))
    return {
        "format": "pe",
        "x86_64": machine == 0x8664,
        "shared": bool(chars & 0x2000),  # IMAGE_FILE_DLL
        "exports": exports,
    }


def parse_elf(d):
    if d[4] != 2 or d[5] != 1:
        raise ValueError("not a 64-bit little-endian ELF")
    etype, machine = struct.unpack_from("<HH", d, 16)
    phoff, shoff = struct.unpack_from("<QQ", d, 32)
    phentsize, phnum, shentsize, shnum = struct.unpack_from("<HHHH", d, 54)
    has_interp = any(struct.unpack_from("<I", d, phoff + phentsize * i)[0] == 3 for i in range(phnum))  # PT_INTERP
    sections = [struct.unpack_from("<IIQQQQIIQQ", d, shoff + shentsize * i) for i in range(shnum)]
    exports = set()
    for _name, typ, _flags, _addr, off, size, link, _info, _align, entsize in sections:
        if typ != 11:  # SHT_DYNSYM
            continue
        stroff = sections[link][4]
        for i in range(size // (entsize or 24)):
            nm, info, other, shndx = struct.unpack_from("<IBBH", d, off + (entsize or 24) * i)
            # Defined GLOBAL or WEAK symbols with DEFAULT or PROTECTED visibility.
            if info >> 4 in (1, 2) and shndx != 0 and other & 3 in (0, 3) and nm:
                p = stroff + nm
                exports.add(d[p : d.index(b"\0", p)].decode("utf-8", "replace"))
    return {
        "format": "elf",
        "x86_64": machine == 62,
        # A PIE executable is also ET_DYN, but it has PT_INTERP.
        "shared": etype == 3 and not has_interp,
        "exports": exports,
    }


def parse_binary(data):
    if data[:2] == b"MZ":
        return parse_pe(data)
    if data[:4] == b"\x7fELF":
        return parse_elf(data)
    return None


def main(argv):
    if len(argv) != 3 or argv[2] not in VST3_BINARY:
        print(__doc__.strip().splitlines()[2], file=sys.stderr)
        return 2
    path, platform = argv[1], argv[2]
    vst3_binary = VST3_BINARY[platform]
    errors = []

    with zipfile.ZipFile(path) as zf:
        names = sorted(info.filename for info in zf.infolist() if not info.is_dir())
        print("\n".join(names))
        expected = sorted(COMMON + [vst3_binary])
        if names != expected:
            errors.append(f"files differ: missing {sorted(set(expected) - set(names))}, unexpected {sorted(set(names) - set(expected))}")

        binaries = {}
        for name in names:
            data = zf.read(name)
            if not data:
                errors.append(f"{name}: empty")
                continue
            parsed = parse_binary(data)
            if parsed:
                binaries[name] = parsed

    for name, binary in sorted(binaries.items()):
        print(f"{name}: {binary['format']}, shared={binary['shared']}, exports={sorted(binary['exports'])}")
        vst2 = binary["exports"] & VST2_EXPORTS
        if binary["shared"] and "main" in binary["exports"]:
            vst2.add("main")
        if vst2:
            errors.append(f"{name}: exports VST 2 entry points {sorted(vst2)}")

    for name, symbol in ((vst3_binary, "GetPluginFactory"), (f"{NAME}.clap", "clap_entry")):
        binary = binaries.get(name)
        if binary is None:
            errors.append(f"{name}: not a binary")
            continue
        if binary["format"] != EXPECTED_FORMAT[platform] or not binary["x86_64"] or not binary["shared"]:
            errors.append(f"{name}: not an x86_64 {EXPECTED_FORMAT[platform]} shared library")
        if symbol not in binary["exports"]:
            errors.append(f"{name}: does not export {symbol}")

    for error in errors:
        print(f"error: {error}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
