import sys, os, re
from waflib.Build import BuildContext

APPNAME='physengine'
VERSION='0.2'

top = '.'
out = 'build'

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
	
def configure(ctx):
	ctx.load('compiler_c')
	ctx.env.append_unique('CFLAGS', ['-g', '-O2', '-Wall', '-pedantic', '-std=gnu11', '-march=native'])
	
	
	ctx.check(features='c cprogram', lib=['m'], uselib_store='MATH')
	ctx.check_cfg(package='sdl2', args='--cflags --libs', uselib_store='SDL')
	ctx.check_cfg(package='glesv2', args='--cflags --libs', uselib_store='GL')
	ctx.check_cfg(package='freetype2', args='--cflags --libs', uselib_store='FT')
	ctx.check_cfg(package='fontconfig', args='--cflags --libs', uselib_store='FC')
	ctx.check_cfg(package='lua5.2', args='--cflags --libs', uselib_store='LUA')
	ctx.check(features='c cprogram', lib=['pthread'], cflags='-pthread', uselib_store='PTHRD')
	
	git_version = try_git_version()
	ctx.define('PACKAGE', APPNAME)
	ctx.define('PACKAGE_VERSION', git_version)
	ctx.define('PACKAGE_STRING', APPNAME + ' ' + git_version)
	ctx.define('OPT_LTO', ctx.options.lto)
	ctx.write_config_header('config.h')
	
	if (ctx.options.lto):
		ctx.env.CFLAGS += ['-flto']
		ctx.env.LDFLAGS += ['-flto']
	
	print '\nCompiling: ', APPNAME, git_version
	print '	CFLAGS:  ', ctx.env.CFLAGS
	
def build(ctx):
	#Generate included files.
	ctx.file2string(
		source = "resources/shaders/object_vs.glsl",
		target = "src/shaders/object_vs.h")
	ctx.file2string(
		source = "resources/shaders/object_fs.glsl",
		target = "src/shaders/object_fs.h")
	ctx.file2string(
		source = "resources/shaders/text_vs.glsl",
		target = "src/shaders/text_vs.h")
	ctx.file2string(
		source = "resources/shaders/text_fs.glsl",
		target = "src/shaders/text_fs.h")
	ctx.file2string(
		source = "resources/elements.lua",
		target = "src/resources/elements.h")

	ctx(name='msg_phys',
		path=ctx.path,
		target='msg_phys',
		source='src/msg_phys.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='parser',
		path=ctx.path,
		uselib='LUA',
		target='parser',
		source='src/parser.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics',
		path=ctx.path,
		uselib='MATH',
		target='physics',
		source='src/physics.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_aux',
		path=ctx.path,
		uselib='LUA',
		target='physics_aux',
		source='src/physics_aux.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_null',
		path=ctx.path,
		target='physics_null',
		source='src/physics_null.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_n_body',
		path=ctx.path,
		uselib='PTHRD',
		target='physics_n_body',
		source='src/physics_n_body.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='physics_barnes_hut',
		path=ctx.path,
		uselib='PTHRD',
		target='physics_barnes_hut',
		source='src/physics_barnes_hut.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='in_molecule',
		path=ctx.path,
		uselib='MATH',
		target='in_molecule',
		source='src/in_molecule.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='out_xyz',
		path=ctx.path,
		uselib='MATH',
		target='out_xyz',
		source='src/out_xyz.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph',
		path=ctx.path,
		uselib='GL',
		target='graph',
		source='src/graph.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_objects',
		path=ctx.path,
		uselib='GL',
		target='graph_objects',
		source='src/graph_objects.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='graph_fonts',
		path=ctx.path,
		uselib='GL, FT, FC',
		target='graph_fonts',
		source='src/graph_fonts.c',
		features  = ['c'],
		includes='. .. ../../',
	)
	ctx(name='main',
		path=ctx.path,
		use=['SDL', 'GL', 'MATH', 'PTHRD', 'LUA', 'FT', 'FC', 'in_molecule', 'msg_phys', 'graph', 'graph_objects', 'graph_fonts', 'parser', 'out_xyz', 'physics', 'physics_aux', 'physics_null', 'physics_n_body', 'physics_barnes_hut'],
		target='physengine',
		source='src/main.c',
		features  = ['c', 'cprogram'],
		includes='. .. ../../',
	)
