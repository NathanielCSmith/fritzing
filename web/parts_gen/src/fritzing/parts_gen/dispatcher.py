AVAIL_SCRIPTS = [
    {'value':'resistor','label':'Create a resistor'}
]

_SCRIPTS_DEF = {
    'resistor' : {
        'resistance' : {
            'label': 'Choose the resistance',
            'type' : 'int',
            'range_min' : 1
        },
        'unit' : {
            'label': 'Choose the unit',
            'type' : 'enum',
            'options' : [('kv','kl'),('mv','ml')]
        }
    } 
}

GEN_FILES_FOLDER = '/home/merun/workspace/fritzing/web/parts_gen/parts/'

def get_params_def(script_id):
    obj = _SCRIPTS_DEF[script_id] 
    return obj

def gen_files(script_id, config):
    # TODO: here execute script, and generate svg and fzp files inside the GEN_FILES_FOLDER folder 
    pass
