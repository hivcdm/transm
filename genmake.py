import sys
import os

class Rule(object):
    def __init__(self, targets, dependencies=[], commands=[]):
        self.targets = targets
        self.dependencies = dependencies
        self.commands = commands

    def __str__(self):
        string = ' '.join(self.targets) + ': ' + ' '.join(self.dependencies)
        if self.commands:
            string += '\n\t' + '\n\t'.join(self.commands)
        return string

class Makefile(object):
    PHONY_RULE = Rule(['.PHONY'], ['clean', 'all', 'install'])

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
        self.use_gui = False
        self.rules = [self.PHONY_RULE]
    
    def find_recursive(self, directory, extension='.cpp'):
        all = os.listdir(directory)
        files = [i for i in all if os.path.isfile(directory + os.sep + i)]
        matching = [i for i in files if i[-len(extension):] == extension]
        subdirectories = [i for i in all if os.path.isdir(directory + os.sep + i)]
        for subdirectory in subdirectories:
            matching += self.find_recursive(directory + os.sep + subdirectory, extension)
        return matching

    def generate_test_rule(self):
        pass

    def generate_module_rules(self, module):
        source_files = self.find_recursive(os.sep.join([os.getcwd(), 'source', module]))
        object_files = [i.replace('.cpp', '.o') for i in source_files]
        module_rule = Rule([module], object_files)
        object_rules = [Rule([o], [s]) for o, s in zip(object_files, source_files)]
        return [module_rule] + object_rules

    def generate_executable_rule(self, modules):
        return Rule([self.executable + self.version], dependencies=modules)

    def generate(self):
        modules = ['core',
                   'util',
                   'statistics',
                   'graphviz',
                   'entities',
                   'data',
                   'cepacbridge',
                   'cepac']
        if self.use_gui: modules.append('gui')

        self.rules.append(self.generate_executable_rule(modules))
        for module in modules:
            self.rules += self.generate_module_rules(module)

    def write(self):
        outfile = open('Makefile', 'w')

        for rule in self.rules:
            outfile.write(str(rule) + '\n\n')

if __name__ == '__main__':
    m = Makefile(sys.argv)
    m.generate()
    m.write()
