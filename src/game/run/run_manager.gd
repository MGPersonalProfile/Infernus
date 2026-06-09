extends Node

## Autoload "RunManager" — orquesta el ciclo de la run:
##   spawn → room → cleared → ability_choice → next room → … → boss → victory
##
## El Player y la Camera viven persistentes COMO HIJOS DE LA SCENE
## RAÍZ (run_root.tscn). El RunManager solo cambia la "room actual"
## (queue_free la vieja, instancia la nueva) y reposiciona al player
## en el spawn de la nueva room. Eso evita perder abilities al
## cambiar de escena.

signal room_loaded(room: Room)

var _player: Node
var _camera: Camera2D
var _current_room: Room
var _root: Node  # nodo donde meteremos las rooms (hermano del player)

const ABILITY_CHOICE_SCENE: PackedScene = preload(
	"res://src/game/ui/ability_choice.tscn"
)
const VICTORY_SCENE: PackedScene = preload(
	"res://src/game/ui/victory.tscn"
)


## Llamado por run_root._ready() después de instanciar el player y
## la camera. Carga la primera room.
func bind(root: Node, player: Node, camera: Camera2D) -> void:
	_root = root
	_player = player
	_camera = camera

	# Conectar muerte del player. Resetea la run.
	var hp: Node = _player.get_node_or_null("Health")
	if hp != null and not hp.died.is_connected(_on_player_died):
		hp.died.connect(_on_player_died)

	_load_room(RunState.current_room_path())


func _load_room(path: String) -> void:
	if path.is_empty():
		push_warning("RunManager: empty room path")
		return
	var packed: PackedScene = load(path) as PackedScene
	if packed == null:
		push_warning("RunManager: failed to load %s" % path)
		return

	if _current_room != null:
		# Desconectar room_cleared: si la room vieja no estaba cleared (ej.
		# player murió en mitad de pelea), la liberación de sus enemies
		# en el next frame disparó room_cleared en cascada y RunManager
		# encadenaba ability_choice fantasma.
		if _current_room.room_cleared.is_connected(_on_room_cleared):
			_current_room.room_cleared.disconnect(_on_room_cleared)
		_current_room.queue_free()
		_current_room = null

	var room: Room = packed.instantiate() as Room
	if room == null:
		push_warning("RunManager: scene %s is not a Room" % path)
		return

	_root.add_child(room)
	# Para que el player se renderice ENCIMA del world de la room,
	# lo movemos al final del orden de hijos.
	_root.move_child(_player, _root.get_child_count() - 1)
	_current_room = room

	# Reposicionar player y camera
	var spawn: Vector2 = room.get_player_spawn_position()
	if _player is Node2D:
		(_player as Node2D).global_position = spawn
	if _player.has_method("reset_velocity"):
		_player.reset_velocity()

	room.room_cleared.connect(_on_room_cleared, CONNECT_ONE_SHOT)
	room_loaded.emit(room)


func _on_room_cleared() -> void:
	# Si era la sala del boss, ganamos.
	if RunState.is_boss_room():
		_show_victory()
		return
	# Si no, mostramos pantalla de elección de ability.
	_show_ability_choice()


func _show_ability_choice() -> void:
	var ui: CanvasLayer = ABILITY_CHOICE_SCENE.instantiate() as CanvasLayer
	if ui == null:
		_advance_room_now()
		return
	_root.add_child(ui)
	if ui.has_signal("ability_picked"):
		ui.ability_picked.connect(_on_ability_picked.bind(ui))


func _on_ability_picked(ability: Ability, ui: Node) -> void:
	if ability != null:
		RunState.add_ability(ability)
		if ability.has_method("on_acquired"):
			ability.on_acquired(_player)
		# Si es ACTIVE, el player la equipa.
		if ability.kind == Ability.Kind.ACTIVE and _player.has_method("equip_active_ability"):
			_player.equip_active_ability(ability)
	if is_instance_valid(ui):
		ui.queue_free()
	_advance_room_now()


func _advance_room_now() -> void:
	RunState.advance_room()
	_load_room(RunState.current_room_path())


func _show_victory() -> void:
	var ui: CanvasLayer = VICTORY_SCENE.instantiate() as CanvasLayer
	if ui != null:
		_root.add_child(ui)
		if ui.has_signal("restart_requested"):
			ui.restart_requested.connect(_on_restart_requested.bind(ui))


func _on_restart_requested(ui: Node) -> void:
	if is_instance_valid(ui):
		ui.queue_free()
	_reset_run()


func _on_player_died() -> void:
	_reset_run()


func _reset_run() -> void:
	RunState.reset()
	# Reseteamos al player.
	var hp: Node = _player.get_node_or_null("Health")
	if hp != null:
		hp.current_hp = hp.max_hp
	var st: Node = _player.get_node_or_null("Stamina")
	if st != null:
		st.current_stamina = st.max_stamina
	if "damage_multiplier" in _player:
		_player.damage_multiplier = 1.0
	if _player.has_method("equip_active_ability"):
		_player.equip_active_ability(null)
	_load_room(RunState.current_room_path())
