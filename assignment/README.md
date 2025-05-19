# Párhuzamos eszközök programozása beadandó feladat
A beadandó feladat célja WAV formátumú hangfájlok elemzése. A fájl megfelelő beolvasását követően a bal és jobb csatorna hangmintái közötti különbség kerül vizsgálatra. Ezeket a különbségeket az OpenCL kernel adott elemszámú blokkokban átlagolja.

- **Makefile**:  A program különböző részeinek (main.exe, runner.exe, graph.exe) fordítását végző parancsokat tartalmazó fájl.
- **dokumentacio.docx**: A feladat dokumentációja.
- **main.c**: A program fő belépési pontját tartalmazó fájl, amely összehangolja az OpenCL kernel betöltését, a hibakezelést és a WAV fájlok feldolgozását.
- **meresek.xlsx**: A mérési adatok táblázatai, diagramokkal szemléltetve.
- **runner.c**: Az adatok összegyűjtéséhez használt segédprogram.

## assets mappa
- **errorcodes.csv**: OpenCL a hibakódok értékeit, neveit, és leírásait tartalmazza.
- **remalomfold.wav**: A hangfájl, melyet a program beolvas.

## include mappa
- **errorcode.h**: Az errorcode struktúra definíciója.
- **kernel_loader.h**: Az OpenCL kernel forráskódját fájlból beolvasó függvény deklarációja.
- **wav.h**: A WAV fájlok kezeléséhez használt adatszerkezet.

## kernels mappa
- **averages.cl**: az OpenCL kernel kódja.

## src mappa 
- **errors.c**: Az OpenCL hibakódok könnyebb értelmezését segíti. Egy megadott hibakódhoz tartozó részletes hibaadatokat kiíró függvényt tartalmaz.
- **kernel_loader.c**: Az OpenCL kernel forrásfájlt beolvasó függvény.
- **wav.c**: A WAV fájlok beolvasását végző függvényt tartalmazza.
