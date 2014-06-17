--Physengine example config file. Syntax is standard Lua. Math libraries are included.
settings = {
	--Only the names of the variables are used. Tables are just for organization(except settings), feel free to drop them.
	physics = {
		threads = 2,
		dt = 0.01,
		algorithm = "n-body",
	},
	visual = {
		width = 1280,
		height = 720,
	},
	constants = {
		elcharge = 1.602176565*10^-12,
		--gconst = 6.67384*10^-11,
		epsno = 8.854187817*10^-18,
		--elcharge = 0,
		gconst = 0,
		--epsno = 0,
	},
	misc = {
		verbosity = 6,
	},
}

maxobjects = 1000;

--Add molecules or any additional objects here
objects = {}

--Spawn objects here. Arrays for position and velocity are not supported yet. Stick to normal variables.
--If atom is set to a non-zero value, mass, charge and radius values will be ignored.
function spawn_objects(varfromC)
	--Get number of non-loop objects and add 1 to avoid overwriting.
	for i = 1, maxobjects, 1 do
		objects[i] = {
			posx = math.sin(i),
			posy = math.cos(i),
			posz = math.cos(i)*(math.random(0,10)-5),
			velx = -math.sin(i),
			vely = -math.cos(i),
			velz = -math.cos(i)*(math.random(0,10)-5),
			charge = 100*math.random(-1,1),
			mass = 1,
			radius = 0.2,
			atom = 0,
			ignore = false,
		}
	end
	return objects, #objects
end
