--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.003,
		algorithm = "barnes-hut",
		bh_ratio = 0.50,
		--Lifetime of a cell before it's freed.
		bh_lifetime = 20,
		--Units are size_t! PER THREAD!
		bh_heapsize_max = 536870912,
		--Maximum threads per octree. Reduce this to spread threads more.
		bh_tree_limit = 8, --Range is [1,8(default)]
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

function spawn_objects(varfromC)
	math.randomseed( os.time() )
	for i = #objects, maxobjects, 1 do
		objects[i] = {
			posx = math.sin(i)*(i/maxobjects),
			posy = (math.random()-0.5)/10,
			posz = math.cos(i)*(i/maxobjects),
			velx = -0.1,
			vely = 0.1,
			velz = 0.04,
			charge = 100,
			mass = i*1000,
			radius = 0.2,
			atom = math.random(1,10),
			ignore = false,
		}
	end
	return objects, #objects
end
