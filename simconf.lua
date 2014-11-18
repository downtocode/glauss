--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.012322312,
		--algorithm = "barnes-hut",
		rng_seed = os.time(),
		bh_ratio = 0.4,
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
	lua = {
		spawn_funct = "spawn_objects",
		--Name of function to read objects from
		timestep_funct = "run_on_timestep",
		--Function to execute upon timestep completion
		exec_funct_freq = 50, --Auto timestep_funct run frequency
		lua_expose_obj_array = true,
		--Expose object array to the timestep_funct, slight performance decrease
	},
	input = {
		input_thread_enable = true,
		--Disable command line interface
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
		skip_model_vec = 40,
	},
}

objects = {
-- 		{
-- 			import = "../SpaceBattleShipYAMATO-MG.obj",
-- 			scale = 999999999,
-- 			pos = {0,-200,0},
-- 			vel = {0,0,0},
-- 			rot = {0,math.pi/2,math.pi/2},
-- 			ignore = false,
-- 		}
}

function spawn_objects(string_from_arg)
	print("Sent value", string_from_arg)
	math.randomseed( settings.physics.rng_seed )
	
	local cube_size = 1
	local velocity = 10
	
	for i = #objects+1, #objects+1+1562, 1 do
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
			mass = 10000000000,
			radius = 0.2,
			atomnumber = math.random(1,10),
			ignore = false,
		}
	end
	
	return objects
end

--Consult physics/physics.h for the format of struct thread_statistics and typedef data
function run_on_timestep(t_stats, obj)
	print("Current progress:", t_stats.progress)
	print("Steps = ", t_stats.total_steps, "Time per step = ", t_stats.time_per_step)
	
	local copied_objs = {}
	local velocity = 10
	
	--if t_stats.progress > 0.2 then phys_pause() end
	
	local cube_size = 2
	
	for i = 4, 16, 1 do
		copied_objs[i] = obj[i]
		
		copied_objs[i].pos = {
			(math.random()-0.5)*cube_size,
			(math.random()-0.5)*cube_size,
			(math.random()-0.5)*cube_size,
		}
		
		copied_objs[i].vel = {
			(math.random()-0.5)*velocity,
			(math.random()-0.5)*velocity,
			(math.random()-0.5)*velocity,
		}
		
	end
	
	return copied_objs
end
