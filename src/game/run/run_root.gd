extends Node2D

## Scene raíz de la run. Contiene los nodos PERSISTENTES (Player,
## Camera, HUD) y delega al autoload RunManager para que cargue la
## primera room y orquestre transiciones.

@onready var _player: Node = $Player
@onready var _camera: Camera2D = $Camera


func _ready() -> void:
	RunState.reset()
	RunManager.bind(self, _player, _camera)
