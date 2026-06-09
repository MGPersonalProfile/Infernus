class_name UtilityCuracionMenor
extends Ability

## UTILITY: cura +30 HP al instante de adquirirla. Útil entre salas
## si recibiste daño en la pelea anterior. Cura, no overheal.

@export var heal_amount: int = 30


func on_acquired(player: Node) -> void:
	if player == null:
		return
	var hp: Node = player.get_node_or_null("Health")
	if hp == null:
		return
	if hp.has_method("heal"):
		hp.heal(heal_amount)
