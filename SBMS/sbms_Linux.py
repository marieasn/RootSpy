#
# Linux specific settings
#

def InitENV(env):
	
	# cMsg and ET use things from the librt library
	# this is only available on Linux
	env.AppendUnique(LIBS='rt')
