# Workflow de Build & Flash (CMake Presets)

Este documento explica **cómo y por qué** usar el flujo de build y flash del proyecto.
Está pensado como referencia cuando algo no queda claro o cambia la configuración.

---

## Conceptos clave

CMake trabaja en **dos fases**:

1. **Configurar**: define *cómo* se va a construir el proyecto.
2. **Construir**: compila (y ejecuta targets como `flash`).

El flasheo es simplemente **un target de CMake**, no un paso especial.

---

## Fases del flujo

### 1. Configurar (raro)
```bash
cmake --preset pico2w-debug
```

Esto:
- Crea el directorio de build asociado al preset.
- Fija `PICO_BOARD`, `FW_NAME` y `CMAKE_BUILD_TYPE`.
- Genera los archivos de Ninja.

Solo se hace:
- la primera vez,
- o cuando cambia la configuración (placa, modo, nombre, CMakeLists).

---

### 2. Compilar (frecuente)
```bash
cmake --build --preset build-pico2w-debug
```

- Build incremental.
- Solo recompila lo que cambió.

---

### 3. Flashear (frecuente)
```bash
cmake --build --preset flash-pico2w-debug
```

- Compila lo necesario (si algo cambió).
- Ejecuta el target `flash`.
- Programa la placa vía Debug Probe (SWD).

---

## ¿Tengo que ejecutar siempre las tres fases?

No.

### Casos típicos

**Primera vez**
```bash
cmake --preset pico2w-debug
cmake --build --preset flash-pico2w-debug
```

**Cambio de código**
```bash
cmake --build --preset flash-pico2w-debug
```

**Solo compilar**
```bash
cmake --build --preset build-pico2w-debug
```

**Solo flashear**
```bash
cmake --build build/pico2w-debug --target flash
```

---

## Cuándo borrar el build directory

Solo cuando cambias configuración:

- `PICO_BOARD`
- `FW_NAME`
- `CMAKE_BUILD_TYPE`
- estructura de CMake
- build corrupto

Ejemplo:
```bash
rm -rf build/pico2w-debug
cmake --preset pico2w-debug
cmake --build --preset flash-pico2w-debug
```

No borres el build por cambios normales de código.

---

## Regla mental

- **Siempre**: `cmake --build --preset flash-*`
- **A veces**: `cmake --preset *`
- **Casi nunca**: borrar `build/`

---

## Filosofía

- El proyecto declara *qué* se puede construir (presets).
- CMake ejecuta *cómo* se construye.
- El flasheo es un target normal.
- BOOTSEL es para emergencias, no para el flujo diario.

Este enfoque escala bien a proyectos largos y múltiples placas.
