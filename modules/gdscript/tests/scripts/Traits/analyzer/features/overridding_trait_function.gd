trait SomeTrait:
	func some_func(param: Node)

class SomeClass:
	uses SomeTrait

	func some_func(param: Object):
		print("overridden some func")

func test():
	SomeClass.new().some_func(Node.new())
	print("ok")
