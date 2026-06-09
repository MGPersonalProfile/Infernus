class_name Ability
extends Resource

## Resource base para abilities. Cada ability concreta hereda y
## override las funciones relevantes (on_acquired para passives y
## utility, on_active_pressed para actives).
##
## Las abilities son DATA + COMPORTAMIENTO en un Resource. Se cargan
## desde .tres y se "ejecutan" via los hooks. La capa de combate /
## player no conoce los detalles concretos — solo llama a los hooks.

enum Kind { ACTIVE, PASSIVE, UTILITY }

@export var id: StringName = &""
@export var display_name: String = "(unnamed)"
@export_multiline var description: String = ""
@export var kind: Kind = Kind.PASSIVE
## Tema para futuras sinergias (fire, blood, ice, peste, sombra, tormenta).
## En Semana 3 no se usa para nada — sólo lo guardamos.
@export var theme: StringName = &"none"


## Llamada cuando el jugador adquiere la ability (passives y utilities
## aplican su efecto aquí). Para ACTIVE puede usarse para wiring inicial.
func on_acquired(_player: Node) -> void:
	pass


## Llamada cada vez que el jugador pulsa el input de active ability.
## Solo relevante para ACTIVE. No hace nada en passives/utilities.
## Retorna true si la ability se ejecutó (consumió recurso, etc.).
func on_active_pressed(_player: Node) -> bool:
	return false
