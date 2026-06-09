class_name ActiveLanzaFlegetonte
extends Ability

## ACTIVE: lanza un proyectil de fuego horizontal hacia donde mira
## el player. Consume stamina. El proyectil atraviesa hasta golpear
## el primer enemigo y se autodestruye.

@export var stamina_cost: float = 20.0
@export var projectile_speed: float = 480.0
@export var projectile_damage: int = 18
@export var projectile_lifetime: float = 1.5


func on_active_pressed(player: Node) -> bool:
	if player == null:
		return false

	# Stamina check
	var stamina: Node = player.get_node_or_null("Stamina")
	if stamina != null and not stamina.try_spend(stamina_cost):
		return false

	# Direction
	var facing: float = 1.0
	if "facing" in player:
		facing = player.facing
	if facing == 0.0:
		facing = 1.0

	_spawn_projectile(player, facing)
	return true


func _spawn_projectile(player: Node, facing: float) -> void:
	var proj := Projectile.new()
	proj.name = "LanzaFlegetonte"
	proj.global_position = player.global_position + Vector2(20.0 * facing, -4.0)
	proj.velocity = Vector2(facing * projectile_speed, 0.0)
	proj.damage = projectile_damage
	proj.lifetime = projectile_lifetime
	proj.source = player

	# Sprite placeholder: barra naranja
	var sprite := Polygon2D.new()
	sprite.color = Color(1.0, 0.55, 0.20, 1.0)
	sprite.polygon = PackedVector2Array([
		Vector2(-10, -3), Vector2(10, -3), Vector2(10, 3), Vector2(-10, 3),
	])
	proj.add_child(sprite)

	# Hit area (Area2D + CollisionShape2D)
	var area := Area2D.new()
	area.name = "Hit"
	area.collision_layer = 0
	area.collision_mask = 16  # capa de HurtBoxes enemy
	area.monitoring = true
	area.monitorable = false
	var shape := CollisionShape2D.new()
	var rect := RectangleShape2D.new()
	rect.size = Vector2(20, 6)
	shape.shape = rect
	area.add_child(shape)
	proj.add_child(area)

	# Lo añadimos al árbol al nivel del current_scene para que sea
	# independiente del player (no se mueva con él).
	player.get_tree().current_scene.add_child(proj)
