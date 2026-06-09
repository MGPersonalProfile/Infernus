class_name BossMinotaur
extends CharacterBody2D

## Boss placeholder del Círculo VII. Tres fases marcadas por
## umbrales de HP: cambia color + velocidad + daño de contacto.
## Sin attack windup todavía (contact damage) — patrones reales
## de boss son Semana 4 dedicada.

signal phase_changed(new_phase: int)
signal boss_defeated

enum State { PATROL, CHASE, STAGGER, DEAD }

@export var base_speed: float = 70.0
@export var gravity: float = 1300.0
@export var base_contact_damage: int = 18
@export var contact_cooldown: float = 0.8
@export var stagger_duration: float = 0.20
@export var stagger_friction: float = 600.0
@export var aggro_range: float = 700.0
@export var patrol_left_offset: float = -250.0
@export var patrol_right_offset: float = 250.0

@export var player_path: NodePath
@export var health_path: NodePath

var state: State = State.PATROL
var current_phase: int = 1  # 1 / 2 / 3

var _player: Node2D
var _health: Node
var _patrol_left_x: float
var _patrol_right_x: float
var _patrol_direction: float = 1.0
var _contact_cd: float = 0.0
var _stagger_remaining: float = 0.0

var _sprite: Polygon2D
var _phase_colors: Array[Color] = [
	Color(0.40, 0.25, 0.15, 1),  # fase 1: marrón oscuro
	Color(0.65, 0.20, 0.10, 1),  # fase 2: rojizo
	Color(0.90, 0.15, 0.05, 1),  # fase 3: rojo brillante
]


func _ready() -> void:
	_patrol_left_x = global_position.x + patrol_left_offset
	_patrol_right_x = global_position.x + patrol_right_offset
	if player_path != NodePath(""):
		_player = get_node_or_null(player_path) as Node2D
	if health_path != NodePath(""):
		_health = get_node_or_null(health_path)
		if _health != null:
			_health.damaged.connect(_on_damaged)
			_health.died.connect(_on_died)
	_sprite = get_node_or_null("Sprite") as Polygon2D
	if _sprite != null:
		_sprite.color = _phase_colors[0]


func _physics_process(delta: float) -> void:
	if state == State.DEAD:
		return

	if _stagger_remaining > 0.0:
		_stagger_remaining -= delta
		if _stagger_remaining <= 0.0:
			state = State.PATROL

	if _contact_cd > 0.0:
		_contact_cd -= delta

	if not is_on_floor():
		velocity.y += gravity * delta

	match state:
		State.PATROL:
			_tick_patrol()
			_check_chase_trigger()
		State.CHASE:
			_tick_chase()
		State.STAGGER:
			velocity.x = move_toward(velocity.x, 0.0, stagger_friction * delta)

	move_and_slide()
	_try_damage_player()


# === States ===

func _tick_patrol() -> void:
	if global_position.x <= _patrol_left_x:
		_patrol_direction = 1.0
	elif global_position.x >= _patrol_right_x:
		_patrol_direction = -1.0
	velocity.x = _patrol_direction * _current_speed()


func _check_chase_trigger() -> void:
	if _player == null:
		return
	if global_position.distance_to(_player.global_position) <= aggro_range:
		state = State.CHASE


func _tick_chase() -> void:
	if _player == null:
		state = State.PATROL
		return
	var dist: float = global_position.distance_to(_player.global_position)
	if dist > aggro_range * 1.4:
		state = State.PATROL
		return
	var dir: float = signf(_player.global_position.x - global_position.x)
	velocity.x = dir * _current_speed()


# === Combat / Phases ===

func _current_speed() -> float:
	match current_phase:
		2: return base_speed * 1.5
		3: return base_speed * 2.0
		_: return base_speed


func _current_contact_damage() -> int:
	match current_phase:
		2: return int(base_contact_damage * 1.5)
		3: return int(base_contact_damage * 2.0)
		_: return base_contact_damage


func _try_damage_player() -> void:
	if _player == null or _contact_cd > 0.0:
		return
	# Hitbox conceptual: si el sprite del player está dentro del rango
	# del boss (~40 px considerando el tamaño del boss), aplicamos daño.
	if global_position.distance_to(_player.global_position) > 36.0:
		return
	if not _player.has_method("take_damage"):
		return
	if _player.call("take_damage", _current_contact_damage(), self):
		_contact_cd = contact_cooldown


func _on_damaged(_amount: int, _source: Node) -> void:
	state = State.STAGGER
	_stagger_remaining = stagger_duration
	_maybe_advance_phase()


func _maybe_advance_phase() -> void:
	if _health == null:
		return
	var pct: float = float(_health.current_hp) / float(_health.max_hp)
	var new_phase: int = current_phase
	if pct <= 0.33:
		new_phase = 3
	elif pct <= 0.66:
		new_phase = 2
	else:
		new_phase = 1
	if new_phase != current_phase:
		current_phase = new_phase
		if _sprite != null and current_phase - 1 < _phase_colors.size():
			_sprite.color = _phase_colors[current_phase - 1]
		phase_changed.emit(current_phase)


func _on_died() -> void:
	state = State.DEAD
	boss_defeated.emit()
	queue_free()
