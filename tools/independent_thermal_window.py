"""Ratiometric thermal-window sensitivity; not a qualified charging circuit."""
import itertools
import json
import math


def divider(bottom, top):
    return bottom / (bottom + top)


def screen(rt_error=.05, resistor_error=.001, error_v=.033, supply_v=3.0):
    """Enumerate independent resistor corners and both comparator errors.

    33 mV is an engineering allowance (8 mV offset + 25 mV hysteresis),
    not a guaranteed error specification at this circuit's common mode.
    R(T) +/-5% and interpolated 45 C resistance are sensitivity assumptions.
    """
    if not all(math.isfinite(x) for x in
               (rt_error, resistor_error, error_v, supply_v)):
        raise ValueError("nonfinite assumption")
    if not (0 <= rt_error < 1 and 0 <= resistor_error < 1
            and error_v >= 0 and supply_v > 0):
        raise ValueError("invalid assumption")
    factors = (1-resistor_error, 1+resistor_error)
    references = {}
    for name, lower in (("cold", 215000), ("hot", 61900)):
        corners = [divider(lower*b, 100000*t) + e*error_v/supply_v
                   for b, t, e in itertools.product(factors, factors, (-1, 1))]
        references[name] = (min(corners), max(corners))
    sensor = {}
    for temp, resistance in ((0, 27280), (25, 10000),
                             (45, math.sqrt(5827*4160))):
        corners = [divider(resistance*r, 10000*p)
                   for r, p in itertools.product((1-rt_error, 1+rt_error), factors)]
        sensor[str(temp)] = (min(corners), max(corners))
    margins = {
        "cold_block": sensor["0"][0] - references["cold"][1],
        "hot_block": references["hot"][0] - sensor["45"][1],
        "room_above_hot": sensor["25"][0] - references["hot"][1],
        "room_below_cold": references["cold"][0] - sensor["25"][1],
    }
    return {
        "classification": "CALCULATED_WITH_ESTIMATED_ERROR_ENVELOPES",
        "sensor_ratios": sensor, "reference_ratios": references,
        "normalized_margins": margins,
        "sensitivity_passes": all(x > 0 for x in margins.values()),
        "hardware_qualified": False,
        "assumptions": dict(rt_error=rt_error, resistor_error=resistor_error,
                            error_v=error_v, supply_v=supply_v),
    }


if __name__ == "__main__":
    print(json.dumps(screen(), indent=2, allow_nan=False))
