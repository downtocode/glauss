--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.001,
		algorithm = "barnes-hut",
		bh_ratio = 0.49,
		--Lifetime of a cell before it's freed.
		bh_lifetime = 20,
		--Units are size_t(bytes)! PER THREAD!
		bh_heapsize_max = 336870912,
		--Maximum threads per octree. Reduce this to spread threads more.
		bh_tree_limit = 8, --Range is [2,8(default)]
		--If only a single thread is available, assign the entire root to it.
		bh_single_assign = true,
		--Assign the octrees of the threads randomly.
		bh_random_assign = true,
	},
	lua_settings = {
		spawn_funct = "spawn_objects",
		--Name of function to read objects from
		timestep_funct = "run_on_timestep",
		--Function to execute upon timestep completion
		exec_funct_freq = 10, --Auto timestep_funct run frequency
		lua_expose_obj_array = false;
		--Expose object array to the timestep_funct, slight performance decrease
	},
	visual = {
		width = 1024,
		height = 600,
		screenshot_template = "sshot_%3.3Lf.png",
		file_template = "system_%0.2Lf.xyz",
		fontname = "Liberation Sans",
		fontsize = 38,
		verbosity = 8,
		dump_sshot = 0, --Auto screenshot dump frequency
		dump_xyz = 0, --Auto xyz dump frequency
	},
	constants = {
		--elcharge = 1.602176565*10^-2,
		gconst = 6.67384*10^-8,
		--epsno = 8.854187817*10^-4,
		elcharge = 0,
		--gconst = 0,
		epsno = 0,
	},
}

maxobjects = 24000

--Add molecules or any additional objects here
objects = {
-- 	{
-- 		import = "../soccer ball.obj",
-- 		scale = 1,
-- 		posx = 0, rotx = 0, velx = 0,
-- 		posy = 0, roty = 0, vely = 0,
-- 		posz = 0, rotz = 0, velz = 0,
-- 		ignore = false,
-- 	}
}

-- Any objects returned to or from the main program have the following format:
-- table/array_of_objects = {
-- 	{ --object
-- 		pos = {table/vector of 3 values},
-- 		vel = {table/vector of 3 values},
-- 		chage = value,
-- 		mass = value,
-- 		radius = value,
-- 		atomnumber = value,
-- 		ignore = value,
-- 	},
-- 	{ --object
-- 		...
-- 	},
-- 	...
-- }

function spawn_objects(var_C)
	math.randomseed( os.time() )
	scale_obj = 45
	for i = #objects+1, maxobjects+1, 1 do
		objects[i] = {
			pos = {
				scale_obj*math.sin(i)*(i/maxobjects),
				scale_obj*(math.random()-0.5)/10,
				scale_obj*math.cos(i)*(i/maxobjects),
			},
			vel = {0,0,0},
			charge = 100,
			mass = 1000000*i,
			radius = 0.2,
			atomnumber = math.random(1,10),
			ignore = false,
		}
	end
	return objects, #objects
end

--Consult physics/physics.h for the format of struct thread_statistics and typedef data
function run_on_timestep(t_stats, obj)
	print("Current progress:", t_stats[1].progress)
	
	memory_usage = 0
	for i = 1, #t_stats, 1 do
		memory_usage = memory_usage + t_stats[i].bh_heapsize
	end
	print("Total octree memory usage:", memory_usage/1048576.0)
	
	if obj == nil then
		return 0
	else
		print("Velocity of object 1:", math.sqrt(obj[1].vel[0]^2 + obj[1].vel[1]^2 + obj[1].vel[2]^2) )
	end
	return 1
end
