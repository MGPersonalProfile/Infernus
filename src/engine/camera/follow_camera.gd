class_name FollowCamera
extends Camera2D

## Cámara que sigue a un Node2D con offset según la dirección de
## movimiento (look-ahead lateral). Snap final a píxel entero para
## evitar que las texturas vibren en pixel art.

@export var target: Node2D
@export_range(0.0, 200.0, 5.0) var look_ahead_distance: float = 60.0
@export_range(0.0, 1.0, 0.01) var smoothing: float = 0.12

var _desired_offset_x: float = 0.0
var _current_offset_x: float = 0.0


func _process(delta: float) -> void:
	if target == null:
		return

	# Look-ahead se decide por velocidad horizontal del target si la tiene.
	var facing: float = 0.0
	if target is CharacterBody2D:
		facing = signf(target.velocity.x)

	_desired_offset_x = facing * look_ahead_distance

	# Lerp framerate-independent.
	var t: float = 1.0 - pow(1.0 - smoothing, delta * 60.0)
	_current_offset_x = lerpf(_current_offset_x, _desired_offset_x, t)

	# Posicionarse y snap a píxel.
	global_position = (target.global_position + Vector2(_current_offset_x, 0.0)).round()
