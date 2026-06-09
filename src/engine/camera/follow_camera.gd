class_name FollowCamera
extends Camera2D

## Cámara que sigue a un Node2D con offset según la dirección de
## movimiento (look-ahead lateral). Snap final a píxel entero para
## evitar que las texturas vibren en pixel art.
##
## NodePath en vez de @export var target: Node2D porque la asignación
## directa de Node desde un .tscn padre no se resuelve de forma fiable
## cuando este nodo vive en una scene root. Resolvemos en _ready().

@export var target_path: NodePath
@export_range(0.0, 200.0, 5.0) var look_ahead_distance: float = 60.0
@export_range(0.0, 1.0, 0.01) var smoothing: float = 0.12

var _target: Node2D
var _desired_offset_x: float = 0.0
var _current_offset_x: float = 0.0


func _ready() -> void:
	make_current()
	if target_path != NodePath(""):
		var node: Node = get_node_or_null(target_path)
		if node is Node2D:
			_target = node
		else:
			push_warning("FollowCamera: target_path no resuelve a un Node2D: %s" % str(target_path))


func _process(delta: float) -> void:
	if _target == null:
		return

	# Look-ahead se decide por velocidad horizontal del target si la tiene.
	var facing: float = 0.0
	if _target is CharacterBody2D:
		facing = signf((_target as CharacterBody2D).velocity.x)

	_desired_offset_x = facing * look_ahead_distance

	# Lerp framerate-independent.
	var t: float = 1.0 - pow(1.0 - smoothing, delta * 60.0)
	_current_offset_x = lerpf(_current_offset_x, _desired_offset_x, t)

	# Posicionarse y snap a píxel.
	global_position = (_target.global_position + Vector2(_current_offset_x, 0.0)).round()
