# Typed conversion constructors are currently only implemented for Array
# Update tests if other built-in types gain this functionality
# Ex: Typed dictionarys are implemented

func test():
	# Error
	# Typed conversion constructor checks if base and the provided type are compatible.
	var convert = float[int]([2.0, 40.0]);
