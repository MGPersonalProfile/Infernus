-- Boss pattern selection — hot-reload with F5.
-- This function is called each time the boss needs a new attack pattern.
--
-- Arguments:
--   phase    (int)   — current boss phase, 0-based
--   hp_ratio (float) — boss HP fraction, 0.0 to 1.0
--   patterns (table) — available pattern names {"charge","ground_slam",...}
--
-- Return: one of the pattern name strings.
-- If this function errors or is missing, C++ falls back to random selection.

function select_boss_pattern(phase, hp_ratio, patterns)
    local n = #patterns
    if n == 0 then return "charge" end

    -- Enraged behavior: below 30% HP, heavily favor aggressive patterns
    if hp_ratio < 0.3 then
        for i = 1, n do
            if patterns[i] == "enraged_charge" then return patterns[i] end
            if patterns[i] == "combo" then return patterns[i] end
        end
    end

    -- Phase 2+: bias toward slams and combos
    if phase >= 1 then
        local heavy = {}
        for i = 1, n do
            local p = patterns[i]
            if p == "ground_slam" or p == "combo" or p == "stomp" then
                table.insert(heavy, p)
            end
        end
        if #heavy > 0 and math.random() < 0.6 then
            return heavy[math.random(#heavy)]
        end
    end

    -- Default: weighted random from all available
    return patterns[math.random(n)]
end
