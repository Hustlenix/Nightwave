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
Directory work is queued on SD, with only one DIR open rather than a depth-sized
handle stack. The SD mount allows eight open files; filesystem cache/descriptor
allocation still needs runtime measurement. Long filenames and UTF-8 FatFs API
encoding are explicit in sdkconfig.defaults. The OLED still has an ASCII glyph
limitation; this does not change paths or pretend to provide a selected TFT.

## Bounds / files

The **reserved** `.nightwave` directory on the selected card root contains
`catalog-v1.bin`, `catalog-v1.bak`, `catalog-v1.tmp` and `directories-v1.tmp`.
Do not store personal data under those names. Rebuild replaces only these cache
files, never media, lyrics or user playlists. The prior committed generation is
kept as backup. Boot can read it if the main header/file is missing or invalid.
Incomplete temporary files are not trusted; the next rebuild truncates them.

The portable little-endian format has a 32-byte magic/version/root-hash/count/
limit/checksum header and 488-byte records (path, title, artist, album, duration,
checksum). File length/count, string termination, checksum and canonical
root-bounded audio path are checked. Header validation is cheap; each record
is validated when used. A corrupt record clears the entire requested page, not
just the damaged row. Checksums are accidental-corruption detection, **not**
cryptographic authentication. Unsupported/corrupt audio may still appear with
filename fallback; only actual decoding establishes playability.

Caps: 10,000 tracks, 4,096 directories, 100,000 examined entries, depth eight
(root depth zero), paths below 256 bytes. A limit/depth/path overflow marks the
result PARTIAL INDEX / LIMIT. Hidden entries and unsupported files are skipped.
Host traversal refuses symlinks; FAT has no symlinks. This is not a general
multi-user desktop filesystem sandbox. Main + backup + build can approach
14.64 MB for 10,000 tracks; directory work queue adds up to about 1.05 MB.

Each UI tick builds at most two entries/operations, or queries four records
(the API hard cap is eight per call). These are **operation bounds**, not a
guaranteed millisecond bound: one bounded metadata read or SD transaction can
block. Query reads can contend with audio SD reads, though never execute on the
audio worker. This needs SD-latency/underrun acceptance on hardware. Index
refresh rereads metadata; per-file mtime reuse is not implemented.

Unfiltered song paging/advance/seek uses direct record offsets. Artist/album
pages, filtered songs and filtered resume scan incrementally through the
catalog. At four records per 25 ms tick, a 10,000-record scan has a theoretical
62.5-second scheduling envelope **before SD latency**. This is an explicit
remaining performance gap, not production-fast filtered navigation. The UI
shows LOADING PAGE / LOADING NEXT TRACK / RESTORING QUEUE and remains responsive
between slices. A secondary group-offset index or dedicated low-priority worker
is required before claiming fast 10,000-track group navigation.

## Resume / failure

Settings persists a checked `@catalog:<filter>:<tag>` context marker in the
existing library-context string. Explicit resume starts the saved track, then
incrementally locates its ordinal and restores the entire matching queue from
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
cycles. The frontend suite drives real catalog + frontend code with fake audio/
hardware and 260 entries, across-page browsing/playback, seek, repeat, group
playback and context resume. These fake media fixtures do not establish audio
decoding or hardware throughput; separate existing suites exercise real Helix.
Record actual CI/local outcomes in software-validation.md after execution.
