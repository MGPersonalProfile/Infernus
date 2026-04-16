"""
Procesador de buzon para Antigravity v2.0
==========================================
CLI helper que Antigravity puede ejecutar para ver tareas pendientes,
marcar como completadas, y consultar estado del bridge.

Uso desde Antigravity:
    python .ai-bridge/process_inbox.py list              # Ver tareas pendientes
    python .ai-bridge/process_inbox.py get <task_id>     # Ver tarea especifica
    python .ai-bridge/process_inbox.py complete <task_id> "<resultado>"
    python .ai-bridge/process_inbox.py dashboard         # Ver estado del bridge
    python .ai-bridge/process_inbox.py context [topic]   # Leer contexto compartido
    python .ai-bridge/process_inbox.py batches           # Ver batches activos
"""

import json
import sys
import io
from datetime import datetime
from pathlib import Path

# Fix Windows console encoding
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

BRIDGE_DIR = Path(__file__).parent
INBOX_DIR = BRIDGE_DIR / "antigravity-inbox"
CLAUDE_INBOX_DIR = BRIDGE_DIR / "claude-inbox"
RESPONSE_DIR = BRIDGE_DIR / "responses"
SHARED_CONTEXT_DIR = BRIDGE_DIR / "shared-context"
LOG_FILE = BRIDGE_DIR / "bridge_log.jsonl"

RESPONSE_DIR.mkdir(parents=True, exist_ok=True)


def _log(event: str, **kwargs):
    """Append event to bridge log."""
    entry = {"timestamp": datetime.now().isoformat(), "event": event, **kwargs}
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(json.dumps(entry, ensure_ascii=False) + "\n")


def _read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, data: dict):
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")


def _task_age(created_at: str) -> str:
    try:
        delta = datetime.now() - datetime.fromisoformat(created_at)
        hours = delta.total_seconds() / 3600
        if hours < 1:
            return f"{int(delta.total_seconds() / 60)}m"
        if hours < 24:
            return f"{hours:.1f}h"
        return f"{delta.days}d"
    except Exception:
        return "?"


def list_pending():
    """Lista todas las tareas pendientes para Antigravity."""
    tasks = []
    for f in sorted(INBOX_DIR.glob("*.json")):
        data = _read_json(f)
        if data.get("status") == "pending":
            tasks.append(data)

    if not tasks:
        print("No hay tareas pendientes.")
        return

    # Sort by priority
    priority_order = {"critical": 0, "high": 1, "medium": 2, "low": 3}
    tasks.sort(key=lambda t: priority_order.get(t.get("priority", "medium"), 2))

    print(f"{len(tasks)} tarea(s) pendiente(s):\n")
    for t in tasks:
        age = _task_age(t.get("created_at", ""))
        batch = f" [batch:{t['batch_id']}]" if t.get("batch_id") else ""
        print(f"  [{t['id']}] Prioridad: {t.get('priority', '?')} | {age} old{batch}")
        print(f"    Tarea: {t.get('task', '?')}")
        if t.get('context'):
            print(f"    Contexto: {str(t['context'])[:200]}")
        print()


def get_task(task_id):
    """Muestra detalles completos de una tarea."""
    task_path = INBOX_DIR / f"{task_id}.json"
    if not task_path.exists():
        print(f"Tarea '{task_id}' no encontrada.")
        return

    data = _read_json(task_path)
    print(json.dumps(data, indent=2, ensure_ascii=False))


def complete_task(task_id, result):
    """Marca una tarea como completada y guarda la respuesta."""
    task_path = INBOX_DIR / f"{task_id}.json"
    if not task_path.exists():
        print(f"Tarea '{task_id}' no encontrada.")
        return

    task_data = _read_json(task_path)

    # Mark as completed
    task_data["status"] = "completed"
    task_data["completed_at"] = datetime.now().isoformat()
    _write_json(task_path, task_data)

    # Create response
    response_data = {
        "id": task_id,
        "from": "antigravity",
        "to": task_data.get("from", "claude-code"),
        "type": "response",
        "original_task": task_data.get("task", ""),
        "result": result,
        "status": "completed",
        "created_at": task_data.get("created_at"),
        "completed_at": datetime.now().isoformat(),
    }

    response_path = RESPONSE_DIR / f"{task_id}_response.json"
    _write_json(response_path, response_data)

    _log("task_completed", task_id=task_id, by="antigravity")

    print(f"Tarea '{task_id}' completada.")
    print(f"Respuesta guardada en: {response_path}")


def show_dashboard():
    """Muestra estado general del bridge."""
    pending = 0
    completed = 0
    stale = 0
    total = 0

    for f in INBOX_DIR.glob("*.json"):
        data = _read_json(f)
        total += 1
        status = data.get("status", "pending")
        if status == "pending":
            pending += 1
            # Check deadline
            deadline = data.get("deadline", "")
            if deadline:
                try:
                    if datetime.now() > datetime.fromisoformat(deadline):
                        stale += 1
                except Exception:
                    pass
        elif status == "completed":
            completed += 1

    pending_claude = sum(
        1 for f in CLAUDE_INBOX_DIR.glob("task_*.json")
        if _read_json(f).get("status") == "pending"
    )

    print("=== AI BRIDGE DASHBOARD ===\n")
    print(f"Tareas -> Antigravity:  {pending} pendientes" +
          (f" ({stale} STALE)" if stale else ""))
    print(f"Tareas -> Claude:       {pending_claude} pendientes")
    print(f"Completadas total:      {completed}")
    print(f"Total historico:        {total}")

    # Show context topics
    print("\nContexto compartido:")
    for f in sorted(SHARED_CONTEXT_DIR.glob("*.md")):
        age = _task_age(datetime.fromtimestamp(f.stat().st_mtime).isoformat())
        print(f"  {f.stem}: {age} ago")


def read_context(topic=None):
    """Lee contexto compartido."""
    if topic:
        path = SHARED_CONTEXT_DIR / f"{topic}.md"
        if path.exists():
            print(path.read_text(encoding="utf-8"))
        else:
            print(f"No hay contexto para '{topic}'.")
    else:
        for f in sorted(SHARED_CONTEXT_DIR.glob("*.md")):
            print(f"=== {f.stem} ===")
            print(f.read_text(encoding="utf-8"))
            print()


def show_batches():
    """Muestra batches activos."""
    batches = {}
    for f in INBOX_DIR.glob("*.json"):
        data = _read_json(f)
        bid = data.get("batch_id")
        if not bid:
            continue
        if bid not in batches:
            batches[bid] = {"tasks": [], "completed": 0, "total": 0}
        batches[bid]["total"] += 1
        response_path = RESPONSE_DIR / f"{data['id']}_response.json"
        if response_path.exists() or data.get("status") == "completed":
            batches[bid]["completed"] += 1
        batches[bid]["tasks"].append({
            "id": data["id"],
            "status": data.get("status", "?"),
            "task": data.get("task", "")[:80],
        })

    if not batches:
        print("No hay batches.")
        return

    for bid, info in batches.items():
        print(f"\n{bid}: {info['completed']}/{info['total']} completadas")
        for t in info["tasks"]:
            mark = "[x]" if t["status"] == "completed" else "[ ]"
            print(f"  {mark} {t['id']} — {t['task']}")


def main():
    if len(sys.argv) < 2:
        print("AI Bridge CLI v2.0 — Procesador de buzon para Antigravity\n")
        print("Uso: python process_inbox.py <comando> [args...]\n")
        print("Comandos:")
        print("  list                        - Ver tareas pendientes")
        print("  get <task_id>               - Ver detalles de una tarea")
        print('  complete <task_id> "result"  - Marcar como completada')
        print("  dashboard                   - Ver estado del bridge")
        print("  context [topic]             - Leer contexto compartido")
        print("  batches                     - Ver batches activos")
        return

    command = sys.argv[1].lower()

    if command == "list":
        list_pending()
    elif command == "get" and len(sys.argv) >= 3:
        get_task(sys.argv[2])
    elif command == "complete" and len(sys.argv) >= 4:
        complete_task(sys.argv[2], sys.argv[3])
    elif command == "dashboard":
        show_dashboard()
    elif command == "context":
        read_context(sys.argv[2] if len(sys.argv) >= 3 else None)
    elif command == "batches":
        show_batches()
    else:
        print(f"Comando desconocido o argumentos insuficientes: {command}")
        print("Usa: python process_inbox.py (sin args) para ver ayuda")


if __name__ == "__main__":
    main()
