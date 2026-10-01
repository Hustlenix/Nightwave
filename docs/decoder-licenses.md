# MP3 dependency provenance

Nightwave uses the ESP-IDF component `chmorgan/esp-libhelix-mp3` exactly 1.0.3.
The host test uses its bundled decoder revision
`f4430799e70a4fff44513e71bd4459411802b819` from
https://github.com/chmorgan/libhelix-mp3. The wrapper is Apache-2.0; that does
not relicense the bundled Helix decoder. Its source carries RealNetworks
RPSL/RCSL notices, reproduced unchanged in `licenses/helix/`.

Nightwave's adapter is separate from, and does not modify, upstream decoder
source. Builds fetch the upstream component/source; a source release must
retain those notices and make the corresponding dependency source available.
Do not describe the complete firmware as wholly MIT/Apache licensed. Review
the upstream license conditions before redistributing binaries, changing
decoder source, or commercial use; this record is not legal advice.

The CI MP3 fixtures are encoded from Nightwave's generated test tones with
FFmpeg/libmp3lame. They are not downloaded copyrighted songs. Fixture encoding
does not link FFmpeg into firmware.
