class_name AbilityPool
extends Object

## Helper estático para rolear abilities. En Semana 3 el pool es de
## sólo 3 abilities (una de cada tipo), por lo que `roll(3)` siempre
## devuelve las mismas. Cuando crezca el pool (Semana 4+) la lógica
## de aquí se vuelve relevante.

const POOL_PATHS: Array[String] = [
	"res://content/abilities/lanza_flegetonte.tres",
	"res://content/abilities/sangre_ardiente.tres",
	"res://content/abilities/curacion_menor.tres",
]


static func load_all() -> Array[Ability]:
	var result: Array[Ability] = []
	for path in POOL_PATHS:
		var res: Resource = load(path)
		if res is Ability:
			result.append(res)
	return result


## Devuelve `n` abilities distintas. Si el pool tiene menos de `n`,
## devuelve el pool entero. No filtra por owned (en Semana 3 el
## jugador puede stackear la misma — útil para la pasiva).
static func roll(n: int) -> Array[Ability]:
	var all := load_all()
	if all.size() <= n:
		return all
	all.shuffle()
	return all.slice(0, n)
