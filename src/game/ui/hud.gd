class_name Hud
extends CanvasLayer

## HUD mínimo de Semana 2: HP bar, stamina bar y debug label.
## Se conecta a los componentes del player vía NodePath y los lee
## cada frame. Cero acoplamiento — si el player no existe, no falla.

@export var player_path: NodePath

@onready var _hp_bar: ProgressBar = $Container/HpBar
@onready var _stamina_bar: ProgressBar = $Container/StaminaBar
@onready var _debug_label: Label = $DebugLabel

var _health: HealthComponent
var _stamina: StaminaComponent


func _ready() -> void:
	if player_path == NodePath(""):
		return
	var player: Node = get_node_or_null(player_path)
	if player == null:
		push_warning("HUD: player_path no resuelve")
		return
	_health = player.get_node_or_null("Health") as HealthComponent
	_stamina = player.get_node_or_null("Stamina") as StaminaComponent

	if _hp_bar != null and _health != null:
		_hp_bar.max_value = _health.max_hp
		_hp_bar.value = _health.current_hp
	if _stamina_bar != null and _stamina != null:
		_stamina_bar.max_value = _stamina.max_stamina
		_stamina_bar.value = _stamina.current_stamina


func _process(_delta: float) -> void:
	if _hp_bar != null and _health != null:
		_hp_bar.value = _health.current_hp
	if _stamina_bar != null and _stamina != null:
		_stamina_bar.value = _stamina.current_stamina
