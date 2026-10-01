"""Reproducible design-screening calculations; no physical results or approvals."""

import math
from power_budget import budget


def positive(*values):
    if not all(math.isfinite(v) and v > 0 for v in values):
        raise ValueError("finite positive inputs required")


def recharge_screen(capacity_ah, current_a, minimum_fraction=0.95, allowance=1.25):
    positive(capacity_ah, current_a, minimum_fraction, allowance)
    if minimum_fraction > 1 or allowance < 1:
        raise ValueError("invalid tolerance/allowance")
    return capacity_ah / (current_a * minimum_fraction) * allowance


def charge_headroom(input_v, input_a, efficiency, system_w, cell_v):
    positive(input_v, input_a, efficiency, cell_v)
    if efficiency > 1 or not math.isfinite(system_w) or system_w < 0:
        raise ValueError("invalid efficiency/system load")
    # Negative means battery supplement is needed, not negative charging.
    return (input_v * input_a * efficiency - system_w) / cell_v


def feedback_voltage(reference_v, upper_ohm, lower_ohm):
    positive(reference_v, upper_ohm, lower_ohm)
    return reference_v * (1 + upper_ohm / lower_ohm)


def headphone_budget(dac_vrms, amplifier_db, target_vrms, load_ohm):
    positive(dac_vrms, target_vrms, load_ohm)
    if not math.isfinite(amplifier_db):
        raise ValueError("finite gain required")
    full_scale = dac_vrms * 10 ** (amplifier_db / 20)
    attenuation = min(1, target_vrms / full_scale)
    return full_scale, attenuation, 20 * math.log10(attenuation), target_vrms ** 2 / load_ohm


def self_test():
    assert math.isclose(recharge_screen(6.6, 1), 8.68421052631579)
    assert recharge_screen(6.6, .5) > 10.5
    assert recharge_screen(6.6, .5) < 21
    assert recharge_screen(6.6, .32) > 21
    assert math.isclose(feedback_voltage(.5, 511000, 91000), 3.3076923076923075)
    assert charge_headroom(5, .1, .85, 1.4168, 4.2) < 0
    assert charge_headroom(5, .5, .85, 1.4168, 4.2) < .17
    full, factor, db, watts = headphone_budget(2.1, -6, .3, 32)
    assert 1.052 < full < 1.053 and .285 < factor < .286
    assert -10.91 < db < -10.89 and math.isclose(watts, .0028125)
    assert headphone_budget(.1, 0, .3, 32)[1] == 1
    invalid_calls = (
        lambda: recharge_screen(0, 1),
        lambda: recharge_screen(6.6, 1, 1.1),
        lambda: recharge_screen(6.6, 1, allowance=.9),
        lambda: charge_headroom(5, 1, 1.1, 0, 4.2),
        lambda: charge_headroom(5, 1, .85, float("nan"), 4.2),
        lambda: feedback_voltage(.5, 1000, 0),
        lambda: headphone_budget(2.1, float("inf"), .3, 32),
        lambda: headphone_budget(2.1, 0, .3, -1),
    )
    for call in invalid_calls:
        try:
            call()
        except ValueError:
            continue
        raise AssertionError("invalid input accepted")


def main():
    self_test()
    power, _ = budget()
    print("CALCULATED screening examples only; not measured or design approval")
    for current in (1, .5, .32):
        print(f"6.6 Ah recharge allowance at {current:.2f} A: {recharge_screen(6.6, current):.2f} h")
    for source in (.5, 1.35):
        headroom = charge_headroom(5, source, .85, power, 4.2)
        margin = charge_headroom(5, source, .85, power * 1.2, 4.2)
        print(f"5 V/{source:.2f} A charge headroom: {headroom:.3f} A; 20% load margin: {margin:.3f} A")
    print(f"TPS reference divider nominal: {feedback_voltage(.5, 511000, 91000):.4f} V")
    full, factor, db, watts = headphone_budget(2.1, -6, .3, 32)
    print(f"Headphone example: unclipped {full:.3f} Vrms; attenuation {factor:.4f} ({db:.2f} dB); target power {watts*1000:.4f} mW")
    print("0.3 Vrms is an illustrative target, not a universal hearing-safety limit")
    print("Calculation self-tests passed")


if __name__ == "__main__":
    main()
