extends Node

## Autoload singleton (registrado como "HitFx") con dos efectos de
## juice básicos: hitstop y screen shake.
##
## Cualquier sistema (player combat, futuras armas, projectiles) llama
## HitFx.hitstop(0.06) y HitFx.shake(4.0, 0.12) cuando conecta un golpe.
## El sistema no acopla a la cámara — busca la Camera2D activa por sí mismo.

var _hitstop_remaining: float = 0.0

var _shake_amplitude: float = 0.0
var _shake_duration: float = 0.0
var _shake_remaining: float = 0.0


func _ready() -> void:
	# El autoload sigue procesando cuando time_scale=0 (para terminar el hitstop).
	process_mode = Node.PROCESS_MODE_ALWAYS


func _process(delta: float) -> void:
	# Para hitstop usamos un delta real (sin time_scale), porque time_scale
	# está a 0. Engine.get_process_time() no nos sirve aquí — usamos
	# get_process_delta_time() del SceneTree, que sí lo da real cuando el
	# nodo está en PROCESS_MODE_ALWAYS.
	if _hitstop_remaining > 0.0:
		_hitstop_remaining -= delta
		if _hitstop_remaining <= 0.0:
			Engine.time_scale = 1.0

	_apply_shake(delta)


func hitstop(seconds: float) -> void:
	if seconds <= 0.0:
		return
	# Acumula — un golpe siguiente extiende el freeze, no lo solapa
	_hitstop_remaining = maxf(_hitstop_remaining, seconds)
	Engine.time_scale = 0.0


func shake(amplitude: float, duration: float) -> void:
	if amplitude <= 0.0 or duration <= 0.0:
		return
	# Si ya hay shake activo, usamos el que sea más fuerte (sin solapar)
	if amplitude > _shake_amplitude or _shake_remaining <= 0.0:
		_shake_amplitude = amplitude
		_shake_duration = duration
		_shake_remaining = duration


func _apply_shake(delta: float) -> void:
	var camera: Camera2D = _get_active_camera()
	if camera == null:
		return
	if _shake_remaining <= 0.0:
		camera.offset = Vector2.ZERO
		return
	_shake_remaining -= delta
	# Atenúa el shake linealmente: golpe fuerte al inicio, suave al final
	var fade: float = _shake_remaining / _shake_duration
	var amp: float = _shake_amplitude * fade
	var offset: Vector2 = Vector2(
		randf_range(-amp, amp),
		randf_range(-amp, amp)
	)
	camera.offset = offset.round()
	if _shake_remaining <= 0.0:
		camera.offset = Vector2.ZERO


func _get_active_camera() -> Camera2D:
	var viewport: Viewport = get_tree().root.get_viewport()
	if viewport == null:
		return null
	return viewport.get_camera_2d()
