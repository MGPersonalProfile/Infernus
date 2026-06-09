class_name PassiveSangreArdiente
extends Ability

## PASSIVE: +15% daño a todos los ataques cuerpo a cuerpo. Aplica al
## adquirirla multiplicando el `damage_multiplier` del Player.
## El PlayerCombat lee ese multiplier al calcular dmg en cada hit.

@export var damage_bonus: float = 0.15


func on_acquired(player: Node) -> void:
	if player == null:
		return
	if not "damage_multiplier" in player:
		return
	player.damage_multiplier *= (1.0 + damage_bonus)
