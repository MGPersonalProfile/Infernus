extends Node2D

## Scene raíz de la run. Contiene los nodos PERSISTENTES (Player,
## Camera, HUD) y delega al autoload RunManager para que cargue la
## primera room y orquestre transiciones.

@onready var _player: Node = $Player
@onready var _camera: Camera2D = $Camera


func _ready() -> void:
	RunState.reset()
	# Deferred: RunRoot aún está añadiendo Player/Camera/HUD a su tree.
	# bind() llama add_child(room), que falla síncrono durante _ready del parent.
	RunManager.bind.call_deferred(self, _player, _camera)
