#  rs_lib.py
#  functions for building RootSpy - adapted from Hall D online SCONS build system
#  sdobbs, 16-oct-2013
#  adapted from ejw, 7-may-2013


import os
import re
from glob import glob
import SCons
import commands
from os import path
from util import cmloptions, loadoptions



#  The Hall D generic SConstruct file calls init_rootspy_env and build_rootspy (see below)
#
#  The system builds doc, scripts, java, and src/libsrc, src/binsrc, src/plugins and src/test
#  Note:  recursively builds doc,scripts,binsrc,plugins,test all the way down the directory hierarchy.
#
#  Intermediate build files for C and C++ stored in architecture-dependent hidden directories in src/libsrc, etc.
#
#  Gets C/C++ compiler from PATH, uses -std=c++11 by default  -  CHANGE THIS
#     Can override with c++11=N (or n or 0) on command line
#
#  Gets global CXXFLAGS,CCFLAGS,CPPPATH,LINKFLAGS first from command line, then from environment variables
#  
#  By default installs c and c++ into <package>/arch/lib, bin, include dirs
#  Exception:  test ALWAYS installs into <package>/arch/bin
#
#
#  Notes:
#     must qualify all includes with the package name, e.g. #include <myPackage/xyz.hxx> 
#     must have link in libsrc to itself with name = package name,  e.g: (cd src/libsrc; ln -s . myPackage)
#     use "java" target to build java code ($scons java) (bug in SCONS?)
#
#
#  Usage:
#      scons [-c] [debug=(1,y,yes,Y,Yes,YES)] [prof=(1,y,yes,Y,Yes,YES)] [opt=level] [xxxFLAGS=yyy] [install] [java]
#     
#        where xxxFLAGS can be CXXFLAGS, CCFLAGS, CPPPATH and/or LINKFLAGS 
#     
#     
#  Examples:  
#        $ scons debug=1                           # sets -Og
#        $ scons prof=yes                          # sets -pg
#        $ scons opt=g                             # sets -Og
#        $ scons opt=3                             # sets -O3
#        $ scons CXXFLAGS=-Wall                    # use -Wall flag when compiling C++
#        $ scons C++11=0                           # do not use -std=c++11 (can also use N and n)
#        $ scons C++11=y                           # use -std=c++11 (can also use Y and 1)
#
#        $ scons install                           # install into package root directory
#        $ scons -c install                        # uninstall from package root directory
#
#        $ scons java                              # build java instead of C and C++
#        $ scons -c java                           # uninstall java
#     
#
#
#  Uses Maurizio's scons util library to implement command-line option stuff
#
#
#  ejw, 10-may-2013
#
#
#
#  Still to do:
#
#   get "$scons install java" to work, or get straight about defaults
#   improve help and cmloptions()
#   modify loadxxx() files for package dependent debug strategy
#   must arch dirs be created explicitely?



def init_rootspy_env(opts):

    # handle command line options, help, etc.
    cmloptions(opts)


    #  create environment, import all env vars, get g++ from path
    env = SCons.Environment.Environment(ENV = os.environ, options = opts)
    
    
    #  import compile or link flags first from command line, then from env vars
    flagList = 'CXXFLAGS CCFLAGS CPPPATH LINKFLAGS'
    for x in flagList.split(' '):
        if SCons.Script.ARGUMENTS.get(x,0):
            exec("env.AppendUnique(" + x + " = SCons.Script.ARGUMENTS.get('" + x + "',0))")
        elif x in env['ENV']:
            exec("env.AppendUnique(" + x + " = (env['ENV'])['" + x + "'])")
            
            
            
    #  set architecture
    arch = commands.getoutput('osrelease.pl')
    env.Replace(arch = arch)
    print '  --> Building for architecture ' + arch + ' <--'
    

    #  get package root, which is the current working dir since this is called by SConstruct (n.b. this is the full path)
    package_root = SCons.Script.Dir('.').srcnode().abspath
    env.Replace(package_root = package_root)

    
    #  get package name
    packageName = package_root.split('/')[len(package_root.split('/'))-1]
    env.Replace(packageName = packageName)
            
            
    #  if requested, set C++11 flag 
    if (SCons.Script.ARGUMENTS.get('C++11',0) and
        ((SCons.Script.ARGUMENTS.get('C++11',0)=='1') or (SCons.Script.ARGUMENTS.get('C++11',0)[0]=='y') 
         or (SCons.Script.ARGUMENTS.get('C++11',0)[0]=='Y'))):
        env.PrependUnique(CXXFLAGS     = ['-std=c++11'])


    #  always include symbol table
    env.PrependUnique(CXXFLAGS     = ['-g'])
    env.PrependUnique(CCFLAGS      = ['-g'])
    env.PrependUnique(CPPPATH      = ['.'])
            

    # add fix to make OS X link shared libs more like Linux does
    # suppresses undefined symbol errors
    if( env['arch'].split('_')[0] == 'Darwin' ):
        env.AppendUnique(LINKFLAGS = '-undefined')
        env.AppendUnique(LINKFLAGS = 'dynamic_lookup')

    # ensure that Debian/Ubuntu does not pass '--as-needed' to the linker
    # which causes linking to be very sensitive to library ordering
    # needed for ROOT libs, at least
    # this change was implemented in upstream Debian around Ubuntu 12.10
    if( (env['arch'].split('_')[1].startswith('Debian')) or (env['arch'].split('_')[1].startswith('Ubuntu')) ):
        env.AppendUnique(LINKFLAGS = '-Wl,--no-as-needed')


    #  add DBG to env, check for debug command line arg
    dbg=False
    if (SCons.Script.ARGUMENTS.get('debug',0) and 
        ((SCons.Script.ARGUMENTS.get('debug',0)=='1') or (SCons.Script.ARGUMENTS.get('debug',0)[0]=='y') 
         or (SCons.Script.ARGUMENTS.get('debug',0)[0]=='Y'))):
        dbg=True
        env.AppendUnique(CXXFLAGS  = '-Og')
        env.AppendUnique(CCFLAGS   = '-Og')
        env.AppendUnique(LINKFLAGS = '-Og')
    env.Replace(dbg = dbg)


    #  add PROF to env, check for prof command line arg
    prof=False
    if (SCons.Script.ARGUMENTS.get('prof',0) and 
        ((SCons.Script.ARGUMENTS.get('prof',0)=='1') or (SCons.Script.ARGUMENTS.get('prof',0)[0]=='y') 
         or (SCons.Script.ARGUMENTS.get('prof',0)[0]=='Y'))):
        prof=True
        env.AppendUnique(CXXFLAGS  = '-pg')
        env.AppendUnique(CCFLAGS   = '-pg')
        env.AppendUnique(LINKFLAGS = '-pg')
    env.Replace(prof = prof)


    #  check for optimization flag
    #  default optimization level is -O0 for g++, if debug on will use -Og
    if(SCons.Script.ARGUMENTS.get('opt',0)):
        level='-O'+SCons.Script.ARGUMENTS.get('opt',0)
        env.AppendUnique(CXXFLAGS  = level)
        env.AppendUnique(CCFLAGS   = level)
        env.AppendUnique(LINKFLAGS = level)


    #  check for java flag, check for HALLD_INSTALL_DIR, set special Java variables
    java=False
    if('java' in SCons.Script.COMMAND_LINE_TARGETS):
        java=True
        java_source = 'org'
        local_java_target = 'build/classes'
        local_jar_target  = 'build/lib/' + packageName + '.jar'

        #if SCons.Script.ARGUMENTS.get('HALLD_INSTALL_DIR',0):
        #    java_target = SCons.Script.ARGUMENTS.get('HALLD_INSTALL_DIR',0) + '/builds/classes'
        #    jar_target  = SCons.Script.ARGUMENTS.get('HALLD_INSTALL_DIR',0) + '/builds/lib/' + packageName + '.jar'
        #elif(os.environ.has_key('HALLD_INSTALL_DIR')):
        #    java_target = os.environ['HALLD_INSTALL_DIR'] + '/builds/classes'
        #    jar_target  = os.environ['HALLD_INSTALL_DIR'] + '/builds/lib' + packageName + '.jar'
        #else:
        java_target = local_java_target
        jar_target  = local_jar_target
        env.Replace(java_source = java_source)
        env.Replace(local_java_target = local_java_target, local_jar_target = local_jar_target)
        env.Replace(java_target = java_target, jar_target = jar_target)
    env.Replace(java = java)


    #  create build directories if needed
    # if((os.path.exists("./src/binsrc/")==True) and (os.path.exists("./src/binsrc/." + arch)==False)):
    #     print "creating build dir ./src/binsrc/." + arch
    #     os.makedirs("./src/binsrc/." + arch)
    if((os.path.exists("src/libsrc/")==True) and (os.path.exists("src/libsrc/." + arch)==False)):
        print "creating build dir src/libsrc/." + arch
        os.makedirs("src/libsrc/." + arch)
    # if((os.path.exists("./src/plugins/")==True) and (os.path.exists("./src/plugins/." + arch)==False)):
    #     print "creating build dir ./src/plugins/." + arch
    #     os.makedirs("./src/plugins/." + arch)
    # if((os.path.exists("./src/test/")==True) and (os.path.exists("./src/test/." + arch)==False)):
    #     print "creating build dir ./src/test/." + arch
    #     os.makedirs("./src/test/." + arch)


    #  set installation dirs, check HALLD_INSTALL_DIR command line option and environment variable
    local_install_dir = path.join(package_root,arch)
    #if SCons.Script.ARGUMENTS.get('HALLD_INSTALL_DIR',0):
    #    install_root = SCons.Script.ARGUMENTS.get('HALLD_INSTALL_DIR',0)
    #    install_dir  = path.join(install_root,arch)
    #elif(os.environ.has_key('HALLD_INSTALL_DIR')):
    #    install_root = os.environ['HALLD_INSTALL_DIR']
    #    install_dir  = path.join(install_root,arch)
    #else:
    install_root = package_root
    install_dir=local_install_dir
    env.Replace(install_root = install_root, install_dir = install_dir, local_install_dir = local_install_dir)

    #  allow for cleaning one package at a time, if you are in that directory
    CurrentDir = env.Dir('.').srcnode().abspath 
    if not CurrentDir.startswith(env.GetLaunchDir()): 
        env.NoClean([myobjs, mylib, installed]) 

    #  setup standard options and help facility
    SCons.Script.Help(opts.GenerateHelpText(env))
    loadoptions(env)


    #  done
    return(env)



#  Performs build in subdirs
#  Only installs scripts for non-default install_dir
#  Package include and library must be first in CPPPATH and LIBPATH for binsrc and test
#  Note that binsrc and test both install into bin directory, but test always into local install dir
#  For some reason Java has to be built separately from other stuff
def build_rootspy(env):

    #  check if any files in lib dir
    libList = env.Glob('src/libsrc/*.cc')+env.Glob('src/libsrc/*.c')


    if(not env['java']):

        if(os.path.exists("doc")==True): 
            walkDocDirs('docs',env.Clone())


        if(os.path.exists("scripts")==True): 
            walkScriptDirs('scripts',env.Clone())


        if(os.path.exists("src/libsrc")==True): 
            libEnv=env.Clone()
            SCons.Script.SConscript('src/libsrc/SConscript',exports=['libEnv'], 
                                    variant_dir='src/libsrc/.' + env['arch'], duplicate=0)


        if(os.path.exists("src/plugins")==True): 
            pluginEnv=env.Clone()
            pluginEnv.PrependUnique(CPPPATH     = [env['package_root'] + '/src/libsrc'])
            pluginEnv.PrependUnique(LIBPATH     = [env['package_root'] + '/src/libsrc/.' + env['arch']])
            if(len(libList)>0):
                pluginEnv.PrependUnique(LIBS = [env['packageName']])
            pluginEnv.Replace(pluginName=env['packageName'])
            walkPluginDirs('src/plugins',pluginEnv)



        if(os.path.exists("src/binsrc")==True): 
            binEnv=env.Clone()
            binEnv.PrependUnique(CPPPATH     = [env['package_root'] + '/src/libsrc'])
            binEnv.PrependUnique(LIBPATH     = [env['package_root'] + '/src/libsrc/.' + env['arch']])
            if(len(libList)>0):
                   binEnv.PrependUnique(LIBS = [env['packageName']])
            walkBinDirs('src/binsrc',binEnv)


        
        if(os.path.exists("src/test")==True): 
            testEnv=env.Clone()
            testEnv.PrependUnique(CPPPATH     = [env['package_root'] + '/src/libsrc'])
            testEnv.PrependUnique(LIBPATH     = [env['package_root'] + '/src/libsrc/.' + env['arch']])
            if(len(libList)>0):
                   testEnv.PrependUnique(LIBS = [env['packageName']])
            walkTestDirs('src/test',testEnv)


    else:

        if(os.path.exists("java")==True):
            javaEnv=env.Clone()
            SCons.Script.SConscript('java/SConscript',exports=['javaEnv'], duplicate=0)
        else:
            print '?no java directory found'



#  recursively walks down doc dirs and executes all SConscript files it finds, ignores hidden dirs
def walkDocDirs(dir,docEnv):
    for file in glob(dir+'/*'):
        if(os.path.isfile(file)):
            head,basename=os.path.split(file)
            if(basename=='SConscript'):
                SCons.Script.SConscript(file,exports=['docEnv'], duplicate=0)
        elif (os.path.isdir(file)):
            walkDocDirs(file,docEnv.Clone())



#  recursively walks down script dirs and executes all SConscript files it finds, ignores hidden dirs
def walkScriptDirs(dir,scriptEnv):
    for file in glob(dir+'/*'):
        if(os.path.isfile(file)):
            head,basename=os.path.split(file)
            if(basename=='SConscript'):
                SCons.Script.SConscript(file,exports=['scriptEnv'], duplicate=0)
        elif (os.path.isdir(file)):
            walkScriptDirs(file,scriptEnv.Clone())



#  recursively walks down plugin dirs and executes all SConscript files it finds, ignores hidden dirs
def walkPluginDirs(dir,pluginEnv):
    for file in glob(dir+'/*'):
        head,basename=os.path.split(file)
        if(os.path.isfile(file)):
            if(basename=='SConscript'):
                SCons.Script.SConscript(file,exports=['pluginEnv'],variant_dir=dir+'/.'+pluginEnv['arch'],duplicate=0)
        elif (os.path.isdir(file)):
            newEnv=pluginEnv.Clone()
            newEnv.Replace(pluginName=file)
            walkPluginDirs(file,newEnv)



def walkBinDirs(dir,binEnv):
    for file in glob(dir+'/*'):
        if(os.path.isfile(file)):
            head,basename=os.path.split(file)
            if(basename=='SConscript'):
                SCons.Script.SConscript(file,exports=['binEnv'],variant_dir=dir+'/.'+binEnv['arch'],duplicate=0)
        elif (os.path.isdir(file)):
            walkBinDirs(file,binEnv.Clone())



#  recursively walks down test dirs and executes all SConscript files it finds, ignores hidden dirs
def walkTestDirs(dir,testEnv):
    for file in glob(dir+'/*'):
        if(os.path.isfile(file)):
            head,basename=os.path.split(file)
            if(basename=='SConscript'):
                SCons.Script.SConscript(file,exports=['testEnv'],variant_dir=dir+'/.'+testEnv['arch'],duplicate=0)
        elif (os.path.isdir(file)):
            walkTestDirs(file,testEnv.Clone())



#  calls loadxxx(env) in initxxx.py for each dependent package in reqlist
def define_dependencies(env,reqlist) :

    list = reqlist.split()
    for l in list:
        exec("from load" + l + " import load" + l)
        exec("load" + l + "(env)")



def buildBin(theEnv):

    #  search for c and cc files that have main routines in them
    filename_regex  = re.compile(r'[\w\.\- ]*\.(c|C)\w*$')
    find_main_regex = re.compile(r'^\s*int\s*main\s*\(.*$')
    
    mains = search_for_files(SCons.Script.Dir('.').srcnode().abspath, filename_regex, find_main_regex)


    #  must have at least one main routine
    if(len(mains)>0):
        others = filter(lambda x: path.splitext(x)[-1] in ['.c', '.cc'], glob(SCons.Script.Dir('.').srcnode().abspath+'/*'))
        others = [os.path.basename(x) for x in others]
        others = [x for x in others if x not in mains]

        # if this binary has ROOT dictionaries, add them to the list
        if ('ROOTCINT_SOURCES' in theEnv) and len(theEnv['ROOTCINT_SOURCES'])>0:
            theEnv.AppendUnique(CPPPATH = '#')
            others = theEnv['ROOTCINT_SOURCES'] if(len(others)==0) else others+theEnv['ROOTCINT_SOURCES']
        
        #  define build (default) and install targets
        if (len(mains[0])>0):
            for m in mains:
                prog = theEnv.Program(m.split('.')[0],[m] if (len(others)==0) else [m]+others)
                theEnv.Alias("install", theEnv.Install(theEnv['install_dir']+"/bin",prog))
                theEnv.Default(prog)
            


def buildTest(theEnv):

    #  search for c and cc files that have main routines in them
    filename_regex = re.compile(r'[\w\.\- ]*\.(c|C)\w*$')
    find_main_regex = re.compile(r'^\s*int\s*main\s*\(.*$')
    
    mains = search_for_files(SCons.Script.Dir('.').srcnode().abspath, filename_regex, find_main_regex)

    
    #  must have at least one main routine
    if(len(mains)>0):
    
        others = filter(lambda x: path.splitext(x)[-1] in ['.c', '.cc'], glob(SCons.Script.Dir('.').srcnode().abspath+'/*'))
        others = [os.path.basename(x) for x in others]
        others = [x for x in others if x not in mains]

        scripts = theEnv.Glob('*.py')+theEnv.Glob('*.pl')
        
        #  define build (default) and install targets
        if (len(mains[0])>0):
            for m in mains:
                prog = theEnv.Program(m.split('.')[0],[m] if (len(others)==0) else [m]+others)
                theEnv.Alias("install", theEnv.Install(theEnv['local_install_dir']+"/bin",prog))
                theEnv.Alias("install", theEnv.Install(theEnv['local_install_dir']+"/scripts",scripts))
                theEnv.Default(prog)
                        


def buildLib(theEnv):

    #  get lists of include and source files
    includes = theEnv.Glob('*.hxx') + theEnv.Glob('*.h')
    source   = theEnv.Glob('*.cc')  + theEnv.Glob('*.c')
    

    #  must have at least one include file to install
    if(len(includes)>0):
        theEnv.Alias("install", theEnv.Install(theEnv['install_dir']+"/include",includes))


    #  must have at least one source file to make library
    if(len(source)>0):
        shlib = theEnv.SharedLibrary(theEnv['packageName'],source)
        theEnv.Default(shlib)
        theEnv.Alias("install", theEnv.Install(theEnv['install_dir']+"/lib",shlib))
    
    


def buildPlugin(theEnv):

    #  get list of source files
    source   = theEnv.Glob('*.cc')  + theEnv.Glob('*.c')


    #  must have at least one source file to make plugin
    if(len(source)>0):
        plugin = theEnv.SharedLibrary(theEnv['pluginName'],source)
        theEnv.Default(plugin)
        theEnv.Alias("install", theEnv.Install(theEnv['install_dir']+"/plugins",plugin))
    
    


def buildDoc(theEnv):

    #  only if installing to different directory, don't copy SConscript file
    if(theEnv['install_root'] != theEnv['package_root']):
        docs = [f for f in theEnv.Glob('*') if not str(f)=='SConscript']
        if(len(docs)>0):
            theEnv.Alias("install", theEnv.Install(theEnv['install_root']+"/doc",docs))
    


def buildScripts(theEnv):

    #  only if installing to different directory, don't copy SConscript file
    if(theEnv['install_root'] != theEnv['package_root']):
        scripts = [f for f in theEnv.Glob('*') if not str(f)=='SConscript']
        if(len(scripts)>0):
            theEnv.Alias("install", theEnv.Install(theEnv['install_root']+"/scripts",scripts))
    


def buildJava(theEnv):

    javaFiles = theEnv.Java(target = theEnv['java_target'], source = theEnv['java_source'])
    jarFile   = theEnv.Jar(target  = theEnv['jar_target'],  source = theEnv['java_target'])
    theEnv.Default(jarFile)
    theEnv.Alias("install",theEnv.Install(theEnv['package_root']+'/jar',jarFile))



#  returns list of base filenames in directory whose name matches filename_regex and which have a line
#    that matches line_match_regex
def search_for_files(directory, filename_regex, line_match_regex):

    result = []
    for fullname in glob(directory+'/*'):
        head,basename=os.path.split(fullname)
        if filename_regex.match(basename):
            for line in open(fullname,'r'):
                if line_match_regex.match(line):
                    result.append(basename)
                    break
    return result
