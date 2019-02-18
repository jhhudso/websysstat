import glob
import sys
import os.path

env = Environment(CCFLAGS = '-ggdb3 -Wall -O3',
                  LINKFLAGS = '-ggdb3 -Wall -O3',
		  CPPPATH = ['.'])
base_src = glob.glob('*.cpp')
env.Program('websysstat', [base_src], LIBS=['httpserver','json-c'], LIBPATH=['.','/usr/local/lib']);
