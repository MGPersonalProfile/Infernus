class_name EnemyMelee
extends CharacterBody2D

## Enemigo simple para validar el combate del Semana 2. AI mínima:
## patrulla entre dos offsets relativos a su spawn, y si el player
## entra en chase_range cambia a perseguirlo. Daño al contacto.
##
## Sin attack windup propio — la AI de enemigos reales (con tells,
## telegraph, etc.) es Semana 3. Aquí solo necesitamos un sparring
## partner para el director sienta el combat.

enum State { PATROL, CHASE, STAGGER, DEAD }

@export var speed: float = 90.0
@export var gravity: float = 1300.0
@export var chase_range: float = 250.0
@export var contact_damage: int = 10
@export var contact_cooldown: float = 0.6
@export var stagger_duration: float = 0.30
@export var stagger_friction: float = 500.0

## Offsets desde la posición inicial para los extremos del patrullaje.
@export var patrol_left_offset: float = -150.0
@export var patrol_right_offset: float = 150.0

## NodePath al player. Sin él, el enemigo solo patrulla.
@export var player_path: NodePath

@export var health_path: NodePath

var state: State = State.PATROL
var _player: Node2D
var _health: HealthComponent

var _patrol_left_x: float
var _patrol_right_x: float
var _patrol_direction: float = 1.0
var _contact_cd_remaining: float = 0.0
var _stagger_remaining: float = 0.0

# Visual feedback al recibir hit: el sprite parpadea blanco
const _HIT_FLASH_DURATION: float = 0.20
var _sprite: Polygon2D
var _sprite_base_color: Color = Color(0.30, 0.55, 0.35, 1)
var _hit_flash_remaining: float = 0.0


func _ready() -> void:
	_patrol_left_x = global_position.x + patrol_left_offset
	_patrol_right_x = global_position.x + patrol_right_offset
	if player_path != NodePath(""):
		_player = get_node_or_null(player_path) as Node2D
	if health_path != NodePath(""):
		_health = get_node_or_null(health_path) as HealthComponent
		if _health != null:
			_health.damaged.connect(_on_damaged)
			_health.died.connect(_on_died)
	_sprite = get_node_or_null("Sprite") as Polygon2D
	if _sprite != null:
		_sprite_base_color = _sprite.color


func _physics_process(delta: float) -> void:
	if state == State.DEAD:
		return

	if _stagger_remaining > 0.0:
		_stagger_remaining -= delta
		if _stagger_remaining <= 0.0:
			state = State.PATROL

	if _contact_cd_remaining > 0.0:
		_contact_cd_remaining -= delta

	if _hit_flash_remaining > 0.0:
		_hit_flash_remaining -= delta
		if _hit_flash_remaining <= 0.0 and _sprite != null:
			_sprite.color = _sprite_base_color

	# Gravedad siempre
	if not is_on_floor():
		velocity.y += gravity * delta

	match state:
		State.PATROL:
			_tick_patrol()
			_check_chase_trigger()
		State.CHASE:
			_tick_chase()
		State.STAGGER:
			# Stagger: friction horizontal mucho más blanda que el movimiento
			# normal, para que el knockback se deje SENTIR antes de pararse.
			velocity.x = move_toward(velocity.x, 0.0, stagger_friction * delta)

	move_and_slide()
	_try_damage_player()


# === States ===

func _tick_patrol() -> void:
	if global_position.x <= _patrol_left_x:
		_patrol_direction = 1.0
	elif global_position.x >= _patrol_right_x:
		_patrol_direction = -1.0
	velocity.x = _patrol_direction * speed


func _check_chase_trigger() -> void:
	if _player == null:
		return
	var dist: float = global_position.distance_to(_player.global_position)
	if dist <= chase_range:
		state = State.CHASE


func _tick_chase() -> void:
	if _player == null:
		state = State.PATROL
		return
	var dist: float = global_position.distance_to(_player.global_position)
	if dist > chase_range * 1.3:
		# El player escapó del cono — vuelve a patrullar.
		state = State.PATROL
		return
	var dir: float = signf(_player.global_position.x - global_position.x)
	velocity.x = dir * speed


# === Daño al player ===

func _try_damage_player() -> void:
	if _player == null or _contact_cd_remaining > 0.0:
		return
	# Aproximación simple: si la distancia es menor que media-suma de tamaños
	# (24px aproximado), aplicamos el daño. Más fino sería detectar via Area2D,
	# pero esto es suficiente para Semana 2.
	if global_position.distance_to(_player.global_position) > 24.0:
		return
	if not _player.has_method("take_damage"):
		return
	if _player.call("take_damage", contact_damage, self):
		_contact_cd_remaining = contact_cooldown


# === Hooks de Health ===

func _on_damaged(amount: int, source: Node) -> void:
	# DEBUG temporal: si esto no aparece en la consola, el callback nunca
	# se ejecuta (problema de signal connect). Si aparece, el daño se
	# aplica y debería verse el flash/knockback.
	print("[enemy] damaged ", amount, " by ", source, " hp=", _health.current_hp if _health else "?")
	state = State.STAGGER
	_stagger_remaining = stagger_duration
	if _sprite != null:
		_sprite.color = Color.WHITE
		_hit_flash_remaining = _HIT_FLASH_DURATION


func _on_died() -> void:
	state = State.DEAD
	queue_free()
