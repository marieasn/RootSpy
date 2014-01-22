import SCons
import os
import re
import subprocess
from glob import glob



def loadroot(env) :
	OSENV  = os.environ

        if(OSENV.has_key('ROOTSYS') and os.path.exists(OSENV['ROOTSYS'])==True):
                rootDir = OSENV['ROOTSYS']
        else:
                print "?ROOTSYS env variable not set...using -DNO_ROOTSYS"
                env.Append(CPPDEFINES = {'NO_ROOTSYS':None})
                return

	if env['SHOWENV'] == "1":
		print "Loading ROOT software from ", rootDir

	# Create Builder that can convert .h file into _Dict.cc file
	#rootsys = os.getenv('ROOTSYS', '/usr/local/root/PRO')
	incl_dir =""
	if( len(env['CPPPATH']) > 0 ):
		incl_dir = "-I" + " -I".join( env['CPPPATH'] )

	if env['SHOWBUILD']==0:
		rootcintaction = SCons.Script.Action("%s/bin/rootcint -f $TARGET -c %s $SOURCE" % (rootDir, incl_dir), 'ROOTCINT   [$SOURCE]')
	else:
		rootcintaction = SCons.Script.Action("%s/bin/rootcint -f $TARGET -c %s $SOURCE" % (rootDir, incl_dir))
	bld = SCons.Script.Builder(action = rootcintaction, suffix='_Dict.cc', src_suffix='.h')
	env.Append(BUILDERS = {'ROOTDict' : bld})

	# Generate ROOT dictionary file targets for each header
	# containing "ClassDef"
	#
	# n.b. It seems if scons is run when the build directory doesn't exist,
	# then the cwd is set to the source directory. Otherwise, it is the
	# build directory. Since the headers will only exist in the source
	# directory, we must temporarily cd into that to look for headers that
	# we wish to generate dictionaries for. (This took a long time to figure
	# out!)
	curpath = os.getcwd()
	srcpath = env.Dir('.').srcnode().abspath
	if(env['SHOWBUILD']!=0):
		print "---- Scanning for headers to generate ROOT dictionaries in: %s" % srcpath
	os.chdir(srcpath)
	for f in glob('*.h*'):
		if 'ClassDef' in open(f).read():
			env.AppendUnique(ROOTCINT_SOURCES = env.ROOTDict(f))
			if(env['SHOWBUILD']!=0):
				print "       ROOT dictionary for %s" % f
	os.chdir(curpath)
	

#  root dictionaries
        # bld = env.Builder(action = 'rootcint -f $TARGET -c -I../../libsrc $SOURCE',
        #                   suffix = '_Dict.cc',
        #                   src_suffix = '.h')
        # env.Append(BUILDERS = {'Rootcint' : bld})
        
        """
#        head = SCons.Script.Dir('.').srcnode().abspath
#	srcpath = SCons.Script.Dir('.').srcnode().abspath
	srcpath = env.Dir('.').srcnode().abspath
        headerlist = find_classdefs(srcpath)

	incl_dir =""
	if( len(env['CPPPATH']) > 0 ):
		incl_dir = "-I" + " -I".join( env['CPPPATH'] )
		
        for h in headerlist:
                base = os.path.splitext(h)[0]

		dictfile_name = base + '_Dict.cc'
		dictfile_path = srcpath + '/' + dictfile_name
		header_path = srcpath + '/' + h

		cmd = 'rootcint -f ' + dictfile_name + ' -c ' + incl_dir + ' ' + h
		if(env['SHOWBUILD']):
			print cmd
			
		subprocess.Popen(cmd.split(), cwd=srcpath)
	"""   
#                os.popen('rootcint -f ' + base + '_Dict.cc -c -I../libsrc ' + h)
#                env.Rootcint(base)
#                env.Command(base+'_Dict.cc',h,'rootcint -f $TARGET -c -I../../libsrc $SOURCE')
#                env.Command(base+'_Dict.cc',h,'touch $TARGET')
#                env.Command(os.path.join(head,base)+'_Dict.cc',os.path.join(head,h),'rootcint -f $TARGET -c -I../../libsrc $SOURCE')




#  include files
        rootincs = os.popen('$ROOTSYS/bin/root-config --incdir').readline().strip()
	env.AppendUnique(CPPPATH = rootincs)


#  library directories
        rootldir = os.popen('$ROOTSYS/bin/root-config --libdir').readline().strip()
	env.AppendUnique(LIBPATH = rootldir)


#  libs
        rootlinkflags = os.popen('$ROOTSYS/bin/root-config --noldflags --glibs').readline().strip().split()
	env.AppendUnique(LINKFLAGS = rootlinkflags)



#  returns list of header files in directory which has a root ClassDef macro in it
def find_classdefs(directory):

        result = []

        find_classdef_regex = re.compile(r'^\s*ClassDef\s*\(.*$')
        for fullname in glob(directory+'/*.h'):
                head,basename=os.path.split(fullname)
                for line in open(fullname,'r'):
                        if find_classdef_regex.match(line):
                                result.append(basename)
                                break
        return result

