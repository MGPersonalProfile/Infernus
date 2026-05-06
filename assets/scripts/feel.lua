-- INFERNUS - Game Feel Tuning
-- Hot-reload con F5. Modificable en vivo desde DebugPanel (F12).
-- Toda variable nombrada `feel.foo` es accesible desde C++ via LuaEngine::getFeel("foo").
--
-- Convenciones:
--   *_intensity    : pixels (screen shake)
--   *_duration     : segundos
--   *_window       : segundos (parry window, combo window)
--   *_iframes      : segundos
--   *_stamina      : puntos (de 100 max)
--   *_speed        : px/segundo
--   *_windup       : segundos (multiplicado por class windupMult)

feel = {
    -- === Combat hitstop (pausa global) ===
    hitstop_normal       = 0.06,   -- s, hits regulares (>=20 dmg)
    hitstop_crit         = 0.10,   -- s, crits
    hitstop_parry        = 0.10,   -- s, parry exitoso

    -- === Screen shake ===
    shake_normal_intensity = 5.0,   -- pixels, hits regulares
    shake_crit_intensity   = 10.0,  -- pixels, crits
    shake_parry_intensity  = 12.0,  -- pixels, parry success
    shake_normal_duration  = 0.2,   -- s
    shake_crit_duration    = 0.3,   -- s
    shake_parry_duration   = 0.3,   -- s

    -- === Player movement ===
    player_speed         = 250.0,   -- px/s base (modificado por items y clase)
    dash_speed           = 1200.0,  -- px/s peak durante el dash
    dash_iframes         = 0.3,     -- s de invulnerabilidad
    dash_stamina         = 30.0,    -- coste

    -- === Player attacks ===
    light_attack_windup  = 0.1,     -- s, antes del active frame
    heavy_attack_windup  = 0.4,     -- s
    light_attack_stamina = 20.0,
    heavy_attack_stamina = 40.0,

    -- === Parry ===
    parry_window         = 0.2,     -- s active
    parry_recovery       = 0.4,     -- s lock si fallas
    parry_stagger_time   = 1.5,     -- s stagger al enemigo en parry exitoso
    parry_stamina        = 15.0,

    -- === Combo ===
    combo_window         = 0.35,    -- s para encadenar el siguiente hit

    -- === Hit response ===
    hit_iframes          = 0.3,     -- s tras recibir un hit
    hit_flash_time       = 0.1,     -- s de tinte rojo en sprite
    hitbox_active_time   = 0.15,    -- s que el hitbox de un attack está activo
    attack_recovery_time = 0.2,     -- s de RECOVERY tras ACTIVE

    -- === Camera ===
    camera_lerp_speed    = 5.0,     -- factor de smooth follow (mayor = más rápido)
    camera_shake_decay   = 10.0,    -- factor de decay del shake
    camera_zoom          = 1.0,     -- multiplicador de zoom (default 1.0; probar 1.5 en Fase 2)

    -- === AI ===
    stagger_duration     = 0.3,     -- s default cuando un enemigo es staggered

    -- === Particles ===
    hit_particles_min    = 3.0,     -- cantidad min de gotas de sangre (int cast)
    hit_particles_max    = 6.0,     -- cantidad max
    hit_particle_lifetime = 0.3,    -- s
}

-- Validación: imprime cuando se recarga (visible en consola del juego)
print("[feel.lua] Cargado. " .. (function()
    local n = 0
    for _ in pairs(feel) do n = n + 1 end
    return n
end)() .. " parámetros activos.")
