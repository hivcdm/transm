import cx_Freeze

exe_args = { 'script': '',
             'initScript': '',
             'base': 'Win32GUI',
             'path': '',
             'targetDir': '',
             'targetName': '',
             'includes': '',
             'excludes': '',
             'packages': '',
             'replacePaths': '',
             'compress': True,
             'copyDependentFiles': True,
             'appendScriptToExe': False,
             'appendScriptToLibrary': False,
             'icon': '',
             'namespacePackages': '',
             'shortcutName': '',
             'shortcuteDir': '' }

executable = cx_Freeze.Executable(**exe_args)

setup_args = { 'name': 'tracer',
               'version': '0.2.0',
               'description': 'GUI for manipulating CEPAC transmission trace data',
               'author': 'John Evans',
               'author_email': 'jgevans at partners dot org',
               'maintainer': 'Thomas Fussell',
               'maintainer_email': 'fussell at hsph dot harvard dont edu',
               'url': 'http://web2.research.partners.org/cepac',
               'executables': [executable] }

cx_Freeze.setup(**setup_args)
