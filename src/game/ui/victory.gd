extends CanvasLayer

## Pantalla simple de victoria. Enter o el botón "Reintentar" reinician
## la run desde la primera sala.

signal restart_requested


func _ready() -> void:
	# La victoria vive sobre cualquier estado, incluso time_scale=0 si murió
	# durante hitstop. Process_mode ALWAYS asegura que reciba inputs.
	process_mode = Node.PROCESS_MODE_ALWAYS
	var btn: Button = $Panel/VBox/Restart as Button
	if btn != null:
		btn.pressed.connect(_emit_restart)
		btn.grab_focus()
	set_process_unhandled_input(true)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("ui_accept"):
		_emit_restart()
		get_viewport().set_input_as_handled()


func _emit_restart() -> void:
	restart_requested.emit()
