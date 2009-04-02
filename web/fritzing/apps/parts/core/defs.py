from django.utils.datastructures import SortedDict

AVAIL_SCRIPTS = [
    {'value':'resistor','label':'Resistor'},
    {'value':'mystery','label':'Mystery part'},
    {'value':'generic-male-header','label':'Generic male header'},
    {'value':'generic-female-header','label':'Generic female header'},
    {'value':'generic-female-header-rounded','label':'Generic female metal rounded header'},
]

SCRIPTS_DEFS = {
    'resistor' : SortedDict(),
    'mystery' : SortedDict(),
    'generic-male-header' : SortedDict(),
    'generic-female-header' : SortedDict(),
    'generic-female-header-rounded' : SortedDict(),
}

SCRIPTS_DEFS['mystery']['title'] = {
    'label'   : 'Part title',
    'type'    : 'string',
    'initial' : 'Generated Mystery Part'
}

_PINS_DICT_AUX = {
    'label': 'Number of pins',
    'type' : 'enum',
    'choices' : [
        (1,1),(2,2),(3,3),(4,4),(5,5),(6,6),(7,7),(8,8),(9,9),
        (10,10),(11,11),(12,12),(14,14),(16,16),(18,18),(20,20)
    ]    
}

SCRIPTS_DEFS['resistor']['resistance'] = {
    'label': 'Choose the resistance',
    'type' : 'regex',
    # just two significant digits
    'regex': '^(([123456789]\d0*(\.0*)?)|([1..9]\.\d0*)|(0\.[1..9]\d?0*))\s*[kKmM]?$',
    'error_messages' : {'invalid' : "You should provide a resistance with only two significant digits. You can use 'k' and 'M' as units as well."},
    'help_text' : "You should provide a resistance with only two significant digits."
}

SCRIPTS_DEFS['mystery']['pins'] = _PINS_DICT_AUX
SCRIPTS_DEFS['generic-male-header']['pins'] = _PINS_DICT_AUX
SCRIPTS_DEFS['generic-female-header']['pins'] = _PINS_DICT_AUX
SCRIPTS_DEFS['generic-female-header-rounded']['pins'] = _PINS_DICT_AUX
