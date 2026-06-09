class_name Projectile
extends Node2D

## Proyectil simple: avanza horizontal, daña el primer HurtBox con
## Health que toca, autodestruye al impactar o por timeout.
##
## Usado por abilities activas tipo "Lanza de Flegetonte". El árbol
## espera un Area2D hijo llamado "Hit" que dispara area_entered.

@export var velocity: Vector2 = Vector2.ZERO
@export var damage: int = 18
@export var lifetime: float = 1.5

var source: Node


func _ready() -> void:
	var area: Area2D = get_node_or_null("Hit") as Area2D
	if area != null:
		area.area_entered.connect(_on_area_entered)


func _physics_process(delta: float) -> void:
	position += velocity * delta
	lifetime -= delta
	if lifetime <= 0.0:
		queue_free()


func _on_area_entered(area: Area2D) -> void:
	var target: Node = area.get_parent()
	if target == null:
		return
	var hp: Node = target.get_node_or_null("Health")
	if hp == null:
		return
	if hp.has_method("take_damage"):
		hp.take_damage(damage, source)
	queue_free()
