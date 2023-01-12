# Typed conversion constructors are currently only implemented for Array
# Update tests if other built-in types gain this functionality eg: Typed dictionarys are implemented

func test():
	var floats = Array[float]([2.4, 40.8, 200.9]);
	var _ints = Array[int](floats);
