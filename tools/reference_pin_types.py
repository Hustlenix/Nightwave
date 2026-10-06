"""Electrical ERC model for the reference, not component qualification.

Basis: each component's manufacturer pin-function table linked in the manifest.
Mode-dependent I/O remains bidirectional unless the reference explicitly fixes
the role. Switch nodes/charge pumps and bidirectional battery paths are passive
because a KiCad power-out declaration would misrepresent their operation.
"""


def groups(**types):
    result = {}
    for kind, pins in types.items():
        for pin in pins.split():
            if pin in result:
                raise ValueError(f"Duplicate pin {pin}")
            result[pin] = kind
    return result


PIN_TYPES = {
    'U1': groups(power_in='1 2 40 41', input='3 9 16 22 23 36 38',
        output='4 5 6 7 8 10 11 17 24 25 31 32 33 34 35 37 39',
        bidirectional='12 13 14 15 18 19 20 21 26 27 28 29 30'),
    'U3': groups(power_in='15 18', power_out='2 5 9 17',
        open_collector='3 10 11', input='4 6 7 13 14', bidirectional='12', passive='1 8 16'),
    'U4': groups(power_in='1 2 4 5 9', input='3', passive='6 8', power_out='7'),
    'U5': groups(input='1 2 4', power_in='3 8 10', power_out='6', open_collector='5', passive='7 9'),
    'U6': groups(input='1 2 4', power_in='3 8 10', power_out='6', open_collector='5', passive='7 9'),
    'U7': groups(input='1 2 6 7', power_in='3 4 9', open_collector='5', bidirectional='8'),
    'U8': groups(power_in='1 3 8 9 19 20', passive='2 4', power_out='5 18',
        output='6 7', input='10 11 12 13 14 15 16 17'),
    'U9': groups(input='1 2 3 4 6 7 13', output='5 16', power_in='10 14 15 17',
        power_out='8 12', passive='9 11'),
    'U10': groups(input='1 2 4 14 16', power_in='3 7 8 11 15 17',
        output='9 10', no_connect='5 6 12 13'),
    'U11': groups(open_collector='1', input='2 3 21 22', power_in='12 24',
        bidirectional='4 5 6 7 8 9 10 11 13 14 15 16 17 18 19 20 23'),
    'U12': groups(input='1 9 10 11 12 13 14 18 19 26 27 28 43',
        output='4 5 6 8 17', passive='7', power_in='16 22 23 50 56 57',
        power_out='15 24 25', open_collector='32 34',
        bidirectional='2 3 20 21 29 30 31 33 35 36 37 38 39 40 41 42 44 45 46 47 48 49'),
}


def pin_type(reference, number):
    if reference.startswith('U'):
        return PIN_TYPES[reference][number]  # Missing active pin is an error.
    return 'passive'  # Physical contacts, passives, test pads.
