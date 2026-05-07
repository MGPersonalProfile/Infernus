"""
Antigravity MCP Bridge Server v2.0
===================================
Servidor MCP que permite comunicacion bidireccional entre Claude Code y
Antigravity (Gemini). Incluye logging, templates, validacion de entregables,
timeout/retry, contexto estructurado, batch tasks, y dashboard.

Registro:
    claude mcp add antigravity-bridge -- python .ai-bridge/antigravity_mcp_server.py

Herramientas (17 tools):
    --- Core ---
    request_antigravity     Enviar tarea a Antigravity
    check_antigravity_response  Verificar respuesta de una tarea
    send_task_to_claude     Crear tarea para Claude (usada por Antigravity)
    list_pending_tasks      Ver tareas pendientes

    --- Responses ---
    get_all_pending_responses   Leer TODAS las respuestas no procesadas
    validate_deliverables       Verificar que archivos entregados existen

    --- Reliability ---
    check_stale_tasks       Detectar tareas con timeout
    retry_task              Re-enviar tarea fallida/expirada

    --- Context ---
    update_context          Escribir contexto compartido (por topico)
    read_context            Leer contexto compartido (por topico o todo)

    --- Automation ---
    request_from_template   Crear tarea desde template predefinido
    request_batch           Enviar grupo de tareas relacionadas
    check_batch             Verificar estado de un batch

    --- Monitoring ---
    bridge_dashboard        Resumen completo del estado del bridge
    bridge_log              Ver eventos recientes del log
    bridge_stats            Estadisticas de uso
    archive_completed       Archivar tareas completadas antiguas

    --- Legacy (backward-compatible) ---
    update_shared_context   Alias de update_context(topic="general")
    read_shared_context     Alias de read_context(topic="general")
"""

import json
import re
import time
from datetime import datetime, timedelta
from difflib import SequenceMatcher
from pathlib import Path

from fastmcp import FastMCP

# ─── Paths ───────────────────────────────────────────────────────────
BRIDGE_DIR = Path(__file__).parent
PROJECT_ROOT = BRIDGE_DIR.parent
INBOX_DIR = BRIDGE_DIR / "antigravity-inbox"
CLAUDE_INBOX_DIR = BRIDGE_DIR / "claude-inbox"
PROCESSED_DIR = CLAUDE_INBOX_DIR / "processed"
RESPONSE_DIR = BRIDGE_DIR / "responses"
SHARED_CONTEXT_DIR = BRIDGE_DIR / "shared-context"
TEMPLATE_DIR = BRIDGE_DIR / "templates"
ARCHIVE_DIR = BRIDGE_DIR / "archive"
LOG_FILE = BRIDGE_DIR / "bridge_log.jsonl"

for d in [INBOX_DIR, CLAUDE_INBOX_DIR, PROCESSED_DIR, RESPONSE_DIR,
          SHARED_CONTEXT_DIR, TEMPLATE_DIR, ARCHIVE_DIR]:
    d.mkdir(parents=True, exist_ok=True)

# Default timeout for tasks (hours)
DEFAULT_TIMEOUT_HOURS = 2.0

# v3 hygiene constants
LOG_ROTATE_BYTES = 1_000_000          # rotate at ~1 MB
LOG_KEEP_ROTATIONS = 3                 # keep last N rotated files
AUTO_ARCHIVE_THRESHOLD_HOURS = 24.0    # archive completed tasks older than this on startup/request
EXPIRE_AFTER_DAYS = 7.0                # tasks stale > this go to archive/expired/

# ─── Logging ─────────────────────────────────────────────────────────

def _rotate_log_if_needed():
    """If bridge_log.jsonl exceeds LOG_ROTATE_BYTES, rotate to a timestamped
    file and prune old rotations. Idempotent and cheap (size check only)."""
    try:
        if not LOG_FILE.exists():
            return
        if LOG_FILE.stat().st_size < LOG_ROTATE_BYTES:
            return
        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        rotated = LOG_FILE.with_name(f"bridge_log.{ts}.jsonl")
        LOG_FILE.rename(rotated)
        # Prune oldest rotations beyond LOG_KEEP_ROTATIONS
        rotations = sorted(BRIDGE_DIR.glob("bridge_log.*.jsonl"))
        excess = len(rotations) - LOG_KEEP_ROTATIONS
        for old in rotations[:max(0, excess)]:
            old.unlink()
    except Exception:
        # Never let log maintenance break the server
        pass


def _log(event: str, **kwargs):
    """Append a structured event to bridge_log.jsonl. Auto-rotates at ~1 MB."""
    _rotate_log_if_needed()
    entry = {
        "timestamp": datetime.now().isoformat(),
        "event": event,
        **kwargs,
    }
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(json.dumps(entry, ensure_ascii=False) + "\n")


def _auto_maintenance():
    """Lightweight upkeep run at startup and before each new task creation:
    - Archive completed tasks older than AUTO_ARCHIVE_THRESHOLD_HOURS
    - Move tasks stale > EXPIRE_AFTER_DAYS to archive/expired/
    Returns dict {archived: N, expired: M} for logging."""
    archived = 0
    expired = 0
    try:
        archive_cutoff = datetime.now() - timedelta(hours=AUTO_ARCHIVE_THRESHOLD_HOURS)
        expire_cutoff  = datetime.now() - timedelta(days=EXPIRE_AFTER_DAYS)
        month_dir = ARCHIVE_DIR / archive_cutoff.strftime("%Y-%m")
        expired_dir = ARCHIVE_DIR / "expired"
        month_dir.mkdir(parents=True, exist_ok=True)
        expired_dir.mkdir(parents=True, exist_ok=True)

        for f in list(INBOX_DIR.glob("*.json")):
            try:
                data = _read_json(f)
            except Exception:
                continue
            status = data.get("status", "")
            created_at = data.get("created_at", "")
            completed_at = data.get("completed_at") or created_at

            # Archive completed tasks (and their responses) past threshold
            if status in ("completed", "failed", "retried") and completed_at:
                try:
                    ct = datetime.fromisoformat(completed_at)
                    if ct <= archive_cutoff:
                        f.rename(month_dir / f.name)
                        rp = RESPONSE_DIR / f"{data['id']}_response.json"
                        if rp.exists():
                            rp.rename(month_dir / rp.name)
                        archived += 1
                        continue
                except Exception:
                    pass

            # Expire pending tasks past EXPIRE_AFTER_DAYS
            if status == "pending" and created_at:
                try:
                    ct = datetime.fromisoformat(created_at)
                    if ct <= expire_cutoff:
                        # Mark expired in the file before moving so audit trail survives
                        data["status"] = "expired"
                        data["expired_at"] = datetime.now().isoformat()
                        _write_json(f, data)
                        f.rename(expired_dir / f.name)
                        expired += 1
                except Exception:
                    pass
    except Exception:
        pass
    return {"archived": archived, "expired": expired}


# ─── Helpers ─────────────────────────────────────────────────────────

def _read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _write_json(path: Path, data: dict):
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")


def _task_age_str(created_at: str) -> str:
    """Human-readable age from ISO timestamp."""
    try:
        created = datetime.fromisoformat(created_at)
        delta = datetime.now() - created
        hours = delta.total_seconds() / 3600
        if hours < 1:
            return f"{int(delta.total_seconds() / 60)}m"
        if hours < 24:
            return f"{hours:.1f}h"
        return f"{delta.days}d {int(hours % 24)}h"
    except Exception:
        return "?"


def _is_stale(created_at: str, timeout_hours: float = DEFAULT_TIMEOUT_HOURS) -> bool:
    """Check if a task has exceeded its timeout."""
    try:
        created = datetime.fromisoformat(created_at)
        return datetime.now() - created > timedelta(hours=timeout_hours)
    except Exception:
        return False


def _extract_file_paths(text: str) -> list[str]:
    """Extract file paths mentioned in response text."""
    patterns = [
        r'assets/[^\s,\'")\]]+',        # assets/... paths
        r'src/[^\s,\'")\]]+',           # src/... paths
        r'[a-zA-Z_/]+\.(?:png|jpg|wav|json|lua|ldtk)',  # files with known extensions
    ]
    paths = []
    for pattern in patterns:
        paths.extend(re.findall(pattern, text))
    return list(set(paths))


def _make_task_id() -> str:
    return f"task_{int(time.time() * 1000)}"


def _normalize_for_dedup(text: str) -> str:
    """Lowercase + collapse whitespace + strip common stopwords for fuzzy match."""
    t = text.lower()
    # Strip URLs and paths to focus on intent
    t = re.sub(r'https?://\S+', '', t)
    t = re.sub(r'[/\\][\w./\\-]+', ' ', t)
    t = re.sub(r'\s+', ' ', t).strip()
    return t


def _find_duplicate(task_text: str, threshold: float = 0.75) -> dict | None:
    """Search pending tasks in INBOX_DIR for a similar one. Returns the
    matching task dict if similarity > threshold, else None.
    Uses difflib's SequenceMatcher on normalized prefixes (first 200 chars).
    """
    norm_new = _normalize_for_dedup(task_text)[:200]
    if len(norm_new) < 20:
        return None  # too short to be meaningful
    best = None
    best_ratio = 0.0
    for f in INBOX_DIR.glob("*.json"):
        try:
            d = _read_json(f)
        except Exception:
            continue
        if d.get("status") != "pending":
            continue
        norm_old = _normalize_for_dedup(d.get("task", ""))[:200]
        if len(norm_old) < 20:
            continue
        ratio = SequenceMatcher(None, norm_new, norm_old).ratio()
        if ratio > best_ratio:
            best_ratio = ratio
            best = d
    if best and best_ratio >= threshold:
        return {**best, "_similarity": round(best_ratio, 3)}
    return None


# ─── FastMCP Server ──────────────────────────────────────────────────

mcp = FastMCP("antigravity-bridge")

# Run hygiene at server startup so any stale state from prior sessions is
# tidied before tools fire. Cheap and idempotent.
_startup_stats = _auto_maintenance()
if _startup_stats["archived"] or _startup_stats["expired"]:
    _log("auto_maintenance_startup",
         archived=_startup_stats["archived"], expired=_startup_stats["expired"])


# ═══════════════════════════════════════════════════════════════════════
#  CORE TOOLS
# ═══════════════════════════════════════════════════════════════════════

@mcp.tool
def request_antigravity(task: str, context: str = "", priority: str = "medium",
                        timeout_hours: float = DEFAULT_TIMEOUT_HOURS,
                        force: bool = False,
                        tags: list[str] | None = None) -> str:
    """Envia una tarea a Antigravity (Gemini).

    Delega: imagenes, sprites, web search, browser, analisis visual, paletas.
    NO delegues: C++, CMake, JSON gameplay, bugs.

    Args:
        task: Descripcion detallada de la tarea
        context: Contexto adicional (archivos, restricciones, dimensiones, paleta)
        priority: low | medium | high | critical
        timeout_hours: Horas antes de considerar la tarea como stale (default 2)
        force: Si True, salta el check de duplicados (default False)
        tags: Etiquetas opcionales para filtrar luego (ej: ['sprite', 'ui', 'audio'])
    """
    # Lightweight hygiene before creating a new task (no-op if nothing to do)
    _auto_maintenance()

    # Dedup check: warn if a near-identical task is already pending
    if not force:
        dup = _find_duplicate(task)
        if dup:
            sim_pct = int(dup["_similarity"] * 100)
            return (
                f"DUPLICADO POSIBLE — tarea similar ya pendiente.\n"
                f"  Existing: {dup['id']} ({sim_pct}% similar, {dup.get('priority')}, "
                f"{_task_age_str(dup.get('created_at',''))} old)\n"
                f"  Summary: {dup.get('task', '')[:140]}\n\n"
                f"Si es realmente diferente, llama otra vez con force=True."
            )

    task_id = _make_task_id()
    deadline = (datetime.now() + timedelta(hours=timeout_hours)).isoformat()

    task_data = {
        "id": task_id,
        "from": "claude-code",
        "to": "antigravity",
        "priority": priority,
        "type": "request",
        "task": task,
        "context": context,
        "status": "pending",
        "created_at": datetime.now().isoformat(),
        "deadline": deadline,
        "completed_at": None,
        "batch_id": None,
        "tags": list(tags) if tags else [],
    }

    _write_json(INBOX_DIR / f"{task_id}.json", task_data)
    _log("task_created", task_id=task_id, to="antigravity", priority=priority,
         summary=task[:120], deadline=deadline, tags=task_data["tags"])

    return (
        f"Tarea '{task_id}' creada en buzon de Antigravity.\n"
        f"Prioridad: {priority} | Deadline: {deadline}\n"
        f"Verificar luego con: check_antigravity_response('{task_id}')"
    )


@mcp.tool
def check_antigravity_response(task_id: str) -> str:
    """Revisa si Antigravity completo una tarea.

    Busca en responses/ y en claude-inbox/ (respuestas directas).

    Args:
        task_id: ID de la tarea (ej: task_1712882400000)
    """
    # Check responses/ directory
    response_path = RESPONSE_DIR / f"{task_id}_response.json"
    if response_path.exists():
        data = _read_json(response_path)
        _log("response_checked", task_id=task_id, status="completed")
        return (
            f"COMPLETADA — {task_id}\n"
            f"Completada: {data.get('completed_at', '?')}\n"
            f"Resultado:\n{data.get('result', '(sin resultado)')}"
        )

    # Check claude-inbox/ for direct responses
    response_in_inbox = CLAUDE_INBOX_DIR / f"response_{task_id.replace('task_', '')}.json"
    if response_in_inbox.exists():
        data = _read_json(response_in_inbox)
        _log("response_checked", task_id=task_id, status="completed", source="claude-inbox")
        return (
            f"COMPLETADA (via claude-inbox) — {task_id}\n"
            f"Mensaje:\n{data.get('message', data.get('result', '(sin resultado)'))}"
        )

    # Check original task status
    task_path = INBOX_DIR / f"{task_id}.json"
    if task_path.exists():
        task_data = _read_json(task_path)
        status = task_data.get("status", "pending")
        age = _task_age_str(task_data.get("created_at", ""))
        deadline = task_data.get("deadline", "")
        stale = _is_stale(task_data.get("created_at", ""),
                          DEFAULT_TIMEOUT_HOURS) if status == "pending" else False

        stale_warn = " ** STALE — considera retry_task()" if stale else ""
        _log("response_checked", task_id=task_id, status=status, stale=stale)
        return (
            f"PENDIENTE — {task_id} (edad: {age}){stale_warn}\n"
            f"Tarea: {task_data.get('task', '?')[:150]}\n"
            f"Deadline: {deadline}"
        )

    return f"No encontrada — ninguna tarea con ID '{task_id}'"


@mcp.tool
def send_task_to_claude(task: str, context: str = "", priority: str = "medium") -> str:
    """Crea una tarea para Claude Code (usada por Antigravity).

    Args:
        task: Descripcion de la tarea
        context: Contexto adicional
        priority: low | medium | high | critical
    """
    task_id = _make_task_id()

    task_data = {
        "id": task_id,
        "from": "antigravity",
        "to": "claude-code",
        "priority": priority,
        "type": "request",
        "task": task,
        "context": context,
        "status": "pending",
        "created_at": datetime.now().isoformat(),
        "completed_at": None,
    }

    _write_json(CLAUDE_INBOX_DIR / f"{task_id}.json", task_data)
    _log("task_created", task_id=task_id, to="claude-code", priority=priority,
         summary=task[:120])

    return f"Tarea '{task_id}' creada en buzon de Claude Code."


@mcp.tool
def list_pending_tasks(direction: str = "both", tag: str = "") -> str:
    """Lista tareas pendientes con edad y deadline.

    Args:
        direction: 'to_antigravity', 'to_claude', o 'both'
        tag: si se proporciona, filtra solo tareas que tengan ese tag
    """
    results = []

    def _matches_tag(data: dict) -> bool:
        if not tag:
            return True
        return tag in (data.get("tags") or [])

    if direction in ("to_antigravity", "both"):
        for f in sorted(INBOX_DIR.glob("*.json")):
            data = _read_json(f)
            if data.get("status") == "pending" and _matches_tag(data):
                age = _task_age_str(data.get("created_at", ""))
                stale = _is_stale(data.get("created_at", ""))
                stale_mark = " [STALE]" if stale else ""
                batch = f" batch:{data['batch_id']}" if data.get("batch_id") else ""
                tags_str = f" #{','.join(data['tags'])}" if data.get("tags") else ""
                results.append(
                    f"  -> Antigravity [{data['id']}] {data.get('priority', '?')}"
                    f" | {age} old{stale_mark}{batch}{tags_str}\n"
                    f"     {data.get('task', '')[:100]}"
                )

    if direction in ("to_claude", "both"):
        for f in sorted(CLAUDE_INBOX_DIR.glob("task_*.json")):
            data = _read_json(f)
            if data.get("status") == "pending":
                age = _task_age_str(data.get("created_at", ""))
                results.append(
                    f"  -> Claude [{data['id']}] {data.get('priority', '?')}"
                    f" | {age} old\n"
                    f"     {data.get('task', '')[:100]}"
                )

    if not results:
        return "No hay tareas pendientes."

    return f"Tareas pendientes ({len(results)}):\n\n" + "\n\n".join(results)


# ═══════════════════════════════════════════════════════════════════════
#  RESPONSE TOOLS
# ═══════════════════════════════════════════════════════════════════════

@mcp.tool
def get_all_pending_responses() -> str:
    """Lee TODAS las respuestas no procesadas de Antigravity de un golpe.

    Busca en responses/ y claude-inbox/response_*.json.
    Mueve las procesadas a claude-inbox/processed/.
    Devuelve todas las respuestas pendientes en un solo bloque.
    """
    results = []

    # 1. Check responses/ for task responses
    processed_ids = set()
    for f in sorted(RESPONSE_DIR.glob("*_response.json")):
        data = _read_json(f)
        task_id = data.get("id", f.stem.replace("_response", ""))
        # Check if already processed by looking at processed dir
        if (PROCESSED_DIR / f"response_{task_id}.processed").exists():
            continue
        results.append({
            "task_id": task_id,
            "source": "responses/",
            "result": data.get("result", ""),
            "completed_at": data.get("completed_at", "?"),
            "original_task": data.get("original_task", ""),
        })
        # Mark as processed
        (PROCESSED_DIR / f"response_{task_id}.processed").write_text(
            datetime.now().isoformat(), encoding="utf-8")
        processed_ids.add(task_id)

    # 2. Check claude-inbox/ for direct responses
    for f in sorted(CLAUDE_INBOX_DIR.glob("response_*.json")):
        data = _read_json(f)
        task_id = data.get("in_reply_to", data.get("id", f.stem))
        if task_id in processed_ids:
            continue
        if (PROCESSED_DIR / f.name).exists():
            continue
        results.append({
            "task_id": task_id,
            "source": "claude-inbox/",
            "result": data.get("message", data.get("result", "")),
            "completed_at": data.get("created_at", "?"),
            "status": data.get("status", "?"),
        })
        # Move to processed
        f.rename(PROCESSED_DIR / f.name)

    if not results:
        _log("responses_polled", count=0)
        return "No hay respuestas nuevas."

    _log("responses_polled", count=len(results),
         task_ids=[r["task_id"] for r in results])

    lines = [f"=== {len(results)} respuesta(s) nueva(s) ===\n"]
    for r in results:
        lines.append(f"--- {r['task_id']} ({r['source']}) ---")
        if r.get("original_task"):
            lines.append(f"Tarea original: {r['original_task'][:120]}")
        lines.append(f"Completada: {r['completed_at']}")
        lines.append(f"Resultado:\n{r['result']}\n")

    return "\n".join(lines)


@mcp.tool
def validate_deliverables(task_id: str) -> str:
    """Verifica que los archivos mencionados en una respuesta realmente existan.

    Parsea el resultado de una tarea completada, extrae paths de archivos,
    y verifica su existencia en el proyecto.

    Args:
        task_id: ID de la tarea completada
    """
    # Find the response
    result_text = ""

    response_path = RESPONSE_DIR / f"{task_id}_response.json"
    if response_path.exists():
        data = _read_json(response_path)
        result_text = str(data.get("result", ""))

    if not result_text:
        # Check claude-inbox
        for f in CLAUDE_INBOX_DIR.glob(f"response_*{task_id.replace('task_', '')}*"):
            data = _read_json(f)
            result_text = str(data.get("message", data.get("result", "")))
            break

    if not result_text:
        return f"No se encontro respuesta para {task_id}"

    # Extract file paths from result text
    paths = _extract_file_paths(result_text)

    if not paths:
        _log("deliverables_validated", task_id=task_id, found_paths=0)
        return f"No se detectaron paths de archivos en la respuesta de {task_id}."

    # Verify each path
    lines = [f"Validacion de entregables — {task_id}\n"]
    ok_count = 0
    missing_count = 0

    for p in sorted(paths):
        full_path = PROJECT_ROOT / p
        if full_path.exists():
            size = full_path.stat().st_size
            lines.append(f"  OK  {p} ({size:,} bytes)")
            ok_count += 1
        else:
            lines.append(f"  FALTA  {p}")
            missing_count += 1

    lines.append(f"\nResumen: {ok_count} OK, {missing_count} faltantes")

    _log("deliverables_validated", task_id=task_id,
         total=len(paths), ok=ok_count, missing=missing_count)

    return "\n".join(lines)


# ═══════════════════════════════════════════════════════════════════════
#  RELIABILITY TOOLS
# ═══════════════════════════════════════════════════════════════════════

@mcp.tool
def check_stale_tasks(timeout_hours: float = DEFAULT_TIMEOUT_HOURS) -> str:
    """Detecta tareas pendientes que superaron su deadline.

    Args:
        timeout_hours: Horas de timeout (default 2). Tareas con deadline
                       propio usan ese valor.
    """
    stale = []

    for f in sorted(INBOX_DIR.glob("*.json")):
        data = _read_json(f)
        if data.get("status") != "pending":
            continue

        created = data.get("created_at", "")
        deadline = data.get("deadline", "")
        task_timeout = timeout_hours

        # Use task's own deadline if set
        if deadline:
            try:
                dl = datetime.fromisoformat(deadline)
                if datetime.now() > dl:
                    age = _task_age_str(created)
                    stale.append(
                        f"  STALE [{data['id']}] {data.get('priority', '?')} | {age} old\n"
                        f"        Deadline: {deadline}\n"
                        f"        {data.get('task', '')[:100]}"
                    )
                continue
            except Exception:
                pass

        if _is_stale(created, task_timeout):
            age = _task_age_str(created)
            stale.append(
                f"  STALE [{data['id']}] {data.get('priority', '?')} | {age} old\n"
                f"        {data.get('task', '')[:100]}"
            )

    _log("stale_check", count=len(stale))

    if not stale:
        return "No hay tareas stale. Todo al dia."

    return f"{len(stale)} tarea(s) stale:\n\n" + "\n\n".join(stale)


@mcp.tool
def retry_task(task_id: str, updated_context: str = "") -> str:
    """Re-envia una tarea fallida o expirada con un nuevo ID.

    Copia la tarea original, le da un nuevo ID, y la marca como pendiente.
    Opcionalmente agrega contexto adicional.

    Args:
        task_id: ID de la tarea original a reintentar
        updated_context: Contexto adicional para el reintento
    """
    # Find original task
    task_path = INBOX_DIR / f"{task_id}.json"
    if not task_path.exists():
        return f"Tarea '{task_id}' no encontrada."

    original = _read_json(task_path)
    new_id = _make_task_id()

    # Build new context
    context = original.get("context", "")
    if updated_context:
        context = f"{context}\n\n[RETRY de {task_id}] {updated_context}"
    else:
        context = f"{context}\n\n[RETRY de {task_id}]"

    new_task = {
        "id": new_id,
        "from": original.get("from", "claude-code"),
        "to": original.get("to", "antigravity"),
        "priority": original.get("priority", "medium"),
        "type": "request",
        "task": original.get("task", ""),
        "context": context,
        "status": "pending",
        "created_at": datetime.now().isoformat(),
        "deadline": (datetime.now() + timedelta(hours=DEFAULT_TIMEOUT_HOURS)).isoformat(),
        "completed_at": None,
        "batch_id": original.get("batch_id"),
        "retry_of": task_id,
    }

    # Mark original as retried
    original["status"] = "retried"
    original["retried_as"] = new_id
    _write_json(task_path, original)

    # Write new task
    _write_json(INBOX_DIR / f"{new_id}.json", new_task)

    _log("task_retried", original_id=task_id, new_id=new_id)

    return (
        f"Tarea re-enviada como '{new_id}' (retry de {task_id}).\n"
        f"Verificar luego con: check_antigravity_response('{new_id}')"
    )


# ═══════════════════════════════════════════════════════════════════════
#  CONTEXT TOOLS
# ═══════════════════════════════════════════════════════════════════════

VALID_TOPICS = [
    "general",        # Analisis general, notas miscelaneas
    "build_status",   # Estado del build, errores, warnings
    "sprite_status",  # Sprites listos, faltantes, problemas
    "decisions",      # Decisiones de diseno
    "blockers",       # Que bloquea a cada agente
]

@mcp.tool
def update_context(topic: str, content: str, append: bool = False) -> str:
    """Escribe contexto compartido por topico.

    Args:
        topic: Topico (general, build_status, sprite_status, decisions, blockers)
        content: Contenido markdown
        append: Si True, agrega al final. Si False, reemplaza.
    """
    if topic not in VALID_TOPICS:
        return f"Topico invalido '{topic}'. Validos: {', '.join(VALID_TOPICS)}"

    ctx_path = SHARED_CONTEXT_DIR / f"{topic}.md"

    if append and ctx_path.exists():
        existing = ctx_path.read_text(encoding="utf-8")
        # Remove old header
        if existing.startswith("<!--"):
            existing = existing.split("-->\n", 1)[-1]
        content = existing.rstrip() + "\n\n---\n\n" + content

    header = f"<!-- topic:{topic} | updated:{datetime.now().isoformat()} | by:claude-code -->\n\n"
    ctx_path.write_text(header + content, encoding="utf-8")

    _log("context_updated", topic=topic, chars=len(content), append=append)

    return f"Contexto '{topic}' actualizado ({len(content)} chars)"


@mcp.tool
def read_context(topic: str = "all") -> str:
    """Lee contexto compartido por topico o todos.

    Args:
        topic: Topico especifico o 'all' para leer todos
    """
    if topic == "all":
        results = []
        for t in VALID_TOPICS:
            path = SHARED_CONTEXT_DIR / f"{t}.md"
            if path.exists():
                content = path.read_text(encoding="utf-8")
                results.append(f"=== {t} ===\n{content}")
        if not results:
            return "No hay contexto compartido."
        return "\n\n".join(results)

    if topic not in VALID_TOPICS:
        return f"Topico invalido '{topic}'. Validos: {', '.join(VALID_TOPICS)} o 'all'"

    path = SHARED_CONTEXT_DIR / f"{topic}.md"
    if path.exists():
        return path.read_text(encoding="utf-8")

    return f"No hay contexto para topico '{topic}'."


# Legacy aliases
@mcp.tool
def update_shared_context(content: str, append: bool = False) -> str:
    """[Legacy] Alias de update_context(topic='general'). Usa update_context() para topicos."""
    return update_context("general", content, append)


@mcp.tool
def read_shared_context() -> str:
    """[Legacy] Alias de read_context(topic='general'). Usa read_context() para topicos."""
    return read_context("general")


# ═══════════════════════════════════════════════════════════════════════
#  AUTOMATION TOOLS
# ═══════════════════════════════════════════════════════════════════════

@mcp.tool
def request_from_template(template_name: str, params: str) -> str:
    """Crea una tarea desde un template predefinido.

    Templates disponibles: sprite_request, palette_check, visual_audit, asset_generation.
    Pasa los parametros como JSON string.

    Args:
        template_name: Nombre del template
        params: JSON string con los parametros del template
    """
    template_path = TEMPLATE_DIR / f"{template_name}.json"
    if not template_path.exists():
        available = [f.stem for f in TEMPLATE_DIR.glob("*.json")]
        return f"Template '{template_name}' no existe. Disponibles: {', '.join(available) or '(ninguno)'}"

    template = _read_json(template_path)

    try:
        params_dict = json.loads(params) if isinstance(params, str) else params
    except json.JSONDecodeError as e:
        return f"Error parseando params JSON: {e}"

    # Fill template placeholders
    task_text = template.get("task_template", "")
    context_text = template.get("context_template", "")
    priority = template.get("default_priority", "medium")

    for key, value in params_dict.items():
        placeholder = f"{{{{{key}}}}}"  # {{key}}
        task_text = task_text.replace(placeholder, str(value))
        context_text = context_text.replace(placeholder, str(value))

    # Check for unfilled placeholders
    unfilled = re.findall(r'\{\{(\w+)\}\}', task_text + context_text)
    if unfilled:
        return f"Faltan parametros requeridos: {', '.join(set(unfilled))}"

    return request_antigravity(task=task_text, context=context_text, priority=priority)


@mcp.tool
def request_batch(tasks_json: str, batch_name: str = "") -> str:
    """Envia un grupo de tareas relacionadas como batch.

    Args:
        tasks_json: JSON array de tareas. Cada una: {task, context?, priority?}
        batch_name: Nombre descriptivo del batch
    """
    try:
        tasks = json.loads(tasks_json)
    except json.JSONDecodeError as e:
        return f"Error parseando JSON: {e}"

    if not isinstance(tasks, list) or len(tasks) == 0:
        return "Se requiere un array JSON no vacio de tareas."

    batch_id = f"batch_{int(time.time() * 1000)}"
    task_ids = []

    for i, t in enumerate(tasks):
        task_id = _make_task_id()
        # Slight delay to ensure unique IDs
        time.sleep(0.002)

        task_data = {
            "id": task_id,
            "from": "claude-code",
            "to": "antigravity",
            "priority": t.get("priority", "medium"),
            "type": "request",
            "task": t.get("task", ""),
            "context": t.get("context", ""),
            "status": "pending",
            "created_at": datetime.now().isoformat(),
            "deadline": (datetime.now() + timedelta(hours=DEFAULT_TIMEOUT_HOURS)).isoformat(),
            "completed_at": None,
            "batch_id": batch_id,
            "batch_index": i,
            "batch_total": len(tasks),
        }

        _write_json(INBOX_DIR / f"{task_id}.json", task_data)
        task_ids.append(task_id)

    _log("batch_created", batch_id=batch_id, batch_name=batch_name,
         count=len(tasks), task_ids=task_ids)

    lines = [f"Batch '{batch_id}' creado con {len(tasks)} tarea(s).\n"]
    for tid in task_ids:
        lines.append(f"  - {tid}")
    lines.append(f"\nVerificar con: check_batch('{batch_id}')")

    return "\n".join(lines)


@mcp.tool
def check_batch(batch_id: str) -> str:
    """Verifica el estado de todas las tareas de un batch.

    Args:
        batch_id: ID del batch (ej: batch_1712882400000)
    """
    tasks = []

    for f in sorted(INBOX_DIR.glob("*.json")):
        data = _read_json(f)
        if data.get("batch_id") == batch_id:
            # Check if response exists
            response_path = RESPONSE_DIR / f"{data['id']}_response.json"
            has_response = response_path.exists()

            tasks.append({
                "id": data["id"],
                "index": data.get("batch_index", "?"),
                "status": "completed" if has_response else data.get("status", "?"),
                "task": data.get("task", "")[:80],
            })

    if not tasks:
        return f"No se encontraron tareas del batch '{batch_id}'."

    tasks.sort(key=lambda t: t.get("index", 0))

    completed = sum(1 for t in tasks if t["status"] == "completed")
    total = len(tasks)

    lines = [f"Batch {batch_id}: {completed}/{total} completadas\n"]
    for t in tasks:
        mark = "[x]" if t["status"] == "completed" else "[ ]"
        lines.append(f"  {mark} {t['id']} — {t['task']}")

    _log("batch_checked", batch_id=batch_id, completed=completed, total=total)

    return "\n".join(lines)


# ═══════════════════════════════════════════════════════════════════════
#  MONITORING TOOLS
# ═══════════════════════════════════════════════════════════════════════

@mcp.tool
def bridge_dashboard(format: str = "text") -> str:
    """Resumen completo del estado del bridge.

    Muestra: tareas pendientes, stale, completadas hoy, ultimo contexto,
    respuestas sin procesar, y salud general del bridge.

    Args:
        format: 'text' (default, human-readable) o 'json' (estructurado para scripts)
    """
    now = datetime.now()
    today = now.date().isoformat()

    # Count tasks by status
    pending_ag = 0
    pending_cl = 0
    stale_count = 0
    completed_today = 0
    total_tasks = 0

    for f in INBOX_DIR.glob("*.json"):
        data = _read_json(f)
        total_tasks += 1
        status = data.get("status", "pending")

        if status == "pending":
            pending_ag += 1
            if _is_stale(data.get("created_at", "")):
                stale_count += 1
        elif status == "completed":
            ca = data.get("completed_at", "")
            if ca and ca.startswith(today):
                completed_today += 1

    for f in CLAUDE_INBOX_DIR.glob("task_*.json"):
        data = _read_json(f)
        if data.get("status") == "pending":
            pending_cl += 1

    # Count unprocessed responses
    unprocessed_responses = 0
    for f in RESPONSE_DIR.glob("*_response.json"):
        data = _read_json(f)
        task_id = data.get("id", "")
        if not (PROCESSED_DIR / f"response_{task_id}.processed").exists():
            unprocessed_responses += 1

    for f in CLAUDE_INBOX_DIR.glob("response_*.json"):
        if not (PROCESSED_DIR / f.name).exists():
            unprocessed_responses += 1

    # Latest context updates
    context_info = []
    for t in VALID_TOPICS:
        path = SHARED_CONTEXT_DIR / f"{t}.md"
        if path.exists():
            mtime = datetime.fromtimestamp(path.stat().st_mtime)
            age = _task_age_str(mtime.isoformat())
            context_info.append(f"  {t}: {age} ago")

    # Log stats
    log_lines = 0
    if LOG_FILE.exists():
        log_lines = sum(1 for _ in open(LOG_FILE, encoding="utf-8"))

    _log("dashboard_viewed", format=format)

    if format == "json":
        ctx_ages = {}
        for t in VALID_TOPICS:
            path = SHARED_CONTEXT_DIR / f"{t}.md"
            if path.exists():
                ctx_ages[t] = datetime.fromtimestamp(path.stat().st_mtime).isoformat()
        return json.dumps({
            "pending_to_antigravity": pending_ag,
            "stale_count": stale_count,
            "pending_to_claude": pending_cl,
            "completed_today": completed_today,
            "total_historic": total_tasks,
            "unprocessed_responses": unprocessed_responses,
            "log_events": log_lines,
            "context_topics_updated_at": ctx_ages,
            "generated_at": now.isoformat(),
        }, ensure_ascii=False, indent=2)

    lines = [
        "=== AI BRIDGE DASHBOARD ===\n",
        f"Tareas pendientes -> Antigravity: {pending_ag}"
        + (f" ({stale_count} STALE)" if stale_count else ""),
        f"Tareas pendientes -> Claude: {pending_cl}",
        f"Completadas hoy: {completed_today}",
        f"Total historico: {total_tasks}",
        f"Respuestas sin procesar: {unprocessed_responses}",
        "",
        "Contexto compartido:",
    ]

    if context_info:
        lines.extend(context_info)
    else:
        lines.append("  (vacio)")

    lines.extend(["", f"Log: {log_lines} eventos registrados"])

    return "\n".join(lines)


@mcp.tool
def bridge_log(last_n: int = 20) -> str:
    """Lee los ultimos N eventos del log del bridge.

    Args:
        last_n: Numero de eventos a mostrar (default 20)
    """
    if not LOG_FILE.exists():
        return "Log vacio — no se han registrado eventos aun."

    with open(LOG_FILE, "r", encoding="utf-8") as f:
        all_lines = f.readlines()

    if not all_lines:
        return "Log vacio."

    entries = all_lines[-last_n:]
    lines = [f"=== Ultimos {min(last_n, len(entries))} eventos (de {len(all_lines)} total) ===\n"]

    for entry_str in entries:
        try:
            entry = json.loads(entry_str)
            ts = entry.get("timestamp", "?")
            # Shorten timestamp to HH:MM:SS
            if "T" in ts:
                ts = ts.split("T")[1][:8]
            event = entry.get("event", "?")
            # Build summary from remaining fields
            extra = {k: v for k, v in entry.items()
                     if k not in ("timestamp", "event")}
            extra_str = " | ".join(f"{k}={v}" for k, v in extra.items())
            lines.append(f"  [{ts}] {event}" + (f" — {extra_str}" if extra_str else ""))
        except json.JSONDecodeError:
            lines.append(f"  (malformed entry)")

    return "\n".join(lines)


@mcp.tool
def bridge_stats() -> str:
    """Estadisticas de uso del bridge: tiempos de respuesta, tasas de exito, volumen."""
    total = 0
    completed = 0
    failed = 0
    retried = 0
    pending = 0
    response_times = []
    by_priority = {"low": 0, "medium": 0, "high": 0, "critical": 0}

    for f in INBOX_DIR.glob("*.json"):
        data = _read_json(f)
        total += 1
        status = data.get("status", "pending")
        priority = data.get("priority", "medium")
        by_priority[priority] = by_priority.get(priority, 0) + 1

        if status == "completed":
            completed += 1
            # Calculate response time
            created = data.get("created_at", "")
            finished = data.get("completed_at", "")
            if created and finished:
                try:
                    c = datetime.fromisoformat(created)
                    f_dt = datetime.fromisoformat(finished)
                    delta = (f_dt - c).total_seconds() / 60  # minutes
                    response_times.append(delta)
                except Exception:
                    pass
        elif status == "failed":
            failed += 1
        elif status == "retried":
            retried += 1
        elif status == "pending":
            pending += 1

    lines = [
        "=== BRIDGE STATS ===\n",
        f"Total tareas: {total}",
        f"Completadas: {completed} ({100*completed//max(total,1)}%)",
        f"Pendientes: {pending}",
        f"Fallidas: {failed}",
        f"Reintentadas: {retried}",
        "",
        "Por prioridad:",
    ]
    for p, c in sorted(by_priority.items()):
        if c > 0:
            lines.append(f"  {p}: {c}")

    if response_times:
        avg = sum(response_times) / len(response_times)
        fastest = min(response_times)
        slowest = max(response_times)
        lines.extend([
            "",
            "Tiempos de respuesta:",
            f"  Promedio: {avg:.1f} min",
            f"  Mas rapido: {fastest:.1f} min",
            f"  Mas lento: {slowest:.1f} min",
        ])
    else:
        lines.extend(["", "Tiempos de respuesta: sin datos"])

    return "\n".join(lines)


@mcp.tool
def archive_completed(older_than_hours: float = 24.0) -> str:
    """Archiva tareas completadas mas antiguas que N horas.

    Mueve tareas y respuestas a archive/YYYY-MM/ para mantener los
    inboxes limpios.

    Args:
        older_than_hours: Solo archivar tareas mas viejas que esto (default 24)
    """
    archived = 0
    cutoff = datetime.now() - timedelta(hours=older_than_hours)
    month_dir = ARCHIVE_DIR / cutoff.strftime("%Y-%m")
    month_dir.mkdir(parents=True, exist_ok=True)

    for f in list(INBOX_DIR.glob("*.json")):
        data = _read_json(f)
        status = data.get("status", "")
        if status not in ("completed", "failed", "retried"):
            continue

        completed_at = data.get("completed_at", data.get("created_at", ""))
        if not completed_at:
            continue

        try:
            ct = datetime.fromisoformat(completed_at)
            if ct > cutoff:
                continue
        except Exception:
            continue

        # Move task file
        f.rename(month_dir / f.name)

        # Move response if exists
        response_path = RESPONSE_DIR / f"{data['id']}_response.json"
        if response_path.exists():
            response_path.rename(month_dir / response_path.name)

        archived += 1

    _log("archive_run", archived=archived, older_than_hours=older_than_hours)

    if archived == 0:
        return "Nada que archivar."

    return f"Archivadas {archived} tarea(s) en {month_dir}"


@mcp.tool
def bridge_search(query: str, since_hours: float = 168.0, limit: int = 50) -> str:
    """Busca eventos en bridge_log.jsonl por keyword + ventana temporal.

    Args:
        query: substring case-insensitive a buscar en cualquier campo del evento
        since_hours: solo eventos ultimos N horas (default 168 = 1 semana)
        limit: maximo de matches a retornar (default 50)
    """
    if not LOG_FILE.exists():
        return "Log vacio."
    cutoff = datetime.now() - timedelta(hours=since_hours)
    q = query.lower()
    matches = []
    try:
        with open(LOG_FILE, "r", encoding="utf-8") as f:
            for line in f:
                try:
                    e = json.loads(line)
                except Exception:
                    continue
                ts = e.get("timestamp", "")
                try:
                    if datetime.fromisoformat(ts) < cutoff:
                        continue
                except Exception:
                    pass
                # Search across all fields as JSON dump (cheap, robust)
                blob = json.dumps(e, ensure_ascii=False).lower()
                if q in blob:
                    matches.append(e)
                    if len(matches) >= limit:
                        break
    except Exception as ex:
        return f"Error leyendo log: {ex}"

    _log("bridge_search", query=query[:80], matches=len(matches))

    if not matches:
        return f"Sin matches para '{query}' en ultimas {since_hours}h."
    out = [f"=== {len(matches)} match(es) para '{query}' (ultimas {since_hours}h) ==="]
    for e in matches:
        ts = e.get("timestamp", "?")
        ev = e.get("event", "?")
        extras = {k: v for k, v in e.items() if k not in ("timestamp", "event")}
        out.append(f"  [{ts}] {ev} — {json.dumps(extras, ensure_ascii=False)[:200]}")
    return "\n".join(out)


@mcp.tool
def bridge_health() -> str:
    """Estado de salud del bridge: ultimo response de cada agente, tareas
    huerfanas, log size. Util para detectar agentes silentes/dormidos."""
    now = datetime.now()
    last_event_by_agent = {"antigravity": None, "claude-code": None}
    last_event_global = None
    if LOG_FILE.exists():
        try:
            with open(LOG_FILE, "r", encoding="utf-8") as f:
                for line in f:
                    try:
                        e = json.loads(line)
                    except Exception:
                        continue
                    ts = e.get("timestamp")
                    if ts:
                        last_event_global = ts
                    # Heuristics: task_completed.by, response_checked, task_created.to
                    by = e.get("by")
                    to = e.get("to")
                    if by in last_event_by_agent and ts:
                        last_event_by_agent[by] = ts
                    if to in last_event_by_agent and ts:
                        last_event_by_agent[to] = ts
        except Exception:
            pass

    def _silence(ts):
        if not ts:
            return "never"
        try:
            return _task_age_str(ts)
        except Exception:
            return "?"

    log_size_bytes = LOG_FILE.stat().st_size if LOG_FILE.exists() else 0
    log_size_kb = log_size_bytes // 1024

    # Orphans: tasks with status=pending and no deadline / very old
    orphans = 0
    for f in INBOX_DIR.glob("*.json"):
        try:
            d = _read_json(f)
        except Exception:
            continue
        if d.get("status") != "pending":
            continue
        if not d.get("deadline"):
            orphans += 1

    lines = [
        "=== BRIDGE HEALTH ===",
        f"Last activity (global):       {_silence(last_event_global)} ago",
        f"Last from Antigravity:        {_silence(last_event_by_agent['antigravity'])} ago",
        f"Last from Claude:             {_silence(last_event_by_agent['claude-code'])} ago",
        f"Log size:                     {log_size_kb} KB"
        + (" (will rotate at 1MB)" if log_size_bytes > LOG_ROTATE_BYTES * 0.8 else ""),
        f"Orphan tasks (no deadline):   {orphans}",
        f"Generated at:                 {now.isoformat()}",
    ]
    _log("bridge_health_check")
    return "\n".join(lines)


@mcp.tool
def snapshot_plans(label: str = "") -> str:
    """Snapshotea los planes del proyecto a archive/plans/<date>/.

    Captura: joint_plan.md, claude_analysis.md, BRIDGE_V3_PLAN.md, MASTER_PLAN.md
    cuando existen. Util antes de cambios mayores para tener historia auditable.

    Args:
        label: etiqueta opcional para el snapshot (ej. 'pre-v3', 'before-migration')
    """
    import shutil
    candidates = [
        PROJECT_ROOT / "joint_plan.md",
        PROJECT_ROOT / "claude_analysis.md",
        BRIDGE_DIR / "BRIDGE_V3_PLAN.md",
        PROJECT_ROOT / "docs" / "MASTER_PLAN.md",
    ]
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    folder_name = f"{ts}_{label}" if label else ts
    snap_dir = ARCHIVE_DIR / "plans" / folder_name
    snap_dir.mkdir(parents=True, exist_ok=True)

    saved = []
    for path in candidates:
        if path.exists() and path.is_file():
            dest = snap_dir / path.name
            shutil.copy2(str(path), str(dest))
            saved.append(path.name)

    if not saved:
        snap_dir.rmdir()  # nothing to keep
        return "Sin planes que snapshotear."

    _log("plans_snapshotted", label=label, files=saved, dir=str(snap_dir))
    return f"Snapshot guardado en {snap_dir}:\n  " + "\n  ".join(saved)


@mcp.tool
def prune_inbox() -> str:
    """Higiene manual: archiva tareas completadas viejas y expira pendientes.

    Llamada explicita para limpieza on-demand. El servidor tambien lo hace
    automaticamente al startup y antes de crear nuevas tareas, asi que en
    operacion normal no necesitas invocarlo. Util para quotas/auditorias.

    Acciones:
      - Archiva tareas completed/failed/retried con +24h de antigüedad
      - Mueve tareas pending stale +7 dias a archive/expired/
      - Rota bridge_log.jsonl si supera 1MB
    """
    _rotate_log_if_needed()
    stats = _auto_maintenance()
    _log("prune_inbox_manual", **stats)
    if stats["archived"] == 0 and stats["expired"] == 0:
        return "Inbox limpio — nada que prunear."
    return (
        f"Prune completado.\n"
        f"  Archivadas: {stats['archived']} tarea(s)\n"
        f"  Expiradas:  {stats['expired']} tarea(s)"
    )


# ═══════════════════════════════════════════════════════════════════════

if __name__ == "__main__":
    mcp.run()
