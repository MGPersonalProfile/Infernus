-- INFERNUS — Game Feel Tuning
-- Hot-reload con F5. Modificable en vivo desde DebugPanel (F12).
-- Toda variable nombrada `feel.foo` es accesible desde C++ via LuaEngine::getFeel("foo").

feel = {
    -- === SPIKE: 5 valores iniciales para validar el loop end-to-end ===
    -- (Después de validación, expandimos a 30+)

    -- Hitstop — pausa global en impacto, da peso al combate
    hitstop_normal       = 0.06,   -- s, hits regulares (>=20 dmg o crit)
    hitstop_crit         = 0.10,   -- s, futuros crits específicos
    hitstop_parry        = 0.10,   -- s, parry exitoso

    -- Screen shake — intensidad y duración base
    shake_normal_intensity = 5.0,  -- pixels
    shake_crit_intensity   = 10.0, -- pixels
    shake_parry_intensity  = 12.0, -- pixels (parry success — más intenso)
}

-- Validación: imprime cuando se recarga (visible en consola del juego)
print("[feel.lua] Cargado. " .. (function()
    local n = 0
    for _ in pairs(feel) do n = n + 1 end
    return n
end)() .. " parámetros activos.")
