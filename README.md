# CIRO KICK

Generador de kicks VST3 para FL Studio (Schranz, Psytrance, Hard Bounce, Hardstyle, Hard Techno, Gabber).

## Conseguir el .vst3 sin instalar nada (GitHub)
1. Subí esta carpeta a un repo de GitHub.
2. Pestaña **Actions** → "Build Windows VST3" → al terminar, bajá el artifact `CIRO-KICK-VST3-Windows`.
3. Copiá la carpeta `CIRO KICK.vst3` a `C:\Program Files\Common Files\VST3\`.
4. En FL Studio: Options → Manage plugins → Find plugins. Aparece como instrumento "CIRO KICK".

## Compilar en tu PC (Windows)
Necesitás Visual Studio 2022 (con "Desarrollo para escritorio con C++") y CMake.
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target CiroKick_VST3
```
El plugin queda en `build\CiroKick_artefacts\Release\VST3\`.

## Uso rápido
- **RANDOM**: genera un kick nuevo según el género elegido. La flecha ↶ vuelve al anterior.
- **PLAY**: escuchar. Cualquier nota MIDI también dispara el kick.
- **KEY**: si está prendido, el tono sigue la nota que tocás.
- Click en el tipo de drive (SOFT/HARD/FOLD/CRUSH) para cambiarlo, click derecho para volver.
