class_name StaminaComponent
extends Node

## Componente reusable de stamina. Cualquier entidad que tenga que
## limitar acciones por un recurso regenerable lo usa.
##
## Regenera continuamente excepto durante una "ventana de delay" tras
## el último gasto — eso evita que la stamina se sienta como un goteo
## constante y crea ritmo souls-like ("descansa antes de volver al
## ataque").

signal spent(amount: float)
signal exhausted   # Se emite cuando un try_spend deja la stamina a 0

@export var max_stamina: float = 100.0
@export var regen_per_second: float = 30.0
@export var regen_delay_after_use: float = 0.5

var current_stamina: float

var _regen_block_remaining: float = 0.0


func _ready() -> void:
	current_stamina = max_stamina


func _process(delta: float) -> void:
	if _regen_block_remaining > 0.0:
		_regen_block_remaining -= delta
		return
	if current_stamina < max_stamina:
		current_stamina = minf(max_stamina, current_stamina + regen_per_second * delta)


func has(amount: float) -> bool:
	return current_stamina >= amount


func try_spend(amount: float) -> bool:
	if amount <= 0.0:
		return true
	if not has(amount):
		return false
	current_stamina -= amount
	spent.emit(amount)
	_regen_block_remaining = regen_delay_after_use
	if current_stamina <= 0.0:
		current_stamina = 0.0
		exhausted.emit()
	return true


func fraction() -> float:
	return current_stamina / max_stamina if max_stamina > 0.0 else 0.0
