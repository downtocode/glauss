--Physengine example config file. Syntax is standard Lua. Math libraries are included and initialized.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.001,
		algorithm = "barnes-hut",
		bh_ratio = 0.70,
		--Lifetime of a cell before it's freed.
		bh_lifetime = 16,
		--Units are size_t(bytes)! PER THREAD!
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
-- 		import = "./resources/molecules/1UAO.pdb",
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
			posx = math.sin(i)*(i/maxobjects)*2,
			posy = (math.random()-0.5)/10,
			posz = math.cos(i)*(i/maxobjects)*2,
			velx = 0,
			vely = 0,
			velz = 0,
			charge = 100,
			mass = 10000*i,
			radius = 0.2,
			atom = math.random(1,10),
			ignore = false,
		}
	end
	return objects, #objects
end
