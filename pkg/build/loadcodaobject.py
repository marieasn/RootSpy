import os


def loadcodaobject(env) :
	OSENV = os.environ

        if(OSENV.has_key('CODAOBJECTROOT') and os.path.exists(OSENV['CODAOBJECTROOT'])==True):
                rootDir = OSENV['CODAOBJECTROOT']
        elif(OSENV.has_key('CODAOBJECT') and os.path.exists(OSENV['CODAOBJECT'])==True):
                rootDir = OSENV['CODAOBJECT']
        else:
                print "?Neither CODAOBJECTROOT nor CODAOBJECT env variables are set...using -DNO_CODAOBJECTROOT"
                env.Append(CPPDEFINES = {'NO_CODAOBJECTROOT':None})
                return

        if env['SHOWENV'] == "1":
                print "Loading CODAOBJECT software from ", rootDir


#  include files
        codaobjectincs = []
        codaobjectincs.append(rootDir + '/include')
        env.AppendUnique(CPPPATH=codaobjectincs)


#  library directories
        codaobjectldir = []
        codaobjectldir.append(rootDir + '/lib')
        env.AppendUnique(LIBPATH = codaobjectldir)


#  libraries
        codaobjectlibs = []
        codaobjectlibs.append('codaObject')
        codaobjectlibs.append('pthread')
        codaobjectlibs.append('rt')
        codaobjectlibs.append('dl')
        env.AppendUnique(LIBS = codaobjectlibs)


#  dependencies on other packages
        from loadcmsg import loadcmsg
        loadcmsg(env)
