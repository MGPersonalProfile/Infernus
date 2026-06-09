class_name PlayerCombatConfig
extends Resource

## Tuning del combate del jugador. Resource para hot-reload sin
## recompilar — edita en el inspector mientras el juego corre.

# === Light attack ===
@export_range(0.0, 0.5, 0.01) var light_windup: float = 0.08
@export_range(0.0, 0.5, 0.01) var light_active: float = 0.10
@export_range(0.0, 1.0, 0.01) var light_recovery: float = 0.18
@export_range(0, 200, 1) var light_damage: int = 10
@export_range(0.0, 100.0, 1.0) var light_stamina_cost: float = 15.0
@export_range(0.0, 600.0, 10.0) var light_knockback: float = 80.0
@export_range(0.0, 0.5, 0.01) var light_hitstop: float = 0.05
@export_range(0.0, 10.0, 0.5) var light_shake_amplitude: float = 2.5
@export_range(0.0, 0.5, 0.01) var light_shake_duration: float = 0.10

# === Heavy attack ===
@export_range(0.0, 1.0, 0.01) var heavy_windup: float = 0.25
@export_range(0.0, 0.5, 0.01) var heavy_active: float = 0.15
@export_range(0.0, 1.5, 0.01) var heavy_recovery: float = 0.40
@export_range(0, 300, 1) var heavy_damage: int = 25
@export_range(0.0, 100.0, 1.0) var heavy_stamina_cost: float = 30.0
@export_range(0.0, 800.0, 10.0) var heavy_knockback: float = 200.0
@export_range(0.0, 0.5, 0.01) var heavy_hitstop: float = 0.10
@export_range(0.0, 15.0, 0.5) var heavy_shake_amplitude: float = 5.0
@export_range(0.0, 0.5, 0.01) var heavy_shake_duration: float = 0.18

# === Combos ===
## Ventana después del recovery durante la cual se acepta un siguiente
## light attack como combo (encadenado, sin re-windup completo).
@export_range(0.0, 0.5, 0.01) var combo_window: float = 0.15

# === Dash stamina cost (gestionado por Player, no Combat — pero vive
# aquí para que un solo Resource describa todo el tuning ofensivo) ===
@export_range(0.0, 100.0, 1.0) var dash_stamina_cost: float = 25.0
