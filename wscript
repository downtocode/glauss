#this script is horrible.
import sys, os, re
from waflib.Build import BuildContext

APPNAME='physengine'
VERSION=''

top = '.'
out = 'build'

def try_git_version():
	version = VERSION
	try:
		version = os.popen('git rev-parse --short=10 HEAD').read().strip()
	except Exception as e:
		print e
	return version

def try_pkg_path(name):
	path = ''
	cmd = 'pkg-config --cflags-only-I ' + name
	try:
		path = os.popen(cmd).read().strip()
		path = os.path.basename(path)
	except Exception as e:
		print e
	return path

#Custom transformation functions
def __file2string_cmd__(ctx):
	return "${{BIN_PYTHON}} {0}/TOOLS/file2string.py ${{SRC}} > ${{TGT}}" \
				.format(ctx.srcnode.abspath())
def __file2string__(ctx, **kwargs):
	ctx(
		rule   = __file2string_cmd__(ctx),
		before = ("c",),
		name   = os.path.basename(kwargs['target']),
		**kwargs
	)
def __png2c_cmd__(ctx):
	return "${{BIN_PYTHON}} {0}/TOOLS/png2c.py ${{SRC}} > ${{TGT}}" \
				.format(ctx.srcnode.abspath())
def __png2c__(ctx, **kwargs):
	ctx(
		rule   = __png2c_cmd__(ctx),
		before = ("c",),
		name   = os.path.basename(kwargs['target']),
		**kwargs
	)
BuildContext.png2c                = __png2c__
BuildContext.file2string          = __file2string__
	
def options(ctx):
	ctx.load('compiler_c')
	ctx.add_option('--enable-lto', dest='lto', default=False, action='store_true',
				help='Enable link-time optimizations.')
	ctx.add_option('--set-version', dest='ver', action='store', default='',
				help='Sets package version.')
	ctx.add_option('--no-debug', dest='no_deb', action='store_true', default=False,
				help='Unsets debug compilation flags.')
	ctx.add_option('--lua', action='store', default='luajit',
				help='Specify Lua version to link against(lua5.1, luajit, lua5.2)', dest='lua_ver')
	
def configure(ctx):
	ctx.load('compiler_c')
	
	if (ctx.options.no_deb == False):
		ctx.env.CFLAGS += ['-g']
	
	ctx.env.append_unique('CFLAGS', ['-O2', '-Wall', '-pedantic', '-std=gnu11', '-march=native'])
	
	#Add mandatory=False when we don't really need it.
	
	ctx.find_program('rst2man', mandatory=False, var='RST2MAN')
	ctx.check(features='c cprogram', lib=['m'], uselib_store='MATH')
	ctx.check(features='c cprogram', lib=['rt'], uselib_store='RT')
	ctx.check(features='c cprogram', lib=['pthread'], cflags='-pthread', uselib_store='PTHRD')
	ctx.check(features='c cprogram', lib=['readline'], uselib_store='READLN')
	ctx.check_cfg(package='sdl2', args='--cflags --libs', uselib_store='SDL')
	ctx.check_cfg(package='glesv2', args='--cflags --libs', uselib_store='GL')
	ctx.check_cfg(package='libpng12', args='--cflags --libs', mandatory=False, uselib_store='PNG')
	ctx.check_cfg(package='freetype2', args='--cflags --libs', uselib_store='FT')
	ctx.check_cfg(package='fontconfig', args='--cflags --libs', uselib_store='FC')
	ctx.check_cfg(package=ctx.options.lua_ver, args='--cflags --libs', mandatory=False, uselib_store='LUA')
	
	if (ctx.options.ver):
		package_ver = ctx.options.ver
	else:
		if (VERSION):
			package_ver = VERSION
		else:
			package_ver = try_git_version() + '-git'
	
	FULL_PACKAGE_NAME = APPNAME + ' ' + package_ver
	
	ctx.define('PACKAGE', APPNAME)
	ctx.define('PACKAGE_VERSION', package_ver)
	ctx.define('PACKAGE_STRING', FULL_PACKAGE_NAME)
	ctx.define('OPT_LTO', ctx.options.lto)
	if (ctx.env.LIB_PNG):
		ctx.define('HAS_PNG', 1)
	if (ctx.env.SDL):
		ctx.define('HAS_SDL', 1)
	
	lua_path = try_pkg_path(ctx.options.lua_ver)
	
	ctx.define('LINKED_LUA_VER', ctx.options.lua_ver)
	ctx.define('LUA_MAINHEAD', lua_path + '/lua.h')
	ctx.define('LUA_AUXLIB', lua_path + '/lauxlib.h')
	ctx.define('LUA_LIB', lua_path + '/lualib.h')
	ctx.define('LUA_HAS_NEW_LEN', ctx.options.lua_ver == 'lua5.2')
	
	ctx.write_config_header('config.h')
	
	if (ctx.options.lto):
		ctx.env.CFLAGS += ['-flto']
		ctx.env.LDFLAGS += ['-flto']
	
	print '\nCompiling: ', FULL_PACKAGE_NAME
	print '	Lua version:	', ctx.options.lua_ver
	print '	CFLAGS:  	', ctx.env.CFLAGS
	
	if (ctx.options.lua_ver != 'luajit'):
		print 'Keep in mind performance might be affected with anything else other than luajit.'
	
def build(ctx):
	#Generate manual page
	if (ctx.env.RST2MAN):
		ctx(name='manpage',
			source = 'DOCS/physengine.rst',
			target = 'physengine.1',
			rule = '${RST2MAN} ${SRC} ${TGT}',
		)
	#Generate included files.
	ctx.png2c(
		source = "resources/circle.png",
		target = "resources/sprite_img.h",
		features = 'c')
	ctx.file2string(
		source = "resources/shaders/object_vs.glsl",
		target = "graph/shaders/object_vs.h")
	ctx.file2string(
		source = "resources/shaders/object_fs.glsl",
		target = "graph/shaders/object_fs.h")
	ctx.file2string(
		source = "resources/shaders/text_vs.glsl",
		target = "graph/shaders/text_vs.h")
	ctx.file2string(
		source = "resources/shaders/text_fs.glsl",
		target = "graph/shaders/text_fs.h")
	ctx.file2string(
		source = "resources/elements.lua",
		target = "resources/elements.h")
	ctx.file2string(
		source = "resources/helpstring.txt",
		target = "resources/helpstring.h")
	ctx(name='msg_phys',
		path=ctx.path,
		target='msg_phys',
		source='main/msg_phys.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='parser',
		path=ctx.path,
		target='parser',
		uselib='LUA',
		source='input/parser.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='sighandle',
		path=ctx.path,
		target='sighandle',
		source='input/sighandle.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_input',
		path=ctx.path,
		uselib='SDL',
		target='graph_input',
		source='input/graph_input.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='input_thread',
		path=ctx.path,
		uselib='READLN',
		target='input_thread',
		source='input/input_thread.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics',
		path=ctx.path,
		uselib=[ 'MATH', 'PTHRD'],
		target='physics',
		source='physics/physics.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_aux',
		path=ctx.path,
		uselib='MATH',
		target='physics_aux',
		source='physics/physics_aux.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_ctrl',
		path=ctx.path,
		uselib=[ 'MATH', 'PTHRD'],
		target='physics_ctrl',
		source='physics/physics_ctrl.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_null',
		path=ctx.path,
		uselib=['PTHRD'],
		target='physics_null',
		source='physics/physics_null.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_n_body',
		path=ctx.path,
		uselib=[ 'MATH', 'PTHRD'],
		target='physics_n_body',
		source='physics/physics_n_body.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_barnes_hut',
		path=ctx.path,
		uselib=[ 'MATH', 'PTHRD'],
		target='physics_barnes_hut',
		source='physics/physics_barnes_hut.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='in_file',
		path=ctx.path,
		target='in_file',
		source='input/in_file.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='output',
		path=ctx.path,
		target='output',
		source='main/output.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph',
		path=ctx.path,
		uselib=[ 'GL', 'PNG' ],
		target='graph',
		source='graph/graph.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_thread',
		path=ctx.path,
		uselib=[ 'GL', 'PNG' ],
		target='graph_thread',
		source='graph/graph_thread.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_sdl',
		path=ctx.path,
		uselib=[ 'GL', 'PNG', 'SDL' ],
		target='graph_sdl',
		source='graph/graph_sdl.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_objects',
		path=ctx.path,
		uselib=[ 'GL', 'MATH', 'PNG' ],
		target='graph_objects',
		source='graph/graph_objects.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_fonts',
		path=ctx.path,
		uselib=[ 'GL', 'PNG', 'FC', 'FT' ],
		target='graph_fonts',
		source='graph/graph_fonts.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='main',
		path=ctx.path,
		uselib=['SDL', 'GL', 'MATH', 'PTHRD', 'PNG', 'LUA', 'FT', 'FC', 'READLN', 'RT'],
		use=['in_file', 'msg_phys', 'sighandle', 'graph', 'graph_sdl', 'graph_input', 'graph_objects', 'graph_fonts', 'graph_thread', 'input_thread', 'parser', 'output', 'physics', 'physics_aux', 'physics_ctrl', 'physics_null', 'physics_n_body', 'physics_barnes_hut'],
		target='physengine',
		source='main/main.c',
		features  = ['c', 'cprogram'],
		includes='. .. ../../',
	)
