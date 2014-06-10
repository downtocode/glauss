--Physengine example config file. Syntax is standard Lua. Math libraries are included.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 1,
		dt = 0.000001,
		algorithm = "n-body",
	},
	visual = {
		width = 1280,
		height = 720,
	},
	constants = {
		--elcharge = 1.602176565*10^-7,
		--gconst = 6.67384*10^-11,
		--epsno = 8.854187817*10^-12,
		elcharge = 0,
		gconst = 0,
		epsno = 0,
	},
	misc = {
		verbosity = 6,
	},
}

--Add molecules or any additional objects here
objects = {}

--Spawn objects here. Arrays for position and velocity are not supported yet. Stick to normal variables.
--If atom is set to a non-zero value, mass, charge and radius values will be ignored.
function spawn_objects(varfromC)
	--Get number of non-loop objects and add 1 to avoid overwriting.
	for i = 1, 1000, 1 do
		objects[i] = {
			posx = i*math.cos(math.pi*i/221)*math.cos(math.pi*i/159),
			posy = i*i*math.sin(math.pi*i/124)*math.sin(math.pi*i/211),
			posz = i/2000,
			velx = 0,
			vely = 0,
			velz = i,
			charge = 10,
			mass = 1,
			radius = 0.2,
			atom = 0,
			ignore = false,
		}
	end
	return objects, #objects
end
