class_name Player
extends CharacterBody2D

## Player controller: walk/jump/dash con coyote time, jump buffer y
## i-frames durante el dash. Todos los números viven en el Resource
## `PlayerMovementConfig` para tunearlos en caliente.

signal jumped
signal landed
signal dash_started
signal dash_ended

enum State { IDLE, RUN, JUMP, FALL, DASH }

@export var config: PlayerMovementConfig
## Path al Label del debug overlay. Se resuelve en _ready(). Usamos
## NodePath en vez de @export var debug_label: Label porque la
## asignación directa de Node desde un .tscn padre no se resuelve
## de forma fiable cuando el Player es una instancia de PackedScene.
@export var debug_label_path: NodePath

var state: State = State.IDLE
var facing: float = 1.0  # 1 derecha, -1 izquierda
var _debug_label: Label

# Timers (se decrementan a 0)
var _coyote_remaining: float = 0.0
var _jump_buffer_remaining: float = 0.0
var _dash_remaining: float = 0.0
var _dash_cooldown_remaining: float = 0.0
var _iframes_remaining: float = 0.0

var _was_on_floor: bool = false
var _dash_direction: float = 1.0


func _ready() -> void:
	if config == null:
		push_warning("Player: no PlayerMovementConfig assigned — usando defaults")
		config = PlayerMovementConfig.new()

	if debug_label_path != NodePath(""):
		var node: Node = get_node_or_null(debug_label_path)
		if node is Label:
			_debug_label = node
		else:
			push_warning("Player: debug_label_path no resuelve a un Label: %s" % str(debug_label_path))


func _physics_process(delta: float) -> void:
	_update_timers(delta)
	_read_input_buffers()

	# Estado DASH bloquea otras transiciones hasta agotarse
	if state == State.DASH:
		_process_dash(delta)
	else:
		_apply_gravity(delta)
		_handle_horizontal(delta)
		_handle_jump()
		_handle_dash()

	move_and_slide()
	_post_move_state_update()
	_update_debug_overlay()


func is_invulnerable() -> bool:
	return _iframes_remaining > 0.0


# === Tick interno ===

func _update_timers(delta: float) -> void:
	_coyote_remaining = maxf(0.0, _coyote_remaining - delta)
	_jump_buffer_remaining = maxf(0.0, _jump_buffer_remaining - delta)
	_dash_remaining = maxf(0.0, _dash_remaining - delta)
	_dash_cooldown_remaining = maxf(0.0, _dash_cooldown_remaining - delta)
	_iframes_remaining = maxf(0.0, _iframes_remaining - delta)


func _read_input_buffers() -> void:
	if Input.is_action_just_pressed("jump"):
		_jump_buffer_remaining = config.jump_buffer


# === Movimiento horizontal ===

func _handle_horizontal(delta: float) -> void:
	var input_dir: float = Input.get_axis("move_left", "move_right")
	var target_velocity: float = input_dir * config.move_speed

	var accel: float = config.accel_ground if is_on_floor() else config.accel_air
	var friction: float = config.friction_ground if is_on_floor() else config.friction_air

	if absf(input_dir) > 0.01:
		velocity.x = move_toward(velocity.x, target_velocity, accel * delta)
		facing = signf(input_dir)
	else:
		velocity.x = move_toward(velocity.x, 0.0, friction * delta)


# === Gravedad ===

func _apply_gravity(delta: float) -> void:
	if not is_on_floor():
		velocity.y += config.gravity * delta


# === Jump ===

func _handle_jump() -> void:
	var can_jump: bool = is_on_floor() or _coyote_remaining > 0.0
	if _jump_buffer_remaining > 0.0 and can_jump:
		velocity.y = config.jump_velocity
		_jump_buffer_remaining = 0.0
		_coyote_remaining = 0.0
		jumped.emit()

	# Jump cut: soltar el botón mientras subes corta la velocidad ascendente
	if Input.is_action_just_released("jump") and velocity.y < 0.0:
		velocity.y *= config.jump_cut_factor


# === Dash ===

func _handle_dash() -> void:
	if not Input.is_action_just_pressed("dash"):
		return
	if _dash_cooldown_remaining > 0.0:
		return
	_dash_direction = facing
	_dash_remaining = config.dash_duration
	_dash_cooldown_remaining = config.dash_cooldown
	_iframes_remaining = config.dash_iframes
	state = State.DASH
	velocity = Vector2(_dash_direction * config.dash_speed, 0.0)
	dash_started.emit()


func _process_dash(_delta: float) -> void:
	velocity = Vector2(_dash_direction * config.dash_speed, 0.0)
	if _dash_remaining <= 0.0:
		dash_ended.emit()


# === Resolución de estado tras move_and_slide ===

func _post_move_state_update() -> void:
	var on_floor_now: bool = is_on_floor()

	# Si acabamos de aterrizar
	if on_floor_now and not _was_on_floor:
		landed.emit()

	# Coyote arranca cuando dejamos el suelo sin saltar
	if _was_on_floor and not on_floor_now and velocity.y >= 0.0:
		_coyote_remaining = config.coyote_time

	# Si el dash terminó, volver a un estado normal
	if state == State.DASH and _dash_remaining <= 0.0:
		state = State.FALL if not on_floor_now else State.IDLE

	# Estados no-DASH
	if state != State.DASH:
		if not on_floor_now:
			state = State.JUMP if velocity.y < 0.0 else State.FALL
		else:
			state = State.RUN if absf(velocity.x) > 1.0 else State.IDLE

	_was_on_floor = on_floor_now


# === Debug ===

func _update_debug_overlay() -> void:
	if _debug_label == null:
		return
	_debug_label.text = (
		"state: %s\n" % State.keys()[state]
		+ "velocity: (%.0f, %.0f)\n" % [velocity.x, velocity.y]
		+ "on_floor: %s\n" % str(is_on_floor())
		+ "coyote: %.2f\n" % _coyote_remaining
		+ "jump_buffer: %.2f\n" % _jump_buffer_remaining
		+ "dash_cd: %.2f\n" % _dash_cooldown_remaining
		+ "iframes: %.2f" % _iframes_remaining
	)
