# Incremental library catalog

2026-10-02. Implemented source and host fixtures; hardware timing, card-loss,
power-cut and large-card acceptance remain unverified. Bluetooth/display are
still builder choices, not selected by this software change.

## Behavior

Settings now has LIBRARY and REBUILD INDEX rows. Library offers Songs, Artists,
Albums and the existing Folders/Playlists browser. Previous/Next moves between
rows and crosses page boundaries. Hold Previous goes up. Group selection opens
its songs; Play starts a track. Playback modes operate across the full indexed
collection/filter, not just the visible 16 rows. Album groups use the exact
album tag; equal album names across artists are intentionally combined. Missing
tags use `(UNKNOWN)`. Groups are bytewise/case-sensitive lexicographic; songs
retain traversal order. No promise of locale collation/title sorting is made.

A valid cached header can load without rescanning the whole card. The UI then
builds a replacement generation in small idle slices, also on an explicit
rebuild. Existing cache remains readable while the replacement is incomplete.
Metadata parsing and index writes pause even when music is *paused* (the audio
workers still own resources). Never run a full-card scan on an audio task.

The catalog is an explicit PSRAM member of UiAssets. At most 16 full metadata
records are resident, approximately 8 KiB, plus bounded traversal/control state.
The v2 accelerator adds one explicitly allocated 227,488-byte PSRAM generation
(fixed names/counts, 16-bit grouped track IDs and build/load scratch). A rebuild
may retain one active and one staging generation: 454,976 bytes peak, excluding
UiAssets/filesystem/decoder/display allocations. No full-library metadata/path
vector is allocated. Allocation failure or more than 1,024 distinct artist or
album keys disables the accelerator without dropping tracks; the UI shows SLOW
LOOKUP / FALLBACK. This fallback is correct but may be slow, not production-fast.
Directory work is queued on SD, with only one DIR open rather than a depth-sized
handle stack. The SD mount allows eight open files; filesystem cache/descriptor
allocation still needs runtime measurement. Long filenames and UTF-8 FatFs API
encoding are explicit in sdkconfig.defaults. The OLED still has an ASCII glyph
limitation; this does not change paths or pretend to provide a selected TFT.

## Bounds / files

The **reserved** `.nightwave` directory on the selected card root contains
`catalog-v2.bin`, `catalog-v2.bak`, `catalog-v2.tmp` and `directories-v2.tmp`.
Do not store personal data under those names. Rebuild replaces only these cache
files, never media, lyrics or user playlists. The prior committed generation is
kept as backup. Boot can read it if the main header/file is missing or invalid.
Incomplete temporary files are not trusted; the next rebuild truncates them.

The portable little-endian v2 format has a 64-byte magic/version/root-hash/count/
limit/checksum header and unchanged 488-byte records (path, title, artist, album,
duration, checksum), followed by an optional checked lookup footer. Footer group
rows contain a 64-byte name, 16-bit start/count; artist and album track-ID arrays
follow. Each field must cover every track exactly once. File length/count,
string termination, checksum and canonical
root-bounded audio path are checked. Header validation is cheap; each record
is validated when used. A corrupt record clears the entire requested page, not
just the damaged row. Checksums are accidental-corruption detection, **not**
cryptographic authentication. Unsupported/corrupt audio may still appear with
filename fallback; only actual decoding establishes playability.

The footer is loaded separately in at most four 1,024-byte chunks per UI tick;
records are available before warmup completes. Footer shape/counts, checksum,
terminated/unique normalized names, contiguous group ranges, in-range IDs and
complete ID permutations are checked. A bad footer drops only the accelerator;
selected filtered records also must match their actual tag, even if a malformed
mapping has recomputed checksums. Group dictionaries are not an authenticated
description of all metadata; checksums are not a hostile-editor security boundary.
Boot tries v2 main/backup, then legacy `catalog-v1.bin`/`catalog-v1.bak`. Legacy
32-byte-header caches remain readable through the bounded slow path. Rebuilding
writes v2 and preserves v1 files; no migration deletes user/legacy media.

Caps: 10,000 tracks, 4,096 directories, 100,000 examined entries, depth eight
(root depth zero), paths below 256 bytes. A limit/depth/path overflow marks the
result PARTIAL INDEX / LIMIT. Hidden entries and unsupported files are skipped.
Host traversal refuses symlinks; FAT has no symlinks. This is not a general
multi-user desktop filesystem sandbox. Main + backup + build can approach
15.18 MB for 10,000 tracks with maximum lookup footers; directory work queue adds
up to about 1.05 MB. Lookup footer is at most 179,264 bytes (1,024 keys per field
plus 10,000 IDs per field). Smaller dictionaries reduce file size, not the fixed
227,488-byte RAM allocation. This bounded memory/performance tradeoff is explicit.

Each UI tick builds at most two entries/operations, or queries four records
(the API hard cap is eight per call). Build sealing handles 64 assignments per
operation; footer writes handle at most 1,024 bytes per operation. Dictionary
sort/structural validation at completion is bounded but not time-sliced further.
These are **operation bounds**, not a
guaranteed millisecond bound: one bounded metadata read or SD transaction can
block. Query reads can contend with audio SD reads, though never execute on the
audio worker. This needs SD-latency/underrun acceptance on hardware. Index
refresh rereads metadata; per-file mtime reuse is not implemented.

With a ready v2 lookup, artist/album names are paged/sorted from RAM (zero catalog
record reads). Filtered song pages directly access grouped IDs (at most 16 record
reads), and filtered track selection/advance requires one checked record read.
Unfiltered selection also uses direct offsets. A correct saved ordinal hint
restores a queue with one checked record read; stale/missing hints still search
for the exact saved path, never selecting a different song just by ordinal.

Without a lookup, artist/album pages, filtered songs and resume use the original
incremental scan. At four records per 25 ms tick, a 10,000-record scan has a
62.5-second scheduling envelope **before SD latency**. Maximum-footer warmup
needs 176 chunks/44 ticks (1.1 seconds of scheduling, not measured SD time).
The UI shows LOOKUP WARMING and restarts pending queries when the lookup becomes
ready. LOADING PAGE / LOADING NEXT TRACK / RESTORING QUEUE remain visible during
queries. Host record-read counts do not prove hardware response/underrun budgets;
SD contention and low-memory/over-cap fallback latency remain acceptance gaps.

## Resume / failure

Settings persists `@catalog2:<filter>:<ordinal>:<tag>` in the existing checked
library-context string, updating the hint on each indexed track change. Legacy
`@catalog:<filter>:<tag>` markers are still accepted, without a hint. The NVS blob
schema is unchanged. Malformed/oversized hints do not drive a catalog jump.
Explicit resume starts the saved track, then
locates its verified ordinal and restores the entire matching queue from
a valid cached generation. Missing cache/path falls back to the one saved track;
no surprise autoplay on boot. While the queue is restoring, Next waits. Folder
resume now reconstructs its bounded queue if the saved canonical folder still
contains the track; folder scanning remains limited to 128 entries. M3U resume
retains its existing 128-entry bound. An explicit stop cancels pending catalog
actions and keeps a single-track checkpoint.

Write/read/rename errors are visible and leave the old readable generation when
possible. Manual rebuild is refused while playback owns workers; stop first.
Publish flushes/syncs/closes before rotating names. This is **not a claim of FAT
transactional durability**: real power-loss injection, card-removal/remount,
read-only/full-card and worst-latency tests remain required. See
[ESP-IDF FatFs behavior and RAM tuning](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/fatfs.html)
and the [pinned v6.1 configuration](https://github.com/espressif/esp-idf/blob/v6.1/components/fatfs/Kconfig).

## Executable coverage

The catalog suite uses 262 original metadata-only/empty/corrupt fixtures, nested
folders, 40 artist keys, album grouping, forward/backward paging, direct ordinal
access, filtered location, cache reload/backup fallback, checksum/root-escape
rejection, truncation, depth overflow, symlink refusal and 1,000 query/cancel
cycles, plus an actual 10,000-file metadata-only catalog. It checks zero-read
group pages, 16-read filtered pages, one-read selection/resume, stale hints,
empty catalogs, legacy cache migration, malformed footer strings/ranges/IDs,
valid-permutation/wrong-tag rejection, 1,024-group success, 1,025-group fallback
and deliberately disabled acceleration. The frontend suite drives real catalog + frontend code with fake audio/
hardware and 260 entries, across-page browsing/playback, seek, repeat, group
playback and cold-boot/legacy/malformed context resume without autoplay.
These fake media fixtures do not establish audio
decoding or hardware throughput; separate existing suites exercise real Helix.
Record actual CI/local outcomes in software-validation.md after execution.
