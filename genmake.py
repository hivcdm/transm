class Rule(object):
    def __init__(self):
        self.targets = []
        self.dependencies = []
        self.commands = []

    def __str__(self):
        string = ' '.join(target) + ': ' + ' '.join(dependencies)
        if self.commands:
            string += '\n\t' + '\n\t'.join(self.commands)
        return string

class Makefile(object):
    PHONY_RULE = Rule('.PHONY', ['clean', 'all', 'install'])

    def __init__(self, params):
        self.source_dir = 'source'
        self.build_dir = 'build'
        self.bin_dir = 'bin'
        self.include = []
        self.libraries = []
        self.compiler = 'g++'
        self.linker = 'ld'
        self.version = '332'
        self.executable = 'transm'
    
    def find_recursive(self, directory, extension='.cpp'):
        all = os.listdir(directory)
        files = [i for i in matching if os.isfile(i)]
        matching = [i for i in files if all[-len(extension):] == extension]
        subdirectories = [i for i in all if os.isdir(i)]
        for subdirectory in subdirectory:
            matching += find_recursive(subdirectory, extension)
        return matching

    def generate_test_rule(self):
        

    def generate_module_rule(self, module):
        source_files = find_recursive(module)

    def generate(self):
        modules = ['core',
                   'util',
                   'statistics',
                   'graphviz',
                   'entities',
                   'data',
                   'cepacbridge',
                   'cepac']
        if use_gui: modules.append('gui')

        self.rules += [self.generate_module_rule(m) for m in modules]
        self.rules.append(self.build_phony_rule())

    def write(self):
        outfile = open('Makefile', 'w')

        for rule in self.rules:
            outfile.write(str(rule))

if __name__ == '__main__':
    Makefile(sys.argv).generate()
