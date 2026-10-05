# PCB mechanical interface - review revision

Coordinates are KiCad board coordinates in millimetres, viewed from the top.
The board outline is (5,5) to (115,105): 110 x 100 mm, nominal 1.2 mm thick.
The selected SJ3-350153AG jack requires the 1.2 mm board thickness; do not
silently substitute a manufacturer's default 1.6 mm stackup.

| Feature | X | Y | Requirement |
| --- | ---: | ---: | --- |
| H1 | 71 | 9 | 2.2 mm non-plated hole for M2 nylon hardware |
| H2 | 9 | 101 | 2.2 mm non-plated hole for M2 nylon hardware |
| H3 | 111 | 101 | 2.2 mm non-plated hole for M2 nylon hardware |

Each mounting point has a 5 mm diameter copper/track/via exclusion on all
copper layers. Three points define the support plane. Enclosure bosses must
match these centres without bending the PCB. Screw length depends on the
enclosure and is not selected here. Use non-conductive hardware; do not place
metal, the battery, display electronics or cables in the radio antenna zones.

The USB and headphone connectors face opposite board edges. microSD insertion,
button travel, display harness bend radius, battery retention and all port
cutouts still need a matching enclosure interference review. The PCB STEP
is a board assembly reference, **not enclosure CAD**; several custom component
bodies are absent. Do not infer a component is physically absent from the
render merely because its 3D model is unavailable.

BM83 manufacturer ground lands 56/57 are connected through vias inside their
lands. These require an assembly-house review of via filling/capping, solder
wicking and paste coverage. Only those ground lands are excluded from the
stock body keepout; the full antenna copper exclusion is preserved.
