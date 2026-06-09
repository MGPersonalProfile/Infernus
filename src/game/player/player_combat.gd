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

	# Hitboxes monitorean siempre. NO conectamos area_entered porque
	# en Godot las áreas que YA están dentro al pasar monitoring de
	# false→true no se reportan vía signal (solo cuando ENTRAN nuevo).
	# Como atacas estando cerca del enemigo, su HurtBox suele estar
	# dentro antes del active → sin signal, sin daño. Workaround:
	# durante ACTIVE preguntamos manualmente get_overlapping_areas()
	# cada frame y procesamos las que aún no hayamos golpeado.
	if _hitbox_light:
		_hitbox_light.monitoring = true
	if _hitbox_heavy:
		_hitbox_heavy.monitoring = true


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
			_check_active_hits()
			_tick_state(delta, _enter_recovery)
		AttackState.RECOVERY:
			_tick_state(delta, _enter_idle_with_combo_window)
			_handle_input() # permite encadenar light durante recovery


func is_attacking() -> bool:
	return state == AttackState.WINDUP or state == AttackState.ACTIVE


# === Input ===

func _handle_input() -> void:
	if Input.is_action_just_pressed("attack_light"):
		print("[combat] attack_light pressed, state=", AttackState.keys()[state])
		_try_start(AttackKind.LIGHT)
	elif Input.is_action_just_pressed("attack_heavy"):
		print("[combat] attack_heavy pressed, state=", AttackState.keys()[state])
		_try_start(AttackKind.HEAVY)


func _try_start(kind: int) -> void:
	if config == null:
		print("[combat] _try_start abort: config is null")
		return
	var cost: float
	if kind == AttackKind.LIGHT:
		cost = config.light_stamina_cost
	else:
		cost = config.heavy_stamina_cost
	if _stamina != null and not _stamina.try_spend(cost):
		print("[combat] _try_start abort: not enough stamina (need ", cost, ", have ", _stamina.current_stamina, ")")
		return
	_current_kind = kind
	if kind == AttackKind.LIGHT:
		_state_timer = config.light_windup
	else:
		_state_timer = config.heavy_windup
	state = AttackState.WINDUP
	_already_hit_this_swing.clear()
	attack_started.emit(kind)
	attack_in_progress_changed.emit(true)
	print("[combat] attack started kind=", kind, " windup=", _state_timer)


# === State transitions ===

func _tick_state(delta: float, on_finished: Callable) -> void:
	_state_timer -= delta
	if _state_timer <= 0.0:
		on_finished.call()


func _enter_active() -> void:
	if _current_kind == AttackKind.LIGHT:
		_state_timer = config.light_active
	else:
		_state_timer = config.heavy_active
	state = AttackState.ACTIVE
	_orient_hitboxes()
	print("[combat] ACTIVE entered, timer=", _state_timer)


func _enter_recovery() -> void:
	if _current_kind == AttackKind.LIGHT:
		_state_timer = config.light_recovery
	else:
		_state_timer = config.heavy_recovery
	state = AttackState.RECOVERY


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
	var sx: float = 1.0
	if facing != 0.0:
		sx = facing
	if _hitbox_light:
		_hitbox_light.scale.x = sx
	if _hitbox_heavy:
		_hitbox_heavy.scale.x = sx


# === Hit detection ===

## Cada physics_frame durante ACTIVE: pregunta al hitbox actual qué
## HurtBoxes está tocando y procesa las que aún no hayamos pegado en
## este swing. _already_hit_this_swing evita doble-tick por target.
func _check_active_hits() -> void:
	var box: Area2D
	if _current_kind == AttackKind.LIGHT:
		box = _hitbox_light
	else:
		box = _hitbox_heavy
	if box == null:
		return
	var areas := box.get_overlapping_areas()
	if not areas.is_empty():
		print("[combat] ACTIVE checking ", areas.size(), " overlapping areas")
	for area in areas:
		_process_hit(area, _current_kind)


func _process_hit(area: Area2D, kind: int) -> void:
	# Las HurtBox son Area2D hijas del nodo "víctima" (Enemy, etc.).
	# El target es el parent de la HurtBox, que debería tener un hijo "Health".
	var target: Node = area.get_parent()
	print("[combat] _process_hit area=", area.name, " parent=", target.name if target else "null")
	if target == null or target in _already_hit_this_swing:
		return
	_already_hit_this_swing.append(target)
	var health: HealthComponent = target.get_node_or_null("Health") as HealthComponent
	if health == null:
		print("[combat] target has no Health child, skip")
		return
	var dmg: int
	if kind == AttackKind.LIGHT:
		dmg = config.light_damage
	else:
		dmg = config.heavy_damage
	if not health.take_damage(dmg, _player):
		print("[combat] take_damage returned false")
		return
	# Knockback al target si tiene velocity (CharacterBody2D)
	if target is CharacterBody2D and _player != null:
		var kb: float
		if kind == AttackKind.LIGHT:
			kb = config.light_knockback
		else:
			kb = config.heavy_knockback
		var dir: float = signf(target.global_position.x - _player.global_position.x)
		if dir == 0.0:
			dir = 1.0
		(target as CharacterBody2D).velocity.x = dir * kb
	# Juice
	var hitstop_s: float
	var shake_amp: float
	var shake_dur: float
	if kind == AttackKind.LIGHT:
		hitstop_s = config.light_hitstop
		shake_amp = config.light_shake_amplitude
		shake_dur = config.light_shake_duration
	else:
		hitstop_s = config.heavy_hitstop
		shake_amp = config.heavy_shake_amplitude
		shake_dur = config.heavy_shake_duration
	HitFx.hitstop(hitstop_s)
	HitFx.shake(shake_amp, shake_dur)
	attack_hit.emit(target, kind)
