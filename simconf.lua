--Physengine example config file. Syntax is standard Lua. Math libraries are included.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 2,
		dt = 0.000001,
		algorithm = "n-body",
	},
	visual = {
		width = 1280,
		height = 720,
		fontname = "./resources/fonts/DejaVuSansMono.ttf",
	},
	constants = {
		elcharge = 1.602176565*10^-7,
		gconst = 6.67384*10^-11,
		epsno = 8.854187817*10^-12,
	},
	misc = {
		verbosity = 8,
	},
}

--Add molecules or any additional objects here
objects = {}

--Spawn objects here. Arrays for position and velocity are not supported yet. Stick to normal variables.
--If atom is set to a non-zero value, mass, charge and radius values will be ignored.
function spawn_objects(varfromC)
	--Get number of non-loop objects and add 1 to avoid overwriting.
	for i = 1, 10, 1 do
		objects[i] = {
			posx = math.sin(i),
			posy = i*math.cos(i),
			posz = i,
			velx = 0,
			vely = 0,
			velz = 100/i,
			charge = 10,
			mass = 10000000000/i,
			radius = 0.2,
			atom = 0,
			ignore = false,
		}
	end
	return objects, #objects
end
