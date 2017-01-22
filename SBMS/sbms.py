#
# $Id: sbms.py 21211 2016-08-16 12:45:01Z davidl $
# $HeadURL: https://halldsvn.jlab.org/repos/trunk/online/packages/SBMS/sbms.py $
#
#
import os
import subprocess
import SCons
import glob
import re
import sys

#===========================================================
# The first 4 functions provide for building a library,
# program, multiple-programs, or plugin from all the source
# in the current directory.
#
# The next section contains useful utility functions.
#
# The functions that follow in the final section add support
# for various packages (e.g. ROOT, Xerces, ...)
#===========================================================


##################################
# library
##################################
def library(env, libname=''):

	# Library name comes from directory name
	if libname=='':
		libname = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++, and FORTRAN targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	sources = env['ALL_SOURCES']

	# Build static library from all source
	myobjs = env.Object(sources)
	mylib = env.Library(target = libname, source = myobjs)

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, mylib])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for library and headers
		includedir = "%s/%s" %(env.subst('$INCDIR'), libname)
		libdir = env.subst('$LIBDIR')
		ginstalldir = env.subst('$GINSTALLDIR')
		glibdir = '%s/lib' % ginstalldir
		gincludedir = '%s/include' % ginstalldir

		# Install targets 
		env.Install(libdir, mylib)
		env.Install(includedir, env.Glob('*.h')+env.Glob('*.hh')+env.Glob('*.hpp'))
		env.Install(glibdir, mylib)
		env.Install(gincludedir, env.Glob('*.h')+env.Glob('*.hh')+env.Glob('*.hpp'))


##################################
# executable
##################################
def executable(env, exename=''):

	# Executable name comes from directory name
	if exename=='':
		exename = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++, and FORTRAN targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	# Push commonly used libraries to end of list
	ReorderCommonLibraries(env)

	sources = env['ALL_SOURCES']

	# Build program from all source
	myobjs = env.Object(sources)
	myexe = env.Program(target = exename, source = myobjs)

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, myexe])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for executable and headers
		bindir = env.subst('$BINDIR')
		ginstalldir = env.subst('$GINSTALLDIR')
		gbindir = '%s/bin' % ginstalldir

		# Install targets 
		env.Install(bindir, myexe)
		env.Install(gbindir, myexe)


##################################
# executables
##################################
def executables(env):

	# This will generate multiple executables from the
	# source in the current directory. It does this
	# by identifying source files that define "main()"
	# and linking those with all source files that do not
	# define "main()". Program names are based on the 
	# filename of the source file defining "main()"
	main_sources = []
	common_sources = []
	curpath = os.getcwd()
	srcpath = env.Dir('.').srcnode().abspath
	os.chdir(srcpath)
	files = glob.glob('*.c') + glob.glob('*.cc') + glob.glob('*.cpp')
	for f in files:
		if 'main(' in open(f).read():
			main_sources.append(f)
		else:
			common_sources.append(f)

	for f in glob.glob('*.F'):
		if '      PROGRAM ' in open(f).read():
			main_sources.append(f)
		else:
			common_sources.append(f)
	os.chdir(curpath)
	
	env.PrependUnique(CPPPATH = ['.'])

	# Push commonly used libraries to end of list
	ReorderCommonLibraries(env)

	common_sources.extend(env['ALL_SOURCES'])

	# Build program from all source
	main_objs = env.Object(main_sources)
	common_objs = env.Object(common_sources)

	progs = []
	for obj in main_objs:
		exename = re.sub('\.o$', '', str(obj))  # strip off ".o" from object file name
		progs.append(env.Program(target = exename, source = [obj, common_objs]))

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([common_objs, main_objs, progs])
	else:
		# We're in launch directory (or descendent) schedule installation
		bindir = env.subst('$BINDIR')
		ginstalldir = env.subst('$GINSTALLDIR')
		gbindir = '%s/bin' % ginstalldir

		env.Install(bindir, progs)
		env.Install(gbindir, progs)


##################################
# plugin
##################################
def plugin(env, pluginname=''):

	# Library name comes from directory name
	if pluginname=='':
		pluginname = os.path.split(os.getcwd())[1]

	env.PrependUnique(CPPPATH = ['.'])

	# Add C/C++ targets
	env.AppendUnique(ALL_SOURCES = env.Glob('*.c'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cc'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.cpp'))
	env.AppendUnique(ALL_SOURCES = env.Glob('*.F'))

	sources = env['ALL_SOURCES']

	# Build static library from all source
	myobjs = env.SharedObject(sources)
	myplugin = env.SharedLibrary(target = pluginname, source = myobjs, SHLIBPREFIX='', SHLIBSUFFIX='.so')

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([myobjs, myplugin])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for plugin and headers
		includedir = "%s/%s" %(env.subst('$INCDIR'), pluginname)
		pluginsdir = env.subst('$PLUGINSDIR')
		ginstalldir = env.subst('$GINSTALLDIR')
		gpluginsdir = '%s/plugins' % ginstalldir
		gincludedir = '%s/include' % ginstalldir

		# Install targets 
		env.Install(pluginsdir, myplugin)
		env.Install(includedir, env.Glob('*.h')+env.Glob('*.hh')+env.Glob('*.hpp'))
		env.Install(gpluginsdir, myplugin)
		env.Install(gincludedir, env.Glob('*.h')+env.Glob('*.hh')+env.Glob('*.hpp'))


##################################
# script
##################################
def script(env, scriptname, installname=None):

	# Only thing to do for script is to install it.

	# Cleaning and installation are restricted to the directory
	# scons was launched from or its descendents
	CurrentDir = env.Dir('.').srcnode().abspath
	if not CurrentDir.startswith(env.GetLaunchDir()):
		# Not in launch directory. Tell scons not to clean these targets
		env.NoClean([scriptname])
	else:
		# We're in launch directory (or descendent) schedule installation

		# Installation directories for executable and headers
		bindir = env.subst('$BINDIR')
		ginstalldir = env.subst('$GINSTALLDIR')
		gbindir = '%s/bin' % ginstalldir

		# Install targets 
		if installname==None:
			env.Install(bindir, scriptname)
			env.Install(gbindir, scriptname)
		else:
			env.InstallAs(bindir+'/'+installname, scriptname)
			env.InstallAs(gbindir+'/'+installname, scriptname)



#===========================================================
# Misc utility routines for the SBMS system
#===========================================================

##################################
# AddCompileFlags
##################################
def AddCompileFlags(env, allflags):

	# The allflags parameter should be a string containing all
	# of the link flags (e.g. what is returned by root-config --cflags)
	# It is split on white space and the parameters sorted into
	# the 2 lists: ccflags, cpppath

	ccflags = []
	cxxflags = []
	cpppath = []
	for f in allflags.split():
		if f.startswith('-I'):
			cpppath.append(f[2:])
		else:
			if f.startswith('-std=c++'):
				cxxflags.append(f)
			else:
				ccflags.append(f)
	
	if len(ccflags)>0 :
		env.AppendUnique(CCFLAGS=ccflags)

	if len(cxxflags)>0 :
		env.AppendUnique(CXXFLAGS=cxxflags)

	if len(cpppath)>0 :
		env.AppendUnique(CPPPATH=cpppath)


##################################
# AddLinkFlags
##################################
def AddLinkFlags(env, allflags):

	# The allflags parameter should be a string containing all
	# of the link flags (e.g. what is returned by root-config --glibs)
	# It is split on white space and the parameters sorted into
	# the 3 lists: linkflags, libpath, and libs

	linkflags = []
	libpath = []
	libs = []
	for f in allflags.split():
		if f.startswith('-L'):
			libpath.append(f[2:])
		elif f.startswith('-l'):
			libs.append(f[2:])
		else:
			linkflags.append(f)

	if len(linkflags)>0 :
		env.AppendUnique(LINKFLAGS=linkflags)

	if len(libpath)>0 :
		env.AppendUnique(LIBPATH=libpath)
		
	if len(libs)>0 :
		env.AppendUnique(LIBS=libs)


##################################
# ReorderCommonLibraries
##################################
def ReorderCommonLibraries(env):

	# Some common libraries are often added by multiple packages 
	# (e.g. libz is one that many packages use). The gcc4.8.0
	# compiler that comes with Ubuntu13.10 seems particularly
	# sensitive to the ordering of the libraries. This means if
	# one package "AppendUnique"s the "z" library, it may appear
	# too early in the link command for another library that needs
	# it, even though the second library tries appending it at the
	# end. This routine looks for some commonly used libraries 
	# in the LIBS variable of the given environment and moves them
	# to the end of the list.
	
	# If LIBS is not defined in this environment, return now
	if 'LIBS' not in env.Dictionary(): return

	# If LIBS is not set or is a simple string, return now
	if type(env['LIBS']) is not list: return

	libs = ['z', 'bz2', 'pthread', 'm', 'dl']
	for lib in libs:
		if lib in env['LIBS']:
			env['LIBS'].remove(lib)
			env.Append(LIBS=[lib])


##################################
# ApplyPlatformSpecificSettings
##################################
def ApplyPlatformSpecificSettings(env, platform):

	# Look for SBMS file based on this platform and run the InitENV
	# function in it to allow for platform-specific settings. Normally,
	# the BMS_OSNAME will be passed in which almost certainly contains
	# "."s. The Python module loader doesn't like these and we have to
	# replace them with "-"s to appease it.

	platform = re.sub('\.', '-', platform)

	modname = "sbms_%s" % platform
	if (int(env['SHOWBUILD']) > 0):
		print "looking for %s.py" % modname
	try:
		InitENV = getattr(__import__(modname), "InitENV")

		# Run the InitENV function (if found)
		if(InitENV != None):
			print "sbms : Applying settings for platform %s" % platform
			InitENV(env)

	except ImportError,e:
		if (int(env['SHOWBUILD']) > 0): print "%s" % e
		pass



##################################
# OptionallyBuild
##################################
def OptionallyBuild(env, dirs):

	# This is used to add directories that are not built as
	# part of the standard build, but can still be added
	# to the dependency tree so that the user can build them
	# by either invoking scons from within the specific source
	# directory or by specifying it on the command line.
	#
	# 

	subdirs = []
	for dir in dirs:
		add_dir = False
		if env.GetLaunchDir().endswith(dir): add_dir = True
		#if dir in env['COMMAND_LINE_TARGETS']: add_dir = True
		for target in env['COMMAND_LINE_TARGETS']:
			if target.endswith(dir): add_dir = True
		
		if add_dir : subdirs.extend([dir])

	if len(subdirs)>0 : env.SConscript(dirs=subdirs, exports='env', duplicate=0)


#===========================================================
# Package support follows
#===========================================================


##################################
# JANA
##################################
def AddJANA(env):
	jana_home = os.getenv('JANA_HOME', '/no/default/jana/path')
	JANA_CFLAGS = subprocess.Popen(["%s/bin/jana-config" % jana_home, "--cflags"], stdout=subprocess.PIPE).communicate()[0]
	JANA_LINKFLAGS = subprocess.Popen(["%s/bin/jana-config" % jana_home, "--libs"], stdout=subprocess.PIPE).communicate()[0]

	AddCompileFlags(env, JANA_CFLAGS)
	AddLinkFlags(env, JANA_LINKFLAGS)


##################################
# HDDS
##################################
def AddHDDS(env):
	hdds_home = os.getenv('HDDS_HOME', 'hdds')
	env.AppendUnique(CPPPATH = ["%s/src" % hdds_home])
	env.AppendUnique(LIBPATH = ["%s/lib/%s" % (hdds_home, env['OSNAME'])])


##################################
# HDDM
##################################
def AddHDDM(env):
	env.AppendUnique(LIBS = 'HDDM')
	Add_xstream(env)


##################################
# DANA
##################################
def AddDANA(env):
	halld_my = os.getenv('HALLD_MY')
	halld_home = os.getenv('HALLD_HOME')
	if halld_my != None:
		env.AppendUnique(CPPPATH=['%s/%s/include' % (halld_my  , env['OSNAME'])])
		env.AppendUnique(LIBPATH=['%s/%s/lib' % (halld_my  , env['OSNAME'])])
	if halld_home != None:
		env.AppendUnique(CPPPATH=['%s/%s/include' % (halld_home, env['OSNAME'])])
		env.AppendUnique(LIBPATH=['%s/%s/lib' % (halld_home, env['OSNAME'])])
	AddHDDM(env)
	AddROOT(env)
	AddJANA(env)
	AddCCDB(env)
	AddHDDS(env)
	AddXERCES(env)
	AddEVIO(env)
	AddET(env)
	AddCODAChannels(env)
	DANA_LIBS  = "DANA ANALYSIS PID TAGGER TRACKING START_COUNTER"
	DANA_LIBS += " CERE DIRC CDC TRIGGER PAIR_SPECTROMETER RF"
	DANA_LIBS += " FDC TOF BCAL FCAL CCAL FMWPC TPOL"
	DANA_LIBS += " EVENTSTORE HDGEOMETRY TTAB DAQ KINFITTER JANA"
	env.PrependUnique(LIBS = DANA_LIBS.split())

##################################
# xstream
##################################
def Add_xstream(env):
	env.AppendUnique(CPPPATH = ['#external/xstream/include'])
	env.AppendUnique(CCFLAGS = ['-fPIC'])
	env.AppendUnique(LIBS=['xstream', 'bz2', 'z'])


##################################
# CCDB
##################################
def AddCCDB(env):
	ccdb_home = os.getenv('CCDB_HOME', 'ccdb')
	CCDB_CPPPATH = "%s/include" % (ccdb_home)
	CCDB_LIBPATH = "%s/lib" % (ccdb_home)
	CCDB_LIBS = "ccdb"
	env.AppendUnique(CPPPATH = CCDB_CPPPATH)
	env.AppendUnique(LIBPATH = CCDB_LIBPATH)
	env.AppendUnique(LIBS    = CCDB_LIBS)


##################################
# EVIO
##################################
def AddEVIO(env):
	evioroot = os.getenv('EVIOROOT')
	if(evioroot != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_EVIO'])
		env.AppendUnique(CPPPATH = ['%s/include' % evioroot])
		env.AppendUnique(LIBPATH = ['%s/lib' % evioroot])
		env.AppendUnique(LIBS=['evioxx', 'evio', 'expat', 'dl'])
		AddET(env)


##################################
# ET
##################################
def AddET(env):

	# Only add ET if ETROOT is set
	etroot = os.getenv('ETROOT')
	if(etroot != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_ET'])
		env.AppendUnique(CPPPATH = ['%s/include' % etroot])
		env.AppendUnique(LIBPATH = ['%s/lib' % etroot])
		env.AppendUnique(LIBS=['et', 'et_remote', 'dl'])
		if os.getenv('OSTYPE', 'none') != 'darwin': AddLinkFlags(env,'-lrt')


##################################
# CodaChannels
##################################
def AddCODAChannels(env):

	# Only add codaChannels library if CODA is set
	coda = os.getenv('CODA')
	arch = os.getenv('ARCH')
	if(coda != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_CODACHANNELS'])
		env.AppendUnique(CPPPATH = ['%s/%s/include' % (coda,arch)])
		env.AppendUnique(LIBPATH = ['%s/%s/lib' % (coda,arch)])
		env.AppendUnique(LIBS=['codaChannels'])


##################################
# CodaObjects
##################################
def AddCODAObjects(env):

	# Only add codaObject library if CODA is set
	codaobject = os.getenv('CODAOBJECTROOT')
	if(codaobject != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_CODAOBJECTS'])
		env.AppendUnique(CPPPATH = ['%s/include' % codaobject])
		env.AppendUnique(LIBPATH = ['%s/lib' % codaobject])
		env.AppendUnique(LIBS=['codaObject'])


##################################
# CMSG
##################################
def AddCMSG(env):

	# Only add cMsg if CMSGROOT is set
	cmsgroot = os.getenv('CMSGROOT')
	if(cmsgroot != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_CMSG'])
		env.AppendUnique(CPPPATH = ['%s/include' % cmsgroot])
		env.AppendUnique(LIBPATH = ['%s/lib' % cmsgroot])
		env.AppendUnique(LIBS=['cmsgxx', 'cmsg', 'cmsgRegex', 'dl', 'pthread'])
		if os.getenv('OSTYPE', 'none') != 'darwin': AddLinkFlags(env,'-lrt')


##################################
# Xerces
##################################
def AddXERCES(env):
	xercescroot = os.getenv('XERCESCROOT', 'xerces')
	XERCES_CPPPATH = "%s/include" % (xercescroot)
	XERCES_LIBPATH = "%s/lib" % (xercescroot)
	XERCES_LIBS = "xerces-c"
	env.AppendUnique(CPPPATH = XERCES_CPPPATH)
	env.AppendUnique(LIBPATH = XERCES_LIBPATH)
	env.AppendUnique(LIBS    = XERCES_LIBS)


##################################
# CERNLIB
##################################
def AddCERNLIB(env):
	env.PrependUnique(FORTRANFLAGS = ['-ffixed-line-length-0', '-fno-second-underscore'])
	env.PrependUnique(FORTRANFLAGS = ['-fno-automatic'])
	env.PrependUnique(FORTRANPATH = ['include'])
	cern = os.getenv('CERN', '/usr/local/cern/PRO')
	cern_level = os.getenv('CERN_LEVEL', '2006')
	cern_root = '%s/%s' % (cern, cern_level)
	CERN_FORTRANPATH = "%s/include" % cern_root
	CERN_LIBPATH = "%s/lib" % cern_root
	env.AppendUnique(FORTRANPATH   = [CERN_FORTRANPATH])
	env.AppendUnique(CPPPATH   = CERN_FORTRANPATH)
	env.AppendUnique(LIBPATH   = CERN_LIBPATH)
	env.AppendUnique(LINKFLAGS = ['-rdynamic'])
	env.AppendUnique(LIBS      = ['gfortran', 'geant321', 'pawlib', 'lapack3', 'blas', 'graflib', 'grafX11', 'packlib', 'mathlib', 'kernlib', 'X11', 'nsl', 'crypt', 'dl'])
	env.SetOption('warn', 'no-fortran-cxx-mix')  # supress warnings about linking fortran with c++


##################################
# ROOT
##################################
def AddROOT(env):
	#
	# Here we use the root-config program to give us the compiler
	# and linker options needed for ROOT. We use the AddCompileFlags()
	# and AddLinkFlags() routines (defined above) to split the arguments
	# into the categories scons wants. E.g. scons wants to know the
	# search path and basenames for libraries rather than just giving it
	# the full compiler options like "-L/path/to/lib -lmylib".
	#
	# We also create a builder for ROOT dictionaries and add targets to
	# build dictionaries for any headers with "ClassDef" in them.

	rootsys = os.getenv('ROOTSYS', '/usr/local/root/PRO')
	if not os.path.isdir(rootsys):
		print 'ROOTSYS not defined or points to a non-existant directory!'
		sys.exit(-1)

	# Only root-config the first time through
	if "ROOT_CFLAGS" not in AddROOT.__dict__:
		AddROOT.ROOT_CFLAGS    = subprocess.Popen(["%s/bin/root-config" % rootsys, "--cflags"], stdout=subprocess.PIPE).communicate()[0]
		AddROOT.ROOT_LINKFLAGS = subprocess.Popen(["%s/bin/root-config" % rootsys, "--glibs" ], stdout=subprocess.PIPE).communicate()[0]

	AddCompileFlags(env, AddROOT.ROOT_CFLAGS)
	AddLinkFlags(env, AddROOT.ROOT_LINKFLAGS)
	env.AppendUnique(LIBS = "Geom")
	if os.getenv('LD_LIBRARY_PATH'  ) != None : env.Append(LD_LIBRARY_PATH   = os.environ['LD_LIBRARY_PATH'  ])
	if os.getenv('DYLD_LIBRARY_PATH') != None : env.Append(DYLD_LIBRARY_PATH = os.environ['DYLD_LIBRARY_PATH'])

	# NOTE on (DY)LD_LIBRARY_PATH :
	# Linux (and most unixes) use LD_LIBRARY_PATH while Mac OS X uses
	# DYLD_LIBRARY_PATH. Unfortunately, the "thisroot.csh" script distributed
	# with ROOT sets both of these so we can't use the presence of the
	# DYLD_LIBRARY_PATH environment variable to decide which of these to 
	# work with. Thus, we just append to whichever are set, which may be both.

	# If additional include dirctories need to be added to the search path
	# when generating a dictionary, they can be via the ROOT_DICT_INCLUDE_PATH
	# variable
	inc_path = ''
	if 'ROOT_DICT_INCLUDE_PATH' in env.Dictionary().keys():
		for p in env['ROOT_DICT_INCLUDE_PATH']: inc_path += '-I'+p+' '

	# Create Builder that can convert .h file into _Dict.cc file
	if os.getenv('LD_LIBRARY_PATH'  ) != None : env.AppendENVPath('LD_LIBRARY_PATH'  , '%s/lib' % rootsys )
	if os.getenv('DYLD_LIBRARY_PATH') != None : env.AppendENVPath('DYLD_LIBRARY_PATH', '%s/lib' % rootsys )
	rootcintpath  = "%s/bin/rootcint" % (rootsys)
	rootclingpath = "%s/bin/rootcling" % (rootsys)
	if env['SHOWBUILD']==0:
		rootcintaction  = SCons.Script.Action("%s %s-f $TARGET -c $SOURCE" % (rootcintpath,  inc_path) , 'ROOTCINT   [$SOURCE]')
		rootclingaction = SCons.Script.Action("%s %s-f $TARGET -c $SOURCE" % (rootclingpath, inc_path), 'ROOTCLING  [$SOURCE]')
	else:
		rootcintaction  = SCons.Script.Action("%s %s-f $TARGET -c $SOURCE" % (rootcintpath,  inc_path) )
		rootclingaction = SCons.Script.Action("%s %s-f $TARGET -c $SOURCE" % (rootclingpath, inc_path))
	if os.path.exists(rootclingpath) :
		bld = SCons.Script.Builder(action = rootclingaction, suffix='_Dict.cc', src_suffix='.h')
	elif os.path.exists(rootcintpath):
		bld = SCons.Script.Builder(action = rootcintaction, suffix='_Dict.cc', src_suffix='.h')
	else:
		print 'Neither rootcint nor rootcling exists. Unable to create ROOT dictionaries if any encountered.'
		return

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
	if(int(env['SHOWBUILD'])>1):
		print "---- Scanning for headers to generate ROOT dictionaries in: %s" % srcpath
	os.chdir(srcpath)
	for f in glob.glob('*.[h|hh|hpp]'):
		if 'ClassDef' in open(f).read():
			env.AppendUnique(ALL_SOURCES = env.ROOTDict(f))
			if(int(env['SHOWBUILD'])>1):
				print "       ROOT dictionary for %s" % f
	os.chdir(curpath)



##################################
# ROOTSPY
##################################
def AddROOTSpy(env):
	rootspy = os.getenv('ROOTSPY')
	if(rootspy != None) :
		env.AppendUnique(CXXFLAGS = ['-DHAVE_ROOTSPY'])
		env.AppendUnique(CPPPATH = ['%s/include' % rootspy, '%s/include/libRootSpy' % rootspy])
		env.AppendUnique(LIBPATH = ['%s/lib' % rootspy])
		env.AppendUnique(LIBS=['RootSpy'])
		AddROOT(env)


##################################
# ROOTSPY Macro build function
##################################
def RootSpyMacroCodeGen(target, source, env):
	# Used by AddROOTSpyMacros below. See comments there for details
	t = '%s' % target[0]
	s = '%s' % source[0]

	base = os.path.basename(s[:-2])
	class_name = '%s_rootspy_class' % base
	fin  = open(s)
	fout = open(t, 'w')

	fout.write('#include <dlfcn.h>\n')
	fout.write('#include <iostream>\n')
	fout.write('#include <string>\n')
	fout.write('using namespace std;\n')
	fout.write('static string macro_data=""\n')
	for line in fin:
		line = line.replace('"', '\\\"')
		line = line.replace('\r', '')
		line = line.replace('\\#', '\\\#')  # used when macro actually wants a "#" in title
		fout.write('"%s\\n"\n' % line[:-1])
	fout.write(';\n')
	fout.write('class %s{\n' % class_name)
	fout.write('   public:\n')
	fout.write('   typedef void rmfcn(string, string, string);\n')
	fout.write('   %s(){\n' % class_name)
	fout.write('      rmfcn* fcn = (rmfcn*)dlsym(RTLD_DEFAULT, "REGISTER_ROOTSPY_MACRO");\n')
	fout.write('      if(fcn) (*fcn)("%s","/", macro_data);\n' % base)
	fout.write('   }\n')
	fout.write('};\n')
	fout.write('static %s tmp;' % class_name)

	fin.close()
	fout.close()

##################################
# ROOTSPY Macros
##################################
def AddROOTSpyMacros(env):
	#
	# This is used to generate a C++ file for each ROOT macro file
	# where the complete macro file is embedded as a string. A small
	# piece of code is also inserted that allows the string to be
	# automatically registered with the RootSpy system, if present.
	# (Basically, if the rootspy plugin is attached.) ROOT macros
	# are identified by a file ending with a ".C" suffix. The macro
	# name will be the basename of the file.
	#

	# Create Builder that can convert .C file into _rootspy_macro.cc file
	if env['SHOWBUILD']==0:
		rootmacroaction = SCons.Script.Action(RootSpyMacroCodeGen, 'ROOTSPY    [$SOURCE]')
	else:
		rootmacroaction = SCons.Script.Action(RootSpyMacroCodeGen)
	bld = SCons.Script.Builder(action = rootmacroaction, suffix='_rootspy_macro.cc', src_suffix='.C')
	env.Append(BUILDERS = {'ROOTSpyMacro' : bld})

	# Find all macro files and schedule them to be converted using the above builder
	curpath = os.getcwd()
	srcpath = env.Dir('.').srcnode().abspath
	if(int(env['SHOWBUILD'])>1):
		print "---- Looking for ROOT macro files (*.C) in: %s" % srcpath
	os.chdir(srcpath)
	for f in glob.glob('*.C'):
		env.AppendUnique(ALL_SOURCES = env.ROOTSpyMacro(f))
		if(int(env['SHOWBUILD'])>1) : print "       ROOTSpy Macro for %s" % f

	os.chdir(curpath)


##################################
# EPICS ezca
##################################
def AddEPICS(env):
	#
	# The EZCA library is an extension of EPICS and so
	# may be installed in the extensions folder inside
	# EPICS_BASE. It also could be installed into 
	# EPICS_BASE directly or not at all. To accomodate
	# all cases, look for the libezca library in
	# both places and only define HAVE_EZCA if it is
	# found.
	#
	epics_base = os.getenv('EPICS_BASE')
	if epics_base == None: return
	EpicsHostArch = epics_base+'/startup/EpicsHostArch'
	arch = subprocess.Popen([EpicsHostArch], stdout=subprocess.PIPE).communicate()[0].rstrip()
	uname = subprocess.Popen(['uname'], stdout=subprocess.PIPE).communicate()[0].rstrip()

	libdirs = [epics_base+'/lib/'+arch, epics_base+'/../extensions/lib/'+arch]
	incdirs = [epics_base+'/include', epics_base+'/../extensions/include']
	incdirs.append(epics_base+'/include/os/'+uname)
	
	# The LD_LIBRARY_PATH may be setup for 32-bit libraries
	# even when running on a 64-bit machine. This is how it is done
	# on the gluon machines, but is likely not done that way everywhere.
	# To make this robust against run time environments, only compile
	# against the static ezca library. Other EPICS libraries are also
	# needed so we do a search for static versions of each of them 
	# in all directories in libdirs
	libs = ['ezca', 'ca', 'Com']
	libpaths = {}
	for libdir in libdirs:
		for lib in libs:
			libpath = libdir+'/lib'+lib+'.a'
			if os.path.exists(libpath): libpaths[lib] = libpath
	
	# Add static libraries as scons "File" objects so they are handled properly
	if len(libpaths) == len(libs):
		for lib,libpath in libpaths.iteritems(): env.AppendUnique(LIBS=[env.File(libpath)])
		env.AppendUnique(CXXFLAGS = ['-DHAVE_EZCA'])
		env.AppendUnique(CPPPATH = incdirs)
		env.AppendUnique(LIBS=['readline'])
	else:
		print 'Unable to find all libs! EPICS ezca support disabled'
		print 'looking for:'
		print libs
		print 'found:'
		print libpaths


##################################
# MYSQL
##################################
def AddMYSQL(env):
	try:
		MYSQL_CFLAGS = subprocess.Popen(["mysql_config", "--cflags"], stdout=subprocess.PIPE).communicate()[0]
		MYSQL_LIBS   = subprocess.Popen(["mysql_config", "--libs"], stdout=subprocess.PIPE).communicate()[0]
		env.AppendUnique(CXXFLAGS = ['-DHAVE_MYSQL'])
		AddCompileFlags(env, MYSQL_CFLAGS)
		AddLinkFlags(env, MYSQL_LIBS)
	except:
		print '=== Failed to run mysql-config. MYSQL support disabled. ==='
		


