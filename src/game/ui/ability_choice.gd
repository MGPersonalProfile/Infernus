extends CanvasLayer

## Pantalla de elección de ability entre salas. Muestra 3 cards
## generadas por AbilityPool.roll(3) y emite `ability_picked` cuando
## el jugador elige una.
##
## En Semana 3 el pool es de 3 abilities, así que las 3 cards
## siempre son las mismas. Reroll oculto hasta que el pool > 3.

signal ability_picked(ability: Ability)

@onready var _cards_container: HBoxContainer = $Panel/VBox/Cards


func _ready() -> void:
	var rolled: Array[Ability] = AbilityPool.roll(3)
	for ability in rolled:
		_add_card(ability)


func _add_card(ability: Ability) -> void:
	var card := Button.new()
	card.custom_minimum_size = Vector2(280, 320)
	card.toggle_mode = false
	card.text = ""
	card.pressed.connect(func(): _on_card_pressed(ability))

	var vbox := VBoxContainer.new()
	vbox.anchor_right = 1.0
	vbox.anchor_bottom = 1.0
	vbox.offset_left = 16
	vbox.offset_top = 16
	vbox.offset_right = -16
	vbox.offset_bottom = -16
	vbox.add_theme_constant_override("separation", 12)
	card.add_child(vbox)

	# Tipo (header coloreado)
	var kind_label := Label.new()
	kind_label.text = _kind_text(ability.kind)
	kind_label.add_theme_color_override("font_color", _kind_color(ability.kind))
	kind_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	vbox.add_child(kind_label)

	# Nombre
	var name_label := Label.new()
	name_label.text = ability.display_name
	name_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	name_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	vbox.add_child(name_label)

	# Separador visual
	var sep := HSeparator.new()
	vbox.add_child(sep)

	# Descripción
	var desc_label := Label.new()
	desc_label.text = ability.description
	desc_label.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	desc_label.vertical_alignment = VERTICAL_ALIGNMENT_TOP
	desc_label.add_theme_color_override("font_color", Color(0.85, 0.85, 0.80, 1))
	vbox.add_child(desc_label)

	_cards_container.add_child(card)


func _kind_text(kind: int) -> String:
	match kind:
		Ability.Kind.ACTIVE:  return "ACTIVA  [Q]"
		Ability.Kind.PASSIVE: return "PASIVA"
		Ability.Kind.UTILITY: return "UTILIDAD"
		_:                    return "?"


func _kind_color(kind: int) -> Color:
	match kind:
		Ability.Kind.ACTIVE:  return Color(0.85, 0.25, 0.20, 1)  # rojo
		Ability.Kind.PASSIVE: return Color(0.85, 0.65, 0.20, 1)  # ámbar
		Ability.Kind.UTILITY: return Color(0.40, 0.75, 0.45, 1)  # verde
		_:                    return Color.WHITE


func _on_card_pressed(ability: Ability) -> void:
	ability_picked.emit(ability)
