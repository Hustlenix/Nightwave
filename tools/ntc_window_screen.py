"""Necessary-condition screen, NOT a qualified thermistor design.

BQ25628E reset cold/hot comparator ratios and SEMITEC nominal 103AT table.
Interpolated 45 C value and +/-2% resistance envelope are ESTIMATED; obtain
manufacturer guaranteed R(T) limits and include thermal lag before release.
"""
import json
import math


def feasibility(cold_r=27280, hot_r=math.sqrt(5827*4160), envelope=.02):
    # Network: pull-up Rt, thermistor || optional fixed shunt Rb to ground.
    # a=1/v-1=Rt/Rntc+Rt/Rb. Positive shunt cannot improve a_hot/a_cold.
    # Must block at 0 C for highest cold threshold; at 45 C for lowest hot.
    cold_min=cold_r*(1-envelope)
    hot_max=hot_r*(1+envelope)
    cold_a=1/.738-1
    hot_a=1/.339-1
    return {
        "classification":"CALCULATED_NECESSARY_CONDITION_WITH_ESTIMATED_RT_ENVELOPE",
        "cold_resistance_min_ohm":cold_min,
        "hot_resistance_max_ohm":hot_max,
        "maximum_pullup_ohm_to_block_cold_without_shunt":cold_min*cold_a,
        "minimum_pullup_ohm_to_block_hot_without_shunt":hot_max*hot_a,
        "available_resistance_ratio":cold_min/hot_max,
        "required_resistance_ratio":hot_a/cold_a,
        "necessary_condition_passes":cold_min/hot_max>=hot_a/cold_a,
        "hardware_qualified":False,
    }


if __name__=="__main__":
    print(json.dumps(feasibility(),indent=2))
