class_name HealthComponent
extends Node

## Componente reusable de salud. Cualquier entidad que pueda recibir
## daño tiene uno como hijo (Player, Enemy, futuras props destructibles).
##
## Soporta i-frames temporales para integrarse con el dash del player
## y futuros parry. El owner decide cómo usar las señales.

signal damaged(amount: int, source: Node)
signal healed(amount: int)
signal died

@export var max_hp: int = 100

var current_hp: int

var _iframes_remaining: float = 0.0


func _ready() -> void:
	current_hp = max_hp


func _process(delta: float) -> void:
	if _iframes_remaining > 0.0:
		_iframes_remaining = maxf(0.0, _iframes_remaining - delta)


func is_invulnerable() -> bool:
	return _iframes_remaining > 0.0


func is_dead() -> bool:
	return current_hp <= 0


func take_damage(amount: int, source: Node = null) -> bool:
	if amount <= 0:
		return false
	if is_dead():
		return false
	if is_invulnerable():
		return false
	current_hp = maxi(0, current_hp - amount)
	damaged.emit(amount, source)
	if current_hp == 0:
		died.emit()
	return true


func heal(amount: int) -> void:
	if amount <= 0 or is_dead():
		return
	current_hp = mini(max_hp, current_hp + amount)
	healed.emit(amount)


func set_invulnerable(seconds: float) -> void:
	_iframes_remaining = maxf(_iframes_remaining, seconds)
