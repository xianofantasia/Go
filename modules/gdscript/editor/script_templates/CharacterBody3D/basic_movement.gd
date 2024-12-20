# meta-description: Classic movement for gravity games (FPS, TPS, ...)

extends _BASE_


const SPEED = 5.0
const ACCEL = 5.0
const JUMP_VELOCITY = 4.5


func _physics_process(delta: float) -> void:
	# Handle gravity.
	if not is_on_floor():
		velocity += get_gravity() * delta

	# Get the vertical velocity.
	var vertical_velocity: Vector3 = velocity.project(up_direction)

	# Get the horizontal velocity.
	var horizontal_velocity: Vector3 = velocity - vertical_velocity

	# Handle jump.
	if Input.is_action_just_pressed("ui_accept") and is_on_floor():
		vertical_velocity = up_direction * JUMP_VELOCITY

	# As good practice, you should replace UI actions with custom gameplay actions.
	var input_vector := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down", 0.15)

	# Calculate the intended direction in 3D space.
	var input_direction: Vector3 = transform.basis.orthonormalized() * Vector3(input_vector.x, 0, input_vector.y)

	# Calculate the target horizontal velocity.
	var target_horizontal_velocity: Vector3 = input_direction * SPEED

	# Move the current horizontal velocity towards the target horizontal velocity.
	horizontal_velocity = horizontal_velocity.move_toward(target_horizontal_velocity, ACCEL * delta)

	# Compose the final velocity.
	velocity = horizontal_velocity + vertical_velocity

	move_and_slide()
