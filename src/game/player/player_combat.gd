class_name PlayerCombat
extends Node

## Componente de combate del jugador. State machine independiente del
## movement: WINDUP → ACTIVE → RECOVERY. Maneja light/heavy y combos.
##
## Notifica al Player cuándo bloquear movimiento (durante WINDUP+ACTIVE)
## y a HitFx cuando un golpe conecta (hitstop + screen shake).

signal attack_in_progress_changed(in_progress: bool)
signal attack_hit(target: Node, kind: int)
signal attack_started(kind: int)

enum AttackState { IDLE, WINDUP, ACTIVE, RECOVERY }
enum AttackKind { LIGHT, HEAVY }

@export var config: PlayerCombatConfig
## NodePath al StaminaComponent. Resolvemos en _ready() — mismo motivo
## que debug_label y FollowCamera.target: la asignación @export
## directa desde un .tscn parent no se resuelve en instancias.
@export var stamina_path: NodePath
## NodePath relativo a este Node hacia el Area2D del hitbox light.
@export var hitbox_light_path: NodePath
## NodePath relativo a este Node hacia el Area2D del hitbox heavy.
@export var hitbox_heavy_path: NodePath
## Referencia al Player CharacterBody2D para leer su `facing` y orientar
## el hitbox al lado correcto.
@export var player_path: NodePath

var _stamina: StaminaComponent

var state: AttackState = AttackState.IDLE
var _current_kind: AttackKind = AttackKind.LIGHT
var _state_timer: float = 0.0
var _combo_window_remaining: float = 0.0

var _hitbox_light: Area2D
var _hitbox_heavy: Area2D
var _player: Node2D

var _already_hit_this_swing: Array[Node] = []


func _ready() -> void:
	if stamina_path != NodePath(""):
		_stamina = get_node_or_null(stamina_path) as StaminaComponent
	if hitbox_light_path != NodePath(""):
		_hitbox_light = get_node_or_null(hitbox_light_path) as Area2D
	if hitbox_heavy_path != NodePath(""):
		_hitbox_heavy = get_node_or_null(hitbox_heavy_path) as Area2D
	if player_path != NodePath(""):
		_player = get_node_or_null(player_path) as Node2D

	if _hitbox_light:
		_hitbox_light.monitoring = false
		_hitbox_light.area_entered.connect(_on_area_entered.bind(AttackKind.LIGHT))
	if _hitbox_heavy:
		_hitbox_heavy.monitoring = false
		_hitbox_heavy.area_entered.connect(_on_area_entered.bind(AttackKind.HEAVY))


func _process(delta: float) -> void:
	if _combo_window_remaining > 0.0:
		_combo_window_remaining = maxf(0.0, _combo_window_remaining - delta)

	match state:
		AttackState.IDLE:
			_handle_input()
		AttackState.WINDUP:
			_tick_state(delta, _enter_active)
		AttackState.ACTIVE:
			_orient_hitboxes()
			_tick_state(delta, _enter_recovery)
		AttackState.RECOVERY:
			_tick_state(delta, _enter_idle_with_combo_window)
			_handle_input() # permite encadenar light durante recovery


func is_attacking() -> bool:
	return state == AttackState.WINDUP or state == AttackState.ACTIVE


# === Input ===

func _handle_input() -> void:
	if Input.is_action_just_pressed("attack_light"):
		_try_start(AttackKind.LIGHT)
	elif Input.is_action_just_pressed("attack_heavy"):
		_try_start(AttackKind.HEAVY)


func _try_start(kind: int) -> void:
	if config == null:
		return
	var cost: float = config.light_stamina_cost if kind == AttackKind.LIGHT else config.heavy_stamina_cost
	if _stamina != null and not _stamina.try_spend(cost):
		return
	_current_kind = kind
	_state_timer = config.light_windup if kind == AttackKind.LIGHT else config.heavy_windup
	state = AttackState.WINDUP
	_already_hit_this_swing.clear()
	attack_started.emit(kind)
	attack_in_progress_changed.emit(true)


# === State transitions ===

func _tick_state(delta: float, on_finished: Callable) -> void:
	_state_timer -= delta
	if _state_timer <= 0.0:
		on_finished.call()


func _enter_active() -> void:
	_state_timer = config.light_active if _current_kind == AttackKind.LIGHT else config.heavy_active
	state = AttackState.ACTIVE
	var box: Area2D = _hitbox_light if _current_kind == AttackKind.LIGHT else _hitbox_heavy
	if box:
		_orient_hitboxes()
		box.monitoring = true


func _enter_recovery() -> void:
	_state_timer = config.light_recovery if _current_kind == AttackKind.LIGHT else config.heavy_recovery
	state = AttackState.RECOVERY
	if _hitbox_light:
		_hitbox_light.monitoring = false
	if _hitbox_heavy:
		_hitbox_heavy.monitoring = false


func _enter_idle_with_combo_window() -> void:
	state = AttackState.IDLE
	_combo_window_remaining = config.combo_window
	attack_in_progress_changed.emit(false)


# === Hitbox orientation ===

func _orient_hitboxes() -> void:
	if _player == null:
		return
	var facing: float = 1.0
	if "facing" in _player:
		facing = _player.facing
	if _hitbox_light:
		_hitbox_light.scale.x = facing if facing != 0.0 else 1.0
	if _hitbox_heavy:
		_hitbox_heavy.scale.x = facing if facing != 0.0 else 1.0


# === Hit detection ===

func _on_area_entered(area: Area2D, kind: int) -> void:
	# Las HurtBox son Area2D hijas del nodo "víctima" (Enemy, etc.).
	# El target es el parent de la HurtBox, que debería tener un hijo "Health".
	var target: Node = area.get_parent()
	if target == null or target in _already_hit_this_swing:
		return
	_already_hit_this_swing.append(target)
	var health: HealthComponent = target.get_node_or_null("Health") as HealthComponent
	if health == null:
		return
	var dmg: int = config.light_damage if kind == AttackKind.LIGHT else config.heavy_damage
	if not health.take_damage(dmg, _player):
		return
	# Knockback al target si tiene velocity (CharacterBody2D)
	if target is CharacterBody2D and _player != null:
		var kb: float = config.light_knockback if kind == AttackKind.LIGHT else config.heavy_knockback
		var dir: float = signf(target.global_position.x - _player.global_position.x)
		if dir == 0.0:
			dir = 1.0
		(target as CharacterBody2D).velocity.x = dir * kb
	# Juice
	var hitstop_s: float = config.light_hitstop if kind == AttackKind.LIGHT else config.heavy_hitstop
	var shake_amp: float = config.light_shake_amplitude if kind == AttackKind.LIGHT else config.heavy_shake_amplitude
	var shake_dur: float = config.light_shake_duration if kind == AttackKind.LIGHT else config.heavy_shake_duration
	HitFx.hitstop(hitstop_s)
	HitFx.shake(shake_amp, shake_dur)
	attack_hit.emit(target, kind)
