extends CanvasLayer

## Pantalla simple de victoria. Enter o el botón "Reintentar" reinician
## la run desde la primera sala.

signal restart_requested


func _ready() -> void:
	var btn: Button = $Panel/VBox/Restart as Button
	if btn != null:
		btn.pressed.connect(_emit_restart)
	# También permitimos ENTER directamente
	set_process_unhandled_input(true)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		_emit_restart()


func _emit_restart() -> void:
	restart_requested.emit()
