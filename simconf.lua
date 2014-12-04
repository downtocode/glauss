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
		exec_funct_freq = 1, --Auto timestep_funct run frequency
		lua_expose_obj_array = true,
		--Expose object array to the timestep_funct, slight performance decrease
	},
	input = {
		input_thread_enable = true,
		--Disable command line interface
	},
	visual = {
		bgcolor = {32, 32, 32, 255},
		default_draw_mode = "MODE_SPHERE",
		width = 1280,
		height = 720,
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
	
	local cube_size = 5
	local z = 1
	local velocity = 10
	
	for i = 0, cube_size, 1 do
		for j = 0, cube_size, 1 do
			for k = 0, cube_size, 1 do
				objects[z] = {
					pos = {
						i-(cube_size/2),
						j-(cube_size/2),
						k-(cube_size/2),
					},
					vel = {
						0,
						0,
						0,
					},
					acc = {
						0,
						0,
						0,
					},
					charge = 0,
					state = 0,
					mass = 1,
					radius = 0.1,
					atomnumber = math.random(1,2),
					ignore = false,
				}
				z = z + 1
			end
		end
	end
	
	objects[z] = {
		pos = {
			cube_size-(cube_size/2),
			cube_size-(cube_size/2),
			cube_size-(cube_size/2),
		},
		vel = {
			0,
			0,
			0,
		},
		acc = {
			0,
			0,
			0,
		},
		charge = 0,
		state = 0,
		mass = 1,
		radius = 0.1,
		atomnumber = math.random(1,2),
		ignore = false,
	}
	
	return objects
end

function vector_sub(lhs, rhs) return { lhs[1]-rhs[1], lhs[2]-rhs[2], lhs[3]-rhs[3] } end
function vector_add(lhs, rhs) return { lhs[1]+rhs[1], lhs[2]+rhs[2], lhs[3]+rhs[3] } end
function vector_mul(lhs, rhs) return { lhs[1]+rhs[1], lhs[2]+rhs[2], lhs[3]+rhs[3] } end
function vector_det(lhs) return lhs[1] + lhs[2] + lhs[3] end

function measure_energy(obj)
	energy = 0.0
	
	local dist_vec = {}
	local dist = 0.0
	
	for i = 1, #obj, 1 do
		for j = 1, #obj, 1 do
			dist_vec = vector_sub(obj[j].pos, obj[i].pos)
			dist = math.sqrt(vector_det(vector_mul(dist_vec, dist_vec)))
			if dist > 2.0 then goto continue end
			energy = energy + 1
			::continue::
		end
	end
	
	return energy
end

--Consult physics/physics.h for the format of struct thread_statistics and typedef phys_obj
function run_on_timestep(t_stats, obj)
	--print("Current progress:", t_stats.progress)
	--print("Steps = ", t_stats.total_steps, "Time per step = ", t_stats.time_per_step)
	
	--if t_stats.progress > 0.0 then phys_pause() end
	if obj == nil then return nil end
	
	local old_energy = measure_energy(obj)
	
	local src_index = 0
	local dest_index = 0
	
	while src_index == dest_index do
		src_index = math.random(1, #obj)
		dest_index = math.random(1, #obj)
	end
	
	--Remember, Lua arrays are indexed from 1(by default, but it doesn't care)
	--however our internal objects are indexed from 0. We send lua stuff indexed from 1
	--but we read the IDs of the objects, not the order in which the objects arrive.
	--Since ids are indexed from 0 and if no objects are imported follow the index
	--of the array offset the obj count such that we match the internal db.
	
	local cp_obj = obj[src_index].pos
	obj[src_index].pos = obj[dest_index].pos
	obj[dest_index].pos = cp_obj
	
	if measure_energy(obj) > 0 then
		print("Move accepted, src =", src_index, "dest =", dest_index)
		return {obj[src_index], obj[dest_index]}
	else
		print("Move rejected")
		return nil
	end
end
