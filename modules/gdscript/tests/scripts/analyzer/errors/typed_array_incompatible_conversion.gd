# Typed conversion constructors are currently only implemented for Array
# Update tests if other built-in types gain this functionality
# Ex: Typed dictionarys are implemented

var base: Array[float] = [0.1, 200.2, 30.5];

func test():
	# Error
	# Typed conversion constructor checks if base and the provided type are compatible.
	var convert = Array[Node](base);
