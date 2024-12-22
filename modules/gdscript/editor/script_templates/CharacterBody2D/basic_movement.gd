# meta-description: Classic movement for gravity games (platformer, ...)

extends _BASE_


const SPEED = 300.0
const ACCEL = 300.0
const JUMP_VELOCITY = -400.0


func _physics_process(delta: float) -> void:
	# Handle gravity.
	if not is_on_floor():
		velocity += get_gravity() * delta

	# Get the vertical velocity.
	var vertical_velocity: Vector2 = velocity.project(up_direction)

	# Get the horizontal velocity.
	var horizontal_velocity: Vector2 = velocity - vertical_velocity

	# Handle jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		vertical_velocity = up_direction * JUMP_VELOCITY

	# As good practice, you should replace UI actions with custom gameplay actions.
	var input_axis: float = Input.get_axis("ui_left", "ui_right")

	# Calculate the intended direction in 2D plane.
	var input_direction: Vector2 = Vector2(input_axis, 0).rotated(rotation)

	# Calculate the target horizontal velocity.
	var target_horizontal_velocity: Vector2 = input_direction * SPEED

	# Move the current horizontal velocity towards the target horizontal velocity.
	horizontal_velocity = horizontal_velocity.move_toward(target_horizontal_velocity, ACCEL * delta)

	# Compose the final velocity.
	velocity = horizontal_velocity + vertical_velocity

	move_and_slide()
