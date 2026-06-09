class_name PlayerMovementConfig
extends Resource

## Tuning del movimiento del jugador. Resource para hot-reload sin
## recompilar — edita en el inspector mientras el juego corre y los
## cambios se aplican al siguiente frame.

# === Velocidad / aceleración ===
@export_range(0.0, 500.0, 5.0) var move_speed: float = 180.0
@export_range(0.0, 5000.0, 50.0) var accel_ground: float = 1600.0
@export_range(0.0, 5000.0, 50.0) var friction_ground: float = 1400.0
@export_range(0.0, 5000.0, 50.0) var accel_air: float = 800.0
@export_range(0.0, 5000.0, 50.0) var friction_air: float = 200.0

# === Gravedad / salto ===
@export_range(0.0, 3000.0, 50.0) var gravity: float = 1300.0
@export_range(-1000.0, 0.0, 10.0) var jump_velocity: float = -420.0
@export_range(0.0, 1.0, 0.05) var jump_cut_factor: float = 0.4

# === Forgiveness ===
@export_range(0.0, 0.5, 0.01) var coyote_time: float = 0.10
@export_range(0.0, 0.5, 0.01) var jump_buffer: float = 0.12

# === Dash ===
@export_range(0.0, 1500.0, 10.0) var dash_speed: float = 480.0
@export_range(0.0, 1.0, 0.01) var dash_duration: float = 0.18
@export_range(0.0, 3.0, 0.05) var dash_cooldown: float = 0.55
@export_range(0.0, 1.0, 0.01) var dash_iframes: float = 0.18
