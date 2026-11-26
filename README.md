# C.I.R.O KICK

Generador de kicks + bajo + rumble VST3 para FL Studio (Schranz, Psytrance, Hard Bounce, Hardstyle, Hard Techno, Gabber, Techno, Industrial).

## Conseguir el .vst3 sin instalar nada (GitHub)
1. Subí esta carpeta a un repo de GitHub.
2. Pestaña **Actions** → "Build Windows VST3" → al terminar, bajá el artifact `CIRO-KICK-VST3-Windows`.
3. Copiá la carpeta `CIRO KICK.vst3` a `C:\Program Files\Common Files\VST3\`.
4. En FL Studio: Options → Manage plugins → Find plugins. Aparece como instrumento "C.I.R.O KICK".

## Compilar en tu PC (Windows)
Necesitás Visual Studio (con "Desarrollo para escritorio con C++") y CMake.
```
cmake -B build -A x64
cmake --build build --config Release --target CiroKick_VST3
```
El plugin queda en `build\CiroKick_artefacts\Release\VST3\`.

## Uso rápido
- **RANDOM**: genera un kick nuevo según el género elegido. La flecha ↶ vuelve al anterior.
- **Presets**: 20 por género. Click en el nombre para elegir, flechas para pasar.
- **RUMBLE**: AMOUNT en 0 lo apaga. **BASS**: LEVEL en 0 lo apaga; click en el patrón (ROLLING/OFFBEAT/GALLOP/REVERSE) para cambiarlo. Sigue el BPM del proyecto.
- **Pestañas KICK / RUMBLE / BASS**: cada una con sus controles y sus puntos editables en la onda.
- **LOOP - / +**: cambia el largo del loop (1 a 32 pasos).
- **Onda**: arrastrá los puntos blancos para dar forma al transiente (doble click resetea). Click en la onda = escuchar.
- **Pad XY**: arrastrá la bolita, X = oscuro/brillante, Y = limpio/sucio.
- **LOOP + pasos**: prendé LOOP y tocá los 16 pasos para armar el patrón.
- **✦ MUTATE**: variación chica del sonido actual. **La cara**: tocala.
- **PLAY**: escuchar. Cualquier nota MIDI también dispara el kick.
- **KEY**: si está prendido, el tono sigue la nota que tocás.
- Click en el tipo de drive (SOFT/HARD/FOLD/CRUSH) para cambiarlo, click derecho para volver.

## Pestaña BUILDER (armado por capas)
- **BODY**: STARTER, PUNCHY, TECHNO, DISTORTED, ROUNDED. El dado lo regenera.
- **PUNCH**: HARDSTYLISH, INDUSTRIAL, TOP KICK, TRANSIENT, TOKYO SNAP.
- **TAIL**: BIGROOM, IND CRUNCH, PSY ROLL, PSY SHOT, RAW CRUNCH, REVERSE, ROBOT, RUMBLE.
- **EXTRA A / B**: CLICK, LOW MID, NOISE, NOISE 2, RESONATOR, REVERB, SCHRANZ TOP.
- Click en la categoría = siguiente, click derecho = anterior. Dado = variación nueva de esa capa.
- **BUILD!** arma un kick completo al azar. RANDOM y los presets usan capas según el género.

## La cabeza
- Las pestañas son los dientes de arriba; LOOP, -, + y HIT! son los de abajo. El gráfico está en la garganta.
- Tocá la cabeza para que gire y dispare el kick. Los ojos siguen el mouse.
- RANDOM lo marea, MUTATE le hace sacudir la cabeza, cambiar de pestaña lo hace morder.
- Con más drive se enoja (cejas y cachetes rojos). El rumble lo hace temblar.
