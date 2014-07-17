--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.003,
		algorithm = "barnes-hut",
		bh_ratio = 0.50,
		bh_lifetime = 20,
		--Units are BYTES! Only calculated once every timer update!
		bh_heapsize_max = 536870912,
		--Normally, the thread assignment for BH will assign each thread in
		--the least occupied octree. Set this to true to change the behaviour
		--so that the cells closest to the root tree origin get assigned to threads.
		bh_thread_offset = false,
	},
	visual = {
		width = 1024,
		height = 600,
	},
	constants = {
		--elcharge = 1.602176565*10^-2,
		gconst = 6.67384*10^-12,
		--epsno = 8.854187817*10^-4,
		elcharge = 0,
		--gconst = 0,
		epsno = 0,
	},
	misc = {
		verbosity = 6,
	},
}

maxobjects = 3000;

--Add molecules or any additional objects here
objects = {
-- 	{
-- 		molfile = "./resources/molecules/1UAO.pdb",
-- 		posx = 0,
-- 		posy = 0,
-- 		posz = 0,
-- 		velz = 0,
-- 		vely = 0,
-- 		velz = 0,
-- 	}
}

--Spawn objects here. Arrays for position and velocity are not supported. Stick to normal variables.
function spawn_objects(varfromC)
	math.randomseed( os.time() )
	for i = 1, maxobjects, 1 do
		objects[i] = {
			posx = math.sin(i)*(i/maxobjects),
			posy = (math.random()-0.5)/10,
			posz = math.cos(i)*(i/maxobjects),
			velx = 0,
			vely = 0,
			velz = 0,
			charge = 100,
			mass = i*1000,
			radius = 0.2,
			atom = math.random(1,10),
			ignore = false,
		}
	end
	return objects, #objects
end
