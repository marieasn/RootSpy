import os

def loadjana(env) :
	OSENV = os.environ

        if(OSENV.has_key('JANA_HOME') and os.path.exists(OSENV['JANA_HOME'])==True):
                rootDir = OSENV['JANA_HOME']
        else:
                print "?JANA_HOME env variable not set...using -DNO_JANA_HOME"
                env.Append(CPPDEFINES = {'NO_JANA_HOME':None})
                return

        if env['SHOWENV'] == "1":
                print "Loading JANA software from ", rootDir


#  include files
	janaincs = []
	janaincs.append(rootDir + '/include')
	env.AppendUnique(CPPPATH=janaincs)


#  lib directories and libs
        janalinkflags = os.popen('$JANA_HOME/bin/jana-config --libs').readline().strip().split()
	env.AppendUnique(LINKFLAGS = janalinkflags)
