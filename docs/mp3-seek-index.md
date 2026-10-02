# MP3 duration and sparse seeking

The idle library builder scans MPEG1/2 Layer III frames without decoding audio.
It records every 128th frame in a fixed 1024-point index and sums represented
samples/rate for duration. Xing/Info frame counts provide a bounded quick estimate
when valid. Full scanning works for supported CBR/VBR without those headers.
WAV duration remains data bytes/byte rate. Zero means unknown, never file-size
divided by an assumed bitrate. No VBRI shortcut or gapless encoder-delay trimming
is claimed; framed duration includes header/padding frames.

## Limits and integration

- Supported index rates: 22.05/32/44.1/48 kHz PCM16 decode paths, MPEG1/2 Layer III.
  MPEG2.5, free bitrate, truncated/rate-changing streams and recovered garbage
  do not publish an index. Structural indexing does not prove decodability.
- At most 131072 frames, 1024 fixed points. Longer tracks remain playable using
  decode-from-start; no catalog entry is discarded for index capacity failure.
- Scanner object is statically bounded to <=12 KiB, resides in UI-assets PSRAM,
  reads <=1536 bytes per refill and does at most eight frame/read operations per
  call. Catalog uses one operation at a time inside its idle budget. Leftovers
  are retained, so normal scanning reads each encoded byte once, not overlapping
  repeated full windows. Indexing pauses whenever playback workers own audio.
- Cache is reserved `.nightwave/seek-<path-hash>.bin`, `NWSEEK01`, <=8500 bytes:
  304-byte identity/header, <=8192 point bytes and four-byte payload checksum.
  No content is written beside user music. Paths/header/count/rate/sample totals,
  strictly increasing offsets, selected frame and checksums are checked.
- Freshness: canonical media path, size, mtime and first/last 512-byte fingerprints,
  rechecked before save. This is accidental-staleness detection, not malicious
  edit authentication. A same-size/same-mtime middle-only edit can evade windows.
  Host cache directory/symlinks are rejected. SD FAT has no host-style symlinks.
- Save uses a temporary file, flush/fsync and rename; existing cache survives
  write failure. FAT power-loss atomicity and directory durability are unproven.
  Maximum cache completion write is <=8.5 KiB; actual UI/SD latency is unmeasured.

## Seek policy

Seek/resume is still capped at 30 minutes. Start chooses one sparse checkpoint
before the target group, providing at least 128 and less than 256 MPEG frames
of reservoir/synthesis preroll for late targets. Helix is reset, preroll is
decoded/discarded, and only target/post-target samples enter the PCM ring.
Consumed underflow frames advance the logical media timeline but never publish
undefined audio. Ordinary decode recovery remains track-budgeted (8192 bytes).

Missing, corrupt, stale, too-short or over-cap index falls back to cancellable
decode-from-start; no bitrate-derived byte jump. Stored index duration is framed,
not an acoustic/gapless timing measurement. Telemetry exposes indexed_seek,
seek_base_samples, duration, decode calls and storage bytes.

Host validation: 2000 structured frames, limits, changed files, corruption,
malformed ID3/Xing, truncation/rate changes, symlink-cache refusal and 1000
cancellations. Generated 30-second MP3 plus real Helix compares 15-second indexed
seek with decode-from-start: PCM hash/sample count match, decoder/SD work drops.
These are software checks, not a real card or headphone latency result.
