extends Node

## Autoload "RunState" — el estado mutable de una run en curso.
## Vive entre cambios de escena. Se resetea al morir o al ganar.
##
## Lo que guarda:
## - Índice de la sala actual + paths de las salas en orden
## - Abilities adquiridas durante esta run
## - Contador de rerolls usados en la elección actual (escala el coste)

signal abilities_changed

const ROOM_PATHS: Array[String] = [
	"res://scenes/rooms/room_01.tscn",
	"res://scenes/rooms/room_02.tscn",
	"res://scenes/rooms/room_boss.tscn",
]

var current_room_index: int = 0
var owned_abilities: Array[Ability] = []
var reroll_count_this_choice: int = 0


func reset() -> void:
	current_room_index = 0
	owned_abilities.clear()
	reroll_count_this_choice = 0
	abilities_changed.emit()


func advance_room() -> void:
	current_room_index += 1
	reroll_count_this_choice = 0


func add_ability(a: Ability) -> void:
	if a == null:
		return
	owned_abilities.append(a)
	abilities_changed.emit()


func current_room_path() -> String:
	if current_room_index < 0 or current_room_index >= ROOM_PATHS.size():
		return ""
	return ROOM_PATHS[current_room_index]


func is_boss_room() -> bool:
	return current_room_index == ROOM_PATHS.size() - 1


func is_post_boss() -> bool:
	return current_room_index >= ROOM_PATHS.size()


## Coste del próximo reroll en %HP máximo. Escala: 5% × (1 + count).
func next_reroll_cost_percent() -> float:
	return 0.05 * float(1 + reroll_count_this_choice)
