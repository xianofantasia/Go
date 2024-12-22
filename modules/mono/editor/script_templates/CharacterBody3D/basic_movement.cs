// meta-description: Classic movement for gravity games (FPS, TPS, ...)

using _BINDINGS_NAMESPACE_;
using System;

public partial class CharacterBody3d : CharacterBody3D
{
	public const float Speed = 5.0f;
	public const float Accel = 5.0f;
	public const float JumpVelocity = 4.5f;

	public override void _PhysicsProcess(double delta)
	{
		Vector3 velocity = Velocity;

		// Add the gravity.
		if (!IsOnFloor())
		{
			velocity += GetGravity() * (float)delta;
		}

		// Get the vertical velocity.
		Vector3 verticalVelocity = velocity.Project(UpDirection);

		// Get the horizontal velocity.
		Vector3 horizontalVelocity = velocity - verticalVelocity;

		// Handle Jump.
		if (Input.IsActionJustPressed("ui_accept") && IsOnFloor())
		{
			verticalVelocity = UpDirection * JumpVelocity;
		}

		// As good practice, you should replace UI actions with custom gameplay actions.
		Vector2 inputVector = Input.GetVector("ui_left", "ui_right", "ui_up", "ui_down", 0.15f);

		// Calculate the intended direction in 3D space.
		Vector3 inputDirection = Transform.Basis.Orthonormalized() * new Vector3(inputVector.X, 0, inputVector.Y);

		// Calculate the target horizontal velocity.
		Vector3 targetHorizontalVelocity = inputDirection * Speed;

		// Move the current horizontal velocity towards the target horizontal velocity.
		horizontalVelocity = horizontalVelocity.MoveToward(targetHorizontalVelocity, Accel * (float)delta);

		// Compose the final velocity.
		velocity = horizontalVelocity + verticalVelocity;

		Velocity = velocity;
		MoveAndSlide();
	}
}
