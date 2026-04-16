# 🔗 AI Bridge — Guía Completa del Sistema

> **Versión:** 1.0.0  
> **Fecha:** 12 de Abril 2026  
> **Proyecto:** INFERNUS (Roguesouls-like)  
> **Autor:** Implementado por Antigravity (Gemini) con Juan Miguel

---

## Tabla de Contenidos

1. [¿Qué es el AI Bridge?](#qué-es-el-ai-bridge)
2. [¿Por qué existe?](#por-qué-existe)
3. [Arquitectura del sistema](#arquitectura-del-sistema)
4. [Componentes](#componentes)
   - [Capa 1: CLI Directo](#capa-1-cli-directo)
   - [Capa 2: Protocolo de Archivos](#capa-2-protocolo-de-archivos)
   - [Capa 3: Servidor MCP](#capa-3-servidor-mcp)
   - [Capa 4: Extensión VS Code](#capa-4-extensión-vs-code)
5. [Brain Wiring: Instrucciones de cada IA](#brain-wiring)
6. [Flujos de trabajo](#flujos-de-trabajo)
7. [Estructura de archivos](#estructura-de-archivos)
8. [Setup e instalación](#setup-e-instalación)
9. [Uso diario](#uso-diario)
10. [Troubleshooting](#troubleshooting)
11. [Limitaciones conocidas](#limitaciones-conocidas)
12. [Ideas futuras](#ideas-futuras)

---

## ¿Qué es el AI Bridge?

El AI Bridge es un **sistema de comunicación bidireccional entre dos inteligencias artificiales** que trabajan en el mismo proyecto de software:

- **Antigravity (Gemini de Google)** — una IA que corre como extensión de VS Code. Puede generar imágenes, navegar web, buscar información, editar archivos, y controlar un browser.
- **Claude Code (Anthropic)** — una IA especializada en razonamiento profundo, refactoring de código, y autonomía en la terminal.

El bridge permite que estas dos IAs:
1. **Se envíen tareas** la una a la otra
2. **Compartan contexto** sobre el estado del proyecto
3. **Verifiquen resultados** de tareas completadas
4. **Trabajen en paralelo** sin que el usuario sea el mensajero

Antes del bridge, el usuario (tú) tenías que copiar información manualmente entre ambas IAs. Ahora, el sistema lo hace automáticamente.

---

## ¿Por qué existe?

### El problema

Cada IA tiene capacidades únicas que la otra no tiene:

| Capacidad | Antigravity | Claude Code |
|-----------|:-----------:|:-----------:|
| Generar imágenes/sprites | ✅ | ❌ |
| Navegar web interactivamente | ✅ | ❌ |
| Búsqueda web | ✅ | ❌ |
| Análisis visual | ✅ | ❌ |
| Razonamiento profundo de código | ⚠️ | ✅ |
| Refactoring agresivo | ⚠️ | ✅ |
| Autonomía en terminal | ⚠️ | ✅ |
| Edición multi-archivo | ✅ | ✅ |
| Ejecución de comandos | ✅ | ✅ |
| Lectura/escritura archivos | ✅ | ✅ |

Sin el bridge, cuando Claude necesitaba un sprite, el flujo era:
1. Claude escribe `claude_to_antigravity.md`
2. El usuario hace commit
3. El usuario le dice a Antigravity que lea el archivo
4. Antigravity genera el sprite
5. Antigravity escribe `antigravity_to_claude.md`
6. El usuario hace commit
7. El usuario le dice a Claude que lea el archivo

**Con el bridge**, el flujo es:
1. Claude llama `request_antigravity("genera un sprite...")`
2. Antigravity recibe la tarea **automáticamente**
3. Antigravity genera el sprite
4. Antigravity marca la tarea como completada
5. Claude verifica con `check_antigravity_response()`

**De 7 pasos manuales a 0 pasos manuales** (el usuario solo observa).

---

## Arquitectura del sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                         VS Code                                  │
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────┐   │
│  │  Claude Code  │    │  Antigravity │    │  Bridge Watcher   │   │
│  │  (Extension)  │    │  (Extension) │    │  (Extension)      │   │
│  └──────┬───────┘    └──────▲───────┘    └────────┬─────────┘   │
│         │                    │                      │             │
│         │ MCP Protocol       │ Chat Message          │ File Watch  │
│         ▼                    │                      │             │
│  ┌──────────────┐           │              ┌───────▼─────────┐  │
│  │  MCP Server   │           │              │ .ai-bridge/     │  │
│  │  (Python)     │──writes──▶│◀──watches─── │ antigravity-    │  │
│  │              │           │              │ inbox/*.json    │  │
│  └──────────────┘           │              └─────────────────┘  │
│                              │                                    │
│         ┌────────────────────┘                                    │
│         │  buildAntigravityMessage(task)                          │
│         │  → workbench.action.chat.open                          │
│         │  → isPartialQuery: false (auto-send)                   │
│         │                                                        │
└─────────┼────────────────────────────────────────────────────────┘
          │
          ▼
    Antigravity procesa la tarea
    y ejecuta process_inbox.py complete
```

### Flujo de datos paso a paso

```
1. Claude Code
   └─→ Llama request_antigravity() via MCP
       └─→ MCP Server (antigravity_mcp_server.py)
           └─→ Escribe JSON en .ai-bridge/antigravity-inbox/task_XXXX.json
               └─→ VS Code FileSystemWatcher detecta el nuevo archivo
                   └─→ bridge-watcher extension lee el JSON
                       └─→ Muestra notificación en VS Code
                           └─→ Ejecuta workbench.action.chat.open con el mensaje
                               └─→ Antigravity recibe el mensaje en su chat
                                   └─→ Antigravity procesa la tarea
                                       └─→ Ejecuta process_inbox.py complete
                                           └─→ Escribe respuesta en .ai-bridge/responses/
                                               └─→ Claude verifica con check_antigravity_response()
```

---

## Componentes

### Capa 1: CLI Directo

**Dirección:** Antigravity → Claude Code  
**Método:** Invocar Claude Code headless desde terminal  
**Estado:** ✅ Funcional

Antigravity puede ejecutar Claude Code directamente como un subproceso:

```bash
npx -y @anthropic-ai/claude-code -p "tu tarea aquí" --output-format text --max-turns 10
```

Esto permite a Antigravity:
- Delegar tareas de razonamiento profundo a Claude
- Pedir análisis de código
- Solicitar refactoring
- Obtener respuestas de Claude sin intervención del usuario

**Ejemplo real probado:**
```
Antigravity ejecutó: 
  npx -y @anthropic-ai/claude-code -p "Lee config.json y confirma que lo puedes ver"

Claude respondió:
  "OK — Define el protocolo de comunicación bidireccional entre Claude Code 
   y Antigravity (Gemini): inboxes, formato de mensajes JSON, contexto 
   compartido, y métodos de invocación de cada agente."
```

**Flags útiles:**
- `--output-format json` — respuesta estructurada
- `--output-format text` — texto plano
- `--max-turns N` — limitar iteraciones (seguridad)
- `--allowedTools "Read,Write,Bash"` — pre-aprobar herramientas

---

### Capa 2: Protocolo de Archivos

**Dirección:** Bidireccional  
**Método:** Carpetas de buzón con archivos JSON  
**Estado:** ✅ Funcional

Estructura de buzones:
```
.ai-bridge/
├── antigravity-inbox/   ← Tareas para Antigravity (de Claude)
├── claude-inbox/        ← Tareas para Claude (de Antigravity)
├── responses/           ← Respuestas completadas
└── shared-context/      ← Contexto compartido entre ambas IAs
    └── current_analysis.md
```

**Formato de mensaje (JSON):**
```json
{
  "id": "task_1712882400000",
  "from": "claude-code",
  "to": "antigravity",
  "priority": "medium",
  "type": "request",
  "task": "Genera un spritesheet de 64x64 para el boss Infernal Guardian",
  "context": "Estilo Dark Souls pixel art, 8 frames de animación, transparente",
  "status": "pending",
  "created_at": "2026-04-12T00:30:00.000",
  "completed_at": null
}
```

**Campos:**
| Campo | Tipo | Descripción |
|-------|------|-------------|
| `id` | string | ID único basado en timestamp en milisegundos |
| `from` | string | `"claude-code"` o `"antigravity"` |
| `to` | string | `"claude-code"` o `"antigravity"` |
| `priority` | string | `"low"`, `"medium"`, `"high"`, `"critical"` |
| `type` | string | `"request"`, `"response"`, `"context_update"` |
| `task` | string | Descripción detallada de la tarea |
| `context` | string | Contexto adicional |
| `status` | string | `"pending"`, `"processing"`, `"completed"`, `"failed"` |
| `created_at` | string | ISO 8601 timestamp de creación |
| `completed_at` | string/null | ISO 8601 timestamp de completado |

**Formato de respuesta (JSON):**
```json
{
  "id": "task_1712882400000",
  "from": "antigravity",
  "to": "claude-code",
  "type": "response",
  "original_task": "Genera un spritesheet...",
  "result": "Spritesheet generado en assets/sprites/bosses/infernal_guardian.png",
  "status": "completed",
  "created_at": "2026-04-12T00:30:00.000",
  "completed_at": "2026-04-12T00:35:12.000"
}
```

---

### Capa 3: Servidor MCP

**Dirección:** Claude Code → Antigravity  
**Método:** Model Context Protocol (MCP) via FastMCP  
**Estado:** ✅ Conectado y funcional

**¿Qué es MCP?**  
MCP (Model Context Protocol) es un estándar abierto para conectar herramientas externas a IAs. Es como un USB para IAs — enchufas una herramienta y la IA puede usarla como si fuera nativa.

**Nuestro servidor:** `antigravity_mcp_server.py`  
**Framework:** FastMCP v3.2.3  
**Transporte:** stdio (el servidor se ejecuta como subproceso de Claude Code)

**Herramientas registradas:**

| Herramienta | Firma | Propósito |
|------------|-------|-----------|
| `request_antigravity` | `(task, context?, priority?)` | Enviar tarea a Antigravity |
| `check_antigravity_response` | `(task_id)` | Verificar si una tarea fue completada |
| `list_pending_tasks` | `(direction?)` | Listar todas las tareas pendientes |
| `update_shared_context` | `(content, append?)` | Escribir contexto compartido |
| `read_shared_context` | `()` | Leer contexto compartido |
| `send_task_to_claude` | `(task, context?, priority?)` | Crear tarea para Claude (usada por Antigravity) |

**Registro en Claude Code:**
```bash
claude mcp add antigravity-bridge -- python .ai-bridge/antigravity_mcp_server.py
```

**Verificación:**
```bash
claude mcp list
# → antigravity-bridge: ✓ Connected
```

**Cómo funciona internamente:**
1. Claude Code inicia el servidor MCP como subproceso al arrancar
2. El servidor expone las herramientas via JSON-RPC sobre stdio
3. Cuando Claude llama una herramienta, el servidor:
   - Para `request_antigravity`: crea un JSON en `antigravity-inbox/`
   - Para `check_antigravity_response`: lee de `responses/`
   - Para shared context: lee/escribe `shared-context/current_analysis.md`
4. El servidor devuelve el resultado a Claude

---

### Capa 4: Extensión VS Code (File Watcher)

**Dirección:** Sistema → Antigravity  
**Método:** VS Code Extension con FileSystemWatcher  
**Estado:** ✅ Instalada

**¿Qué problema resuelve?**  
El eslabón más débil del sistema: ¿cómo le llega a Antigravity una tarea si nadie le avisa? Antigravity solo "existe" cuando el usuario le escribe en el chat. No tiene un listener externo.

**Solución:** Una extensión de VS Code que vigila la carpeta `antigravity-inbox/`. Cuando aparece un archivo nuevo, la extensión:

1. Lee el JSON de la tarea
2. Muestra una **notificación** en VS Code con el resumen
3. El usuario hace click en "Enviar a Antigravity"
4. La extensión ejecuta `workbench.action.chat.open` con `isPartialQuery: false`
5. Esto **abre el chat de Antigravity y envía el mensaje automáticamente**

**Ubicación:** `~/.vscode/extensions/ai-bridge-watcher/`

**Archivos:**
```
ai-bridge-watcher/
├── package.json     # Manifiesto de la extensión (nombre, activación, comandos)
└── extension.js     # Lógica: file watcher, notificaciones, auto-send
```

**Activación:**  
La extensión se activa automáticamente en cualquier workspace que contenga `.ai-bridge/config.json` (via `activationEvents: ["workspaceContains:.ai-bridge/config.json"]`).

**Status Bar:**  
Muestra un indicador permanente en la barra inferior de VS Code:
- `🔗 AI Bridge` — activo, sin tareas pendientes
- `🔔 AI Bridge: N tarea(s)` — tareas pendientes (fondo amarillo de warning)
- `⟳ AI Bridge: procesando...` — Antigravity está trabajando

**Comandos registrados:**
- `AI Bridge: Check Inbox for Pending Tasks` — escanea manualmente el buzón
- `AI Bridge: Show Bridge Status` — muestra estadísticas del bridge

**Prioridades:**  
Las tareas `critical` muestran un **modal bloqueante** — toda la UI se para hasta que el usuario lo atienda. Las demás muestran notificaciones normales.

**Fallback triple:**  
Si `workbench.action.chat.open` falla:
1. Intenta con `isPartialQuery: true` (el usuario presiona Enter)
2. Si eso falla, copia al portapapeles y avisa al usuario

---

## Brain Wiring

### ¿Qué es brain wiring?

Ambas IAs necesitan **instrucciones predefinidas** sobre cómo usar el bridge. Sin estas instrucciones:
- Claude no sabría que puede llamar `request_antigravity()`
- Antigravity no sabría cómo procesar una tarea del bridge
- Ninguno sabría qué delegar y qué hacer por sí mismo

Las instrucciones están en:
- `docs/CLAUDE_PROTOCOL.md` — protocolo obligatorio para Claude Code
- `docs/ANTIGRAVITY_PROTOCOL.md` — protocolo para Antigravity
- `CLAUDE.md` (raíz del proyecto) — entry point que referencia los protocolos
- `.ai-bridge/CLAUDE.md` — redirect que apunta a docs/

### Instrucciones de Claude (resumen)

Claude tiene instrucciones de:
1. **CUÁNDO delegar** — todo lo visual, web, imágenes → Antigravity
2. **CUÁNDO NO delegar** — código C++, CMake, JSON, refactoring → lo hace él
3. **CÓMO delegar** — usar `request_antigravity()` con tareas detalladas y específicas
4. **QUÉ DECIR** — informar al usuario que envió la tarea, NO decir "dile a Antigravity"
5. **CÓMO VERIFICAR** — usar `check_antigravity_response()` cuando sea oportuno
6. **NO BLOQUEAR** — seguir trabajando mientras Antigravity procesa

### Instrucciones de Antigravity (resumen)

Antigravity tiene instrucciones de:
1. **RECONOCER** tareas del bridge (contienen "TAREA DEL AI BRIDGE")
2. **PARSEAR** el task_id, la descripción, el contexto
3. **EJECUTAR** usando sus herramientas (generate_image, browser, search_web, etc.)
4. **COMPLETAR** siempre con `process_inbox.py complete <id> "resultado"`
5. **RESPETAR TERRITORIOS** — no tocar código C++ ni build system
6. **PUEDE INVOCAR A CLAUDE** directamente vía CLI si lo necesita

### El mensaje self-contained

La extensión VS Code construye un mensaje que contiene TODO lo que Antigravity necesita para procesar la tarea, sin necesidad de que Antigravity "recuerde" nada o lea archivos de protocolo. El mensaje incluye:
- La tarea
- El contexto
- La prioridad
- Las instrucciones exactas de cómo completar
- El comando de completado con el task_id correcto

---

## Flujos de trabajo

### Flujo 1: Claude necesita un sprite

```
Claude Code                          Sistema                          Antigravity
    │                                   │                                  │
    │ request_antigravity(              │                                  │
    │   "Genera spritesheet 64x64      │                                  │
    │    del boss Infernal Guardian")   │                                  │
    │──────────────────────────────────►│                                  │
    │                                   │                                  │
    │                              MCP Server                              │
    │                              escribe JSON                            │
    │                                   │                                  │
    │                              File Watcher                            │
    │                              detecta archivo                         │
    │                                   │                                  │
    │                              Extension muestra                       │
    │                              notificación                            │
    │                                   │──────────────────────────────────►│
    │                                   │     chat.open(message)           │
    │                                   │                                  │
    │  (Claude sigue trabajando         │                     Antigravity  │
    │   en otras cosas)                 │                     genera el    │
    │                                   │                     sprite       │
    │                                   │                                  │
    │                                   │◄──────────────────────────────────│
    │                                   │  process_inbox.py complete       │
    │                                   │  task_123 "sprite generado"      │
    │                                   │                                  │
    │ check_antigravity_response(       │                                  │
    │   "task_123")                     │                                  │
    │──────────────────────────────────►│                                  │
    │                                   │                                  │
    │◄──────────────────────────────────│                                  │
    │ "✅ Completada: sprite            │                                  │
    │  generado en assets/sprites/..."  │                                  │
```

### Flujo 2: Antigravity necesita razonamiento de Claude

```
Antigravity                          Terminal                         Claude Code
    │                                   │                                  │
    │ run_command:                       │                                  │
    │ npx claude-code -p                │                                  │
    │ "Analiza CombatSystem.cpp         │                                  │
    │  y dime si el patrón Strategy     │                                  │
    │  es viable"                        │                                  │
    │──────────────────────────────────►│──────────────────────────────────►│
    │                                   │                                  │
    │                                   │                     Claude analiza│
    │                                   │                     el código     │
    │                                   │                                  │
    │◄──────────────────────────────────│◄──────────────────────────────────│
    │                                   │   "Sí, el patrón Strategy es     │
    │ Recibe respuesta                  │    viable porque..."             │
    │ en stdout                         │                                  │
```

### Flujo 3: Contexto compartido

```
Claude Code                     .ai-bridge/shared-context/          Antigravity
    │                                   │                                  │
    │ update_shared_context(            │                                  │
    │   "## Análisis del CombatSystem   │                                  │
    │    El sistema usa patrón X...")    │                                  │
    │──────────────────────────────────►│                                  │
    │                              escribe en                              │
    │                              current_analysis.md                     │
    │                                   │                                  │
    │                                   │    (Antigravity en otra sesión)   │
    │                                   │                                  │
    │                                   │◄──────────────────────────────────│
    │                                   │    read_shared_context()         │
    │                                   │    o lee el archivo directamente │
    │                                   │──────────────────────────────────►│
    │                                   │                                  │
    │                                   │          "Ah, Claude ya analizó  │
    │                                   │           el CombatSystem..."    │
```

---

## Estructura de archivos

```
Roguesouls-like/
├── CLAUDE.md                          # Entry point para Claude (referencia al bridge)
│
├── .ai-bridge/                        # ← TODO EL BRIDGE VIVE AQUÍ
│   ├── CLAUDE.md                      # Redirect a docs/CLAUDE_PROTOCOL.md
│   ├── config.json                    # Configuración del protocolo
│   ├── antigravity_mcp_server.py      # Servidor MCP (FastMCP, 6 tools)
│   ├── process_inbox.py              # Helper CLI para procesar buzón
│   │
│   ├── docs/                          # 📚 Documentación completa
│   │   ├── COMPLETE_GUIDE.md          # ← ESTE ARCHIVO (guía exhaustiva)
│   │   ├── CLAUDE_PROTOCOL.md         # Protocolo obligatorio para Claude
│   │   ├── ANTIGRAVITY_PROTOCOL.md    # Protocolo para Antigravity
│   │   └── README.md                  # Resumen rápido
│   │
│   ├── antigravity-inbox/             # 📥 Tareas pendientes para Antigravity
│   │   └── task_XXXXX.json
│   │
│   ├── claude-inbox/                  # 📥 Tareas pendientes para Claude
│   │   └── task_XXXXX.json
│   │
│   ├── responses/                     # ✅ Respuestas completadas
│   │   └── task_XXXXX_response.json
│   │
│   └── shared-context/                # 📋 Contexto compartido
│       └── current_analysis.md
│
└── ~/.vscode/extensions/              # (fuera del proyecto)
    └── ai-bridge-watcher/             # 🧩 Extensión VS Code
        ├── package.json
        └── extension.js
```

---

## Setup e instalación

### Requisitos previos
- VS Code 1.90+ (tienes v1.115 ✅)
- Python 3.10+ (tienes 3.12 ✅)
- Node.js + npm (para npx)
- Claude Code extension en VS Code
- Antigravity (Gemini) extension en VS Code

### Paso 1: Dependencias Python
```bash
pip install fastmcp
```
Esto instala FastMCP v3.2.3+ y todas sus dependencias (mcp, pydantic, uvicorn, etc.)

### Paso 2: Registrar servidor MCP en Claude Code
```bash
npx -y @anthropic-ai/claude-code mcp add antigravity-bridge -- python .ai-bridge/antigravity_mcp_server.py
```

**Verificar:**
```bash
npx -y @anthropic-ai/claude-code mcp list
# Debería mostrar: antigravity-bridge: ✓ Connected
```

### Paso 3: Instalar extensión VS Code
La extensión se instala automáticamente al colocar los archivos en:
```
%USERPROFILE%\.vscode\extensions\ai-bridge-watcher\
├── package.json
└── extension.js
```

### Paso 4: Recargar VS Code
`Ctrl+Shift+P` → "Developer: Reload Window"

La extensión se activa automáticamente al detectar `.ai-bridge/config.json` en el workspace.

### Paso 5: Verificar
Deberías ver en la barra de estado: `🔗 AI Bridge`

---

## Uso diario

### Como usuario

**El 90% del tiempo no tienes que hacer nada.** El bridge es automático:
1. Claude decide que necesita ayuda de Antigravity
2. Envía la tarea via MCP
3. La extensión te notifica y le envía el mensaje a Antigravity
4. Antigravity procesa
5. Claude verifica

**Lo único que podrías necesitar hacer:**
- Si la notificación aparece y Antigravity no está activo, haz click en "Enviar a Antigravity"
- Si ves `🔔 AI Bridge: N tarea(s)` en la status bar, tienes tareas pendientes

### Como Claude Code

Lee `docs/CLAUDE_PROTOCOL.md` para el protocolo detallado. Resumen:
```python
# Enviar tarea
request_antigravity(task="...", context="...", priority="medium")

# Verificar después
check_antigravity_response(task_id="task_XXXX")

# Compartir contexto
update_shared_context(content="## Mi análisis\n...")
```

### Como Antigravity

Lee `docs/ANTIGRAVITY_PROTOCOL.md`. Resumen:
1. Recibes un mensaje con "TAREA DEL AI BRIDGE"
2. Procesas la tarea con tus herramientas
3. Ejecutas: `python .ai-bridge/process_inbox.py complete <id> "resultado"`

---

## Troubleshooting

### El MCP server no conecta
```bash
# Verificar que FastMCP está instalado
python -c "from fastmcp import FastMCP; print('OK')"

# Re-registrar el servidor
npx -y @anthropic-ai/claude-code mcp add antigravity-bridge -- python .ai-bridge/antigravity_mcp_server.py

# Verificar conexión
npx -y @anthropic-ai/claude-code mcp list
```

### La extensión VS Code no se activa
1. Verificar que los archivos existen en `~/.vscode/extensions/ai-bridge-watcher/`
2. Verificar que `.ai-bridge/config.json` existe en el workspace
3. Hacer "Developer: Reload Window"
4. Ver consola de desarrollo (Help → Toggle Developer Tools → Console) para errores

### Claude no tiene la herramienta request_antigravity
1. Verificar MCP: `claude mcp list`
2. Si no conecta, verificar que el path a Python es correcto
3. En Windows, puede necesitar la ruta completa a `python.exe`

### Emojis no se muestran en terminal Windows
El fix está en `process_inbox.py` — fuerza encoding UTF-8 en stdout. Si sigues viendo caracteres raros, ejecuta:
```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$env:PYTHONIOENCODING = "utf-8"
```

### Antigravity no recibe la tarea
1. Verificar que la extensión bridge-watcher está activa (ver status bar)
2. Verificar que el archivo JSON se creó en `antigravity-inbox/`
3. Manualmente: `python .ai-bridge/process_inbox.py list`

---

## Limitaciones conocidas

### 1. Antigravity no tiene listener externo
Antigravity solo reacciona cuando recibe un mensaje en su chat. La extensión VS Code soluciona esto parcialmente mostrando una notificación y enviando el mensaje al chat, pero requiere que VS Code esté abierto y visible.

### 2. No es real-time
Hay un pequeño delay (600ms) entre que Claude crea el archivo y la extensión lo detecta. Para la mayoría de casos es imperceptible.

### 3. Límites de API
Tanto Claude Code como Antigravity tienen límites de uso diario. Si Claude alcanza su límite, no puede enviar ni verificar tareas.

### 4. Single workspace
El bridge asume un solo workspace de VS Code. Si trabajas con múltiples workspaces, cada uno necesitaría su propia configuración `.ai-bridge/`.

### 5. No hay encriptación
Los mensajes se envían como JSON plano en el filesystem. No hay encriptación ni autenticación. Esto es aceptable para uso local pero no para entornos compartidos.

---

## Ideas futuras

### Orquestador con File Watcher (Fase 4)
Un script Python que corre de fondo y automáticamente despacha tareas a Claude vía CLI cuando Antigravity deja algo en `claude-inbox/`. Eliminaría completamente al usuario como intermediario para el flujo Antigravity → Claude.

### Dashboard web
Una página HTML simple que muestra el estado del bridge en tiempo real: tareas pendientes, historial, contexto compartido. Antigravity podría generarla y servirla localmente.

### Multi-proyecto
Hacer que el bridge funcione cross-proyecto — un solo servidor MCP que maneja múltiples `.ai-bridge/` de distintos proyectos.

### Logging y métricas
Registrar cuántas tareas se envían, cuánto tardan, tasa de éxito, etc. Útil para optimizar el flujo de trabajo.

### Auto-routing inteligente
Un clasificador que automáticamente decide si una tarea la puede resolver Claude solo, necesita a Antigravity, o requiere intervención humana. Basado en el tipo de tarea y las capacidades de cada IA.

---

## Glosario

| Término | Definición |
|---------|-----------|
| **AI Bridge** | El sistema completo de comunicación entre Antigravity y Claude Code |
| **MCP** | Model Context Protocol — estándar para conectar herramientas a IAs |
| **FastMCP** | Framework Python para crear servidores MCP rápidamente |
| **Bridge Watcher** | La extensión VS Code que vigila el buzón de Antigravity |
| **Inbox** | Carpeta donde se depositan tareas pendientes |
| **Brain Wiring** | Instrucciones predefinidas que le dicen a cada IA cómo usar el bridge |
| **Shared Context** | Archivo markdown compartido para intercambiar análisis y decisiones |
| **File Watcher** | Mecanismo de VS Code que detecta cambios en el filesystem |
| **Headless mode** | Modo no-interactivo de Claude Code (`-p` flag) |

---

*Guía creada el 12 de Abril de 2026. Actualiza este documento cuando se implementen cambios significativos al bridge.*
