import sys, os, re
from waflib.Build import BuildContext

APPNAME='physengine'
VERSION=''

top = '.'
out = 'build'
manual_true = 0

def try_git_version():
	version = VERSION
	try:
		version = os.popen('git rev-parse --short=10 HEAD').read().strip()
	except Exception as e:
		print e
	return version

def __file2string_cmd__(ctx):
	return "${{BIN_PERL}} {0}/TOOLS/file2string.pl ${{SRC}} > ${{TGT}}" \
				.format(ctx.srcnode.abspath())
def __file2string__(ctx, **kwargs):
	ctx(
		rule   = __file2string_cmd__(ctx),
		before = ("c",),
		name   = os.path.basename(kwargs['target']),
		**kwargs
	)
BuildContext.file2string          = __file2string__
	
def options(ctx):
	ctx.load('compiler_c')
	ctx.add_option('--enable-lto', dest='lto', default=False, action='store_true',
				help='Enable link-time optimizations.')
	ctx.add_option('--set-version', dest='ver', action='store', default='',
				help='Sets package version.')
	ctx.add_option('--no-debug', dest='no_deb', action='store_true', default=False,
				help='Unsets debug compilation flags.')
	ctx.add_option('--enable-manual', dest='enable_manual', action='store_true', default=False,
				help='Compile and install the manual(requires rst2man)')
	
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
	ctx.check_cfg(package='lua5.2', args='--cflags --libs', uselib_store='LUA')
	
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
	ctx.write_config_header('config.h')
	
	if (ctx.options.enable_manual):
		manual_true = 1
	
	if (ctx.options.lto):
		ctx.env.CFLAGS += ['-flto']
		ctx.env.LDFLAGS += ['-flto']
	
	print '\nCompiling: ', FULL_PACKAGE_NAME
	print '	CFLAGS:  ', ctx.env.CFLAGS
	
def build(ctx):
	#Generate manual page
	if (ctx.env.RST2MAN):
		ctx(name='manpage',
			source = 'DOCS/physengine.rst',
			target = 'physengine.1',
			rule = '${RST2MAN} ${SRC} ${TGT}',
		)
	#Generate included files.
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
		target = "physics/resources/elements.h")
	ctx.file2string(
		source = "resources/helpstring.txt",
		target = "main/resources/helpstring.h")
	ctx(name='msg_phys',
		path=ctx.path,
		target='msg_phys',
		source='main/msg_phys.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='parser',
		path=ctx.path,
		uselib='LUA',
		target='parser',
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
		target='graph_input',
		source='input/graph_input.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='input_thread',
		path=ctx.path,
		target='input_thread',
		source='input/input_thread.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics',
		path=ctx.path,
		uselib='MATH',
		target='physics',
		source='physics/physics.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_aux',
		path=ctx.path,
		uselib='LUA',
		target='physics_aux',
		source='physics/physics_aux.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_ctrl',
		path=ctx.path,
		target='physics_ctrl',
		source='physics/physics_ctrl.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_null',
		path=ctx.path,
		target='physics_null',
		source='physics/physics_null.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_n_body',
		path=ctx.path,
		uselib='PTHRD',
		target='physics_n_body',
		source='physics/physics_n_body.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_barnes_hut',
		path=ctx.path,
		uselib='PTHRD',
		target='physics_barnes_hut',
		source='physics/physics_barnes_hut.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='in_file',
		path=ctx.path,
		uselib='MATH',
		target='in_file',
		source='input/in_file.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='output',
		path=ctx.path,
		uselib='MATH',
		target='output',
		source='main/output.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph',
		path=ctx.path,
		uselib='GL, PNG',
		target='graph',
		source='graph/graph.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_thread',
		path=ctx.path,
		uselib='GL, PNG',
		target='graph_thread',
		source='graph/graph_thread.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_sdl',
		path=ctx.path,
		uselib='GL, PNG, SDL',
		target='graph_sdl',
		source='graph/graph_sdl.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_objects',
		path=ctx.path,
		uselib='GL',
		target='graph_objects',
		source='graph/graph_objects.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_fonts',
		path=ctx.path,
		uselib='GL, FT, FC',
		target='graph_fonts',
		source='graph/graph_fonts.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='main',
		path=ctx.path,
		use=['SDL', 'GL', 'MATH', 'PTHRD', 'PNG', 'LUA', 'FT', 'FC', 'READLN', 'RT', 'in_file', 'msg_phys', 'sighandle', 'graph', 'graph_sdl', 'graph_input', 'graph_objects', 'graph_fonts', 'graph_thread', 'input_thread', 'parser', 'output', 'physics', 'physics_aux', 'physics_ctrl', 'physics_null', 'physics_n_body', 'physics_barnes_hut'],
		target='physengine',
		source='main/main.c',
		features  = ['c', 'cprogram'],
		includes='. .. ../../',
	)
