import os

def loadcmsg(env) :
	OSENV = os.environ

        if(OSENV.has_key('CMSGROOT') and os.path.exists(OSENV['CMSGROOT'])==True):
                rootDir = OSENV['CMSGROOT']
        elif(OSENV.has_key('CMSG') and os.path.exists(OSENV['CMSG'])==True):
                rootDir = OSENV['CMSG']
        else:
                print "?Neither CMSGROOT nor CMSG env variables are set...using -DNO_CMSGROOT"
                env.Append(CPPDEFINES = {'NO_CMSGROOT':None})
                return

        if env['SHOWENV'] == "1":
                print "Loading CMSG software from ", rootDir


#  include files
	cmsgincs = []
	cmsgincs.append(rootDir + '/include')
	env.AppendUnique(CPPPATH=cmsgincs)

#  library directories
	cmsgldir = []
	cmsgldir.append(rootDir + '/lib')
	env.AppendUnique(LIBPATH = cmsgldir)


#  libraries
	cmsglibs = []
	cmsglibs.append('cmsgxx')
	cmsglibs.append('cmsg')
	cmsglibs.append('cmsgRegex')
	cmsglibs.append('pthread')
	if( env['arch'].split('_')[0] == 'Linux' ):
		cmsglibs.append('rt')
	cmsglibs.append('dl')
	env.AppendUnique(LIBS = cmsglibs)
