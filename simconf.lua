--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.001,
		algorithm = "null",
		rng_seed = os.time(),
	},
	lua_settings = {
		spawn_funct = "spawn_objects",
		--Name of function to read objects from
		timestep_funct = "run_on_timestep",
		--Function to execute upon timestep completion
		exec_funct_freq = 1, --Auto timestep_funct run frequency
		lua_expose_obj_array = false;
		--Expose object array to the timestep_funct, slight performance decrease
		reset_stats_freq = 1;
		--Disable any averaging and reset global stats every cycle
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
		skip_model_vec = 200,
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

objects = {}

function spawn_objects(string_from_arg)
	print("Sent value", string_from_arg)
	math.randomseed( settings.physics.rng_seed )
	
	cube_size = 10
	velocity = 10
	
	for i = 1, 2000, 1 do
			objects[i] = {
			pos = {
				(math.random()-0.5)*cube_size,
				(math.random()-0.5)*cube_size,
				(math.random()-0.5)*cube_size,
			},
			vel = {
				(math.random()-0.5)*velocity,
				(math.random()-0.5)*velocity,
				(math.random()-0.5)*velocity,
			},
			charge = 0,
			mass = 100000,
			radius = 0.2,
			atomnumber = math.random(1,10),
			ignore = false,
		}
	end
	
	return objects, #objects
end

--Consult physics/physics.h for the format of struct thread_statistics and typedef data
function run_on_timestep(t_stats, obj)
	print("Current progress:", t_stats.progress)
	
	for i = 1, #t_stats, 1 do
		--print("Thread", i, "Usage: ", t_stats[i].bh_heapsize/1048576, "MiB")
	end
	print("Total octree memory usage:", t_stats.bh_heapsize/1048576, "MiB")
	
	if obj == nil then
		return 0
	else
		print("Velocity of object 1:", math.sqrt(obj[1].vel[0]^2 + obj[1].vel[1]^2 + obj[1].vel[2]^2) )
	end
	return 1
end
