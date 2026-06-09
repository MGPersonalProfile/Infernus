extends Node

## Autoload singleton (registrado como "HitFx") con dos efectos de
## juice básicos: hitstop y screen shake.
##
## OJO: como ponemos `Engine.time_scale = 0` durante el hitstop,
## NO podemos usar `delta` para medir su duración — ese delta sería
## 0 también (PROCESS_MODE_ALWAYS te exime de la pausa, pero no de
## la escala de tiempo). Usamos `Time.get_ticks_msec()` que cuenta
## tiempo real del sistema y avanza independientemente de time_scale.

var _hitstop_end_ticks_msec: int = 0

var _shake_amplitude: float = 0.0
var _shake_start_ticks_msec: int = 0
var _shake_end_ticks_msec: int = 0


func _ready() -> void:
	process_mode = Node.PROCESS_MODE_ALWAYS


func _process(_delta: float) -> void:
	var now_msec: int = Time.get_ticks_msec()
	if _hitstop_end_ticks_msec > 0 and now_msec >= _hitstop_end_ticks_msec:
		_hitstop_end_ticks_msec = 0
		Engine.time_scale = 1.0
	_apply_shake(now_msec)


func hitstop(seconds: float) -> void:
	if seconds <= 0.0:
		return
	# Acumula — un hitstop adicional puede extender el freeze pero no acortarlo.
	var new_end_msec: int = Time.get_ticks_msec() + int(seconds * 1000.0)
	if new_end_msec > _hitstop_end_ticks_msec:
		_hitstop_end_ticks_msec = new_end_msec
	Engine.time_scale = 0.0


func shake(amplitude: float, duration: float) -> void:
	if amplitude <= 0.0 or duration <= 0.0:
		return
	var now_msec: int = Time.get_ticks_msec()
	# Si ya hay shake activo, sólo nos quedamos con el más fuerte (sin solapar).
	if amplitude > _shake_amplitude or now_msec >= _shake_end_ticks_msec:
		_shake_amplitude = amplitude
		_shake_start_ticks_msec = now_msec
		_shake_end_ticks_msec = now_msec + int(duration * 1000.0)


func _apply_shake(now_msec: int) -> void:
	var camera: Camera2D = _get_active_camera()
	if camera == null:
		return
	if _shake_end_ticks_msec <= 0 or now_msec >= _shake_end_ticks_msec:
		camera.offset = Vector2.ZERO
		return
	# Atenúa linealmente: golpe fuerte al inicio, suave al final.
	var total: int = _shake_end_ticks_msec - _shake_start_ticks_msec
	var remaining: int = _shake_end_ticks_msec - now_msec
	var fade: float = float(remaining) / float(maxi(total, 1))
	var amp: float = _shake_amplitude * fade
	var offset: Vector2 = Vector2(
		randf_range(-amp, amp),
		randf_range(-amp, amp)
	)
	camera.offset = offset.round()


func _get_active_camera() -> Camera2D:
	var viewport: Viewport = get_tree().root.get_viewport()
	if viewport == null:
		return null
	return viewport.get_camera_2d()
