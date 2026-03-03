# RETool - RE Engine PAK & TEX Tool

A command-line tool for working with PAK archives and TEX textures from Capcom's RE Engine games.

Originally created by [FluffyQuack](https://www.youtube.com/user/FluffyQuack). This fork adds support for **RE9 / Monster Hunter Wilds** TEX textures (GDeflate compression) and **ZSTD-compressed PAK creation** for modern RE Engine modding.

## What's New in This Fork

- **RE9 / MH Wilds TEX support** — Full read/write of the new TEX format (extension `.tex.250813143`) with GDeflate tile-stream compression. Round-trip TEX-to-DDS-to-TEX is byte-identical.
- **ZSTD PAK compression** (`-zstd`) — Create PAK files with ZSTD compression, matching the format used by working RE9 mods. Without this flag, deflate is used (original behavior).
- **Embedded manifest** — Created PAKs automatically include a `__MANIFEST/MANIFEST.TXT` file listing all contained paths. During extraction or listing, this manifest is auto-read so filenames resolve without needing an external hash list.

## Requirements

- Windows (x64)
- `libGDeflate.dll` must be in the same directory as `REtool.exe` (included in release, required only for RE9/MHWilds TEX files)

## Quick Start

### Extract a PAK

```
REtool.exe example.pak
```

The tool auto-detects PAK files and extracts them. If the PAK contains an embedded manifest, filenames are resolved automatically.

### Create a PAK (for RE9 / MH Wilds mods)

```
REtool.exe -c mymod -zstd
```

Creates `mymod.pak` from the `mymod` folder with ZSTD compression and an embedded manifest. This is the format expected by modern RE Engine titles.

### Convert Textures

```
REtool.exe texture.tex.250813143       # TEX to DDS
REtool.exe -dds texture.dds            # DDS to TEX (needs original .tex file in same dir)
```

## Command Reference

### PAK Operations

| Command | Description |
|---------|-------------|
| `-x [pak]` | Extract a PAK archive. Can also be triggered by dragging a `.pak` file onto the exe. |
| `-c [folder]` | Create a PAK file from a directory. Output is `folder.pak`. |
| `-l [pak]` | List all files in a PAK archive with compression stats. |
| `-zstd` | Use ZSTD compression when creating a PAK (default is deflate). Use this for RE9/MHWilds mods. |
| `-h [hashList]` | Load an external hash list file for filename resolution during extraction/listing. |
| `-skipUnknowns` | Skip files with unresolved filenames during extraction. |
| `-noExtractDir` | Extract files into the current directory instead of creating a subfolder. |
| `-dontOverwrite` | Skip files that already exist during extraction. |
| `-i [path]` | Invalidate entries in a PAK (for mod installation). |
| `-iAlt [path] [pak]` | Alternate invalidation method using a path starting at `natives`. |
| `-trimList` | Output a trimmed file list containing only paths that matched during listing. |

### TEX / DDS Conversion

| Command | Description |
|---------|-------------|
| `-tex [file]` | Convert a single TEX file to DDS. Also triggered by dragging a `.tex.*` file onto the exe. |
| `-dds [file]` | Convert a single DDS file to TEX. The original TEX file must be in the same directory. Also triggered by dragging a `.dds` file onto the exe. |
| `-tex` (no file) | When used with `-x`, converts all TEX files to DDS during PAK extraction. |
| `-keepBC7type` | Preserve the BC7 sub-type during DDS-to-TEX conversion instead of overwriting it. |
| `-keepWidth` | Keep the original width from the TEX header instead of recalculating from image data. |
| `-savedWidth [width]` | Force a specific width value in the TEX header during DDS-to-TEX conversion. |
| `-texInfo [file]` | Display detailed header information for a TEX file. |
| `-texReduce [file]` | Reduce the resolution of a TEX file (creates a low-res variant for non-streaming use). |
| `-texReduceBy [#]` | Set the resolution divisor for `-texReduce` (default is 4). |

### Utility

| Command | Description |
|---------|-------------|
| `-murmur [string]` | Compute the MurmurHash3 values for a given string. |
| `-outputHashList` | Deduplicate a hash list and write the result as `hashlist.new`. |
| `-combineHashlists` | Combine two hash list files, removing duplicates. Usage: `REtool -combineHashlists list1 list2` |

## Included Batch Files

| File | Usage |
|------|-------|
| `Extract-PAK.bat` | Drag a PAK file onto this to extract it. Edit the batch file to set your hash list filename if needed. |
| `Create-PAK-2023.bat` | Drag a folder onto this to create a PAK (for RE Engine games 2023 and earlier). |
| `Create-PAK-2024.bat` | Drag a folder onto this to create a PAK (for RE Engine games 2024 and newer). |
| `Texture-Conv.bat` | Drag a TEX or DDS file onto this to convert it. |
| `Texture-Conv-All-TEX.bat` | Converts all `.tex.*` files in the same directory to DDS. |
| `Texture-Conv-All-DDSs.bat` | Converts all `.dds` files in the same directory to TEX (preserving BC7 type). |
| `Texture-Conv-Make-Small-Version.bat` | Drag a TEX file onto this to create a 1/4 resolution variant. |

> **Tip for RE9/MHWilds modders:** You don't need the batch files. Just run `REtool -c yourmod -zstd` directly. The embedded manifest means you also don't need a hash list when extracting your own PAKs.

## Supported Games

RETool works with PAK and TEX files from RE Engine titles including:

- Resident Evil 2, 3, 4, 7, 8, 9 (Village, Biohazard)
- Devil May Cry 5
- Monster Hunter Rise, Monster Hunter Wilds
- Dragon's Dogma 2

## Notes

- DDS-to-TEX conversion updates an existing TEX file rather than creating one from scratch. You need the original `.tex` file in the same directory as the DDS.
- For RE9/MHWilds TEX files (`.tex.250813143`), `libGDeflate.dll` is required for decompression and compression.
- When extracting a PAK that contains `__MANIFEST/MANIFEST.TXT`, the manifest is automatically used for filename resolution. No external `-h` hash list is needed.
- PAKs created with `-c` always include a manifest so they are self-describing on re-extraction.
- Without `-zstd`, PAK compression uses deflate (compatible with older RE Engine titles).

## Building from Source

Requires Visual Studio 2022 with MSVC v143 (C++ Desktop workload). Open `PAKtool/PAKtool.vcxproj` and build the x64 Release configuration.

## Credits

- [FluffyQuack](https://www.patreon.com/fluffyquack) — Original RETool
- [Ekey](https://github.com/Ekey) — RE Engine PAK format research
- [gibbed](https://github.com/gibbed) — MH Wilds texture DLC PAK support, PAK checksum algorithm
- GDeflate decompression via `libGDeflate.dll` from [REE-Content-Editor](https://github.com/mhvuze/REE-Content-Editor)

## License

Includes Zstandard (BSD License) — Copyright (c) 2016-present, Facebook, Inc. See source for full license text.
