class_name Room
extends Node2D

## Base class de cualquier sala jugable. Una sala tiene "spawn" para
## el player y un conjunto de enemigos. Cuando todos los enemigos
## están muertos, emite `room_cleared`.
##
## Convención: los enemigos deben estar agrupados como hijos directos
## de un nodo "Enemies" (Node2D). Se considera muerto cualquier
## enemigo que sale del árbol (queue_free) — Health.died ya lo
## gatilla en enemy_melee.gd y boss_minotaur.gd.

signal room_cleared

@export var player_spawn_path: NodePath = NodePath("PlayerSpawn")
@export var enemies_root_path: NodePath = NodePath("Enemies")

var _alive_enemies: int = 0
var _cleared: bool = false


func _ready() -> void:
	var enemies_root: Node = get_node_or_null(enemies_root_path)
	if enemies_root == null:
		# Sala sin enemigos — cleared al instante (útil para safe rooms).
		call_deferred("_emit_cleared")
		return

	for child in enemies_root.get_children():
		_alive_enemies += 1
		child.tree_exited.connect(_on_enemy_freed)

	if _alive_enemies == 0:
		call_deferred("_emit_cleared")


func get_player_spawn_position() -> Vector2:
	var marker: Node = get_node_or_null(player_spawn_path)
	if marker is Node2D:
		return (marker as Node2D).global_position
	return Vector2(200, 580)  # fallback razonable


func _on_enemy_freed() -> void:
	_alive_enemies -= 1
	if _alive_enemies <= 0 and not _cleared:
		_emit_cleared()


func _emit_cleared() -> void:
	if _cleared:
		return
	_cleared = true
	room_cleared.emit()
