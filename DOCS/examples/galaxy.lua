settings = {
	algorithm = "none",
}

function spawn_objects(var_C)
	scale_obj = 45
	maxobjects = 2400
	objects = {}
	for i = 1, maxobjects, 1 do
		objects[i] = {
			posx = scale_obj*math.sin(i)*(i/maxobjects),
			posy = scale_obj*(math.random()-0.5)/10,
			posz = scale_obj*math.cos(i)*(i/maxobjects),
		}
	end
	return objects, #objects
end
