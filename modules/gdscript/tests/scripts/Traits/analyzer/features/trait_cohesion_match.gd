uses SomeTrait, OtherTrait

trait SomeTrait:
		func func_one():
				print("function one")

trait OtherTrait:
		func func_two():
				print("function two")

func test():
		func_one()
		func_two()
		print("ok")
