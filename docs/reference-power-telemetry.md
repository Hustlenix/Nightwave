# Reference power telemetry

The reference firmware now samples MAX17048 and BQ25628E through one shared
I2C0 owner (GPIO8/9). The bench firmware keeps its unavailable power backend.
The console `power` command emits values with explicit validity flags. Values
with a false validity flag are placeholders, not measurements. Source enum:
0 battery, 1 USB input present, 2 unknown; USB presence does not rule out battery
supplement. Runtime recording uses the same sampled data but cannot certify a
physical run merely because registers are readable.

MAX17048 voltage uses big-endian words and 78.125 microvolt units; SOC is a
model estimate with pack/temperature compensation still unqualified. Sleep,
unexpected version, implausible voltage, SOC above 100%, and bus errors are
rejected. Its hibernate conversion interval can be 45 seconds: the timestamp is
the register-read time, not a guarantee of a new ADC conversion. No safety
cutoff or low-battery decision is enabled by this driver.
The shared VERSION pattern cannot distinguish MAX17048 from MAX17049; correct
single-cell part assembly must be checked independently.

BQ25628E part identity and status are read along with little-endian configured
charge voltage/current and input-limit registers. The input register limit is
NOT USB permission or the effective current limit: ILIM, DPM and thermal loops
may constrain it further. Raw faults are reported, not dismissed. No current,
temperature, remaining-hours or validated charging safety is inferred.

The driver deliberately has no register-write method. It does not enable ADC,
clear event flags, quick-start the gauge, reset safety timers, change charging,
or disable protections. Reading the chip cannot fix unsafe autonomous defaults.
Reference bus devices use 100 kHz, with a 100 us guard before power reads for
the charger's START spacing requirement. Devices borrow the lifetime-long bus;
creation/retry is serialized before application transfers. Native tests cover
register conversion, wrong identity, every transfer failure, recovery and all
status encodings. ESP-IDF builds and physical bus verification are separate.

Sources: [MAX17048/MAX17049 Rev 7, register summary](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX17048-MAX17049.pdf),
[BQ25628E Rev C, sections 8.4–8.6](https://www.ti.com/lit/ds/symlink/bq25628e.pdf).
