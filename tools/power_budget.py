"""Deterministic preliminary energy model, never a measured runtime report."""
import math


def budget(load_ma=278, speaker_w=0.25, regulator_eff=0.85, amp_eff=0.80,
           margin=1.20, usable=0.80, aging=0.80, hours=8, extra_cell_w=0):
    values = (load_ma, speaker_w, regulator_eff, amp_eff, margin, usable, aging, hours, extra_cell_w)
    if not all(math.isfinite(v) for v in values):
        raise ValueError("finite values required")
    if load_ma < 0 or speaker_w < 0 or extra_cell_w < 0 or hours <= 0 or margin < 1:
        raise ValueError("invalid load/duration/margin")
    if not all(0 < v <= 1 for v in (regulator_eff, amp_eff, usable, aging)):
        raise ValueError("efficiencies/fractions must be in (0,1]")
    power = 3.3 * load_ma / 1000 / regulator_eff + speaker_w / amp_eff + 0.025 + extra_cell_w
    capacity_ah = power * hours * margin / (3.7 * usable * aging)
    return power, capacity_ah


def main():
    power, required = budget()
    assert 5.74 < required < 5.75
    high_volume = budget(speaker_w=0.5)[1]
    assert high_volume > 6.6
    for invalid in (0, -1, 1.01, float("nan")):
        try:
            budget(regulator_eff=invalid)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid model accepted")
    print("CALCULATED model only; physical validation pending")
    print(f"Cell-equivalent power: {power:.4f} W; capacity requirement: {required:.3f} Ah")
    for capacity in (2.5, 6.6):
        runtime = capacity * 3.7 * 0.8 * 0.8 / (power * 1.2)
        print(f"{capacity:.1f} Ah estimated runtime with margins: {runtime:.2f} h")
    print(f"0.5 W speaker sensitivity: {high_volume:.3f} Ah required")
    print("EXPANDED SCOPE: scenario assumptions, not display/module measurements")
    scenarios = (
        ("TFT 60mA / speaker 0.25W / BT off", 333, 0.25, 0),
        ("TFT 60mA / speaker 0.50W / BT off", 333, 0.50, 0),
        ("TFT 100mA / speaker 0.25W / BT off", 373, 0.25, 0),
        ("TFT 60mA / BT 50mA cell / speaker muted", 333, 0, 3.7 * 0.050),
        ("TFT 60mA / speaker 0.25W / BT scanning", 333, 0.25, 3.7 * 0.050),
    )
    for name, load, speaker, extra in scenarios:
        scenario_power, ah = budget(load_ma=load, speaker_w=speaker, extra_cell_w=extra)
        runtime = 6.6 * 3.7 * 0.8 * 0.8 / (scenario_power * 1.2)
        assert ah > 0 and runtime > 0
        print(f"{name}: cell power={scenario_power:.4f}W capacity={ah:.3f}Ah 6.6Ah runtime={runtime:.2f}h")


if __name__ == "__main__":
    main()
