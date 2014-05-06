--Physengine example config file. Syntax is standard Lua. Math libraries are included.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 2,
		dt = 0.000004,
	},
	visual = {
		width = 1280,
		height = 720,
		fontname = "./resources/fonts/DejaVuSansMono.ttf",
	},
	constants = {
		--elcharge = 1.602176565*10^-7,
		--gconst = 6.67384*10^-11,
		--epsno = 8.854187817*10^-12,
	},
	misc = {
		verbosity = 10,
	},
}

--Add molecules or any additional objects here
objects = {}
objects[1] = {
	molfile = "./resources/molecules/benzene.xyz",
	posx = 10,
	posy = 10,
	posz = 10,
	velx = 0,
	vely = 0,
	velz = 0,
}

--Spawn objects here. Arrays for position and velocity are not supported yet. Stick to normal variables.
--If atom is set to a non-zero value, mass, charge and radius values will be ignored.
function spawn_objects(varfromC)
	--Get number of non-loop objects and add 1 to avoid overwriting.
	for i = 1+(#objects), 100, 1 do
		objects[i] = {
			posx = i,
			posy = 10*math.cos(i),
			posz = math.cos(25*i),
			velx = 0,
			vely = 0,
			velz = 0,
			charge = 1,
			mass = 1,
			radius = 1,
			atom = 0,
			ignore = false,
		}
	end
	return objects, #objects
end
