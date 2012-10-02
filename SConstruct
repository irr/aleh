import os
env = Environment(ENV = os.environ,
                  CCFLAGS='-g -O3 -std=c++0x',
                  CPPPATH=['/usr/local/include/hiredis'],
                  LIBPATH=['/usr/local/lib'])
targets = {"aleh" : "aleh.cc"}
common_sources = []
CommonObjects = env.Object(common_sources)
for target, file in targets.iteritems():
    env.Program(target = target, source = [file, CommonObjects],
                LINKFLAGS = '',
                LIBS = ['event', 'hiredis', 'pthread', 'rt'])
env.Clean('.', '.sconsign.dblite')