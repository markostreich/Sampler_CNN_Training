# Workflow:

* __Kompilieren__:
    Zum kompilieren des Projekts einfach ```make``` eingeben.
    Nach Abschluss des Kompiliervorgangs findet sich in __bin__ ```$RUNDIR``` eine ausführbare Datei __runner__ ```$TARGET```

* __Zurücksetzen__:
    Mit ```make clean``` werden alle Kompilierten Dateien wieder gelöscht.

* __Testen__:
    Zum testen wird ```make test``` aufgerufen.


# Zu beachten:

1.  Die Makefile funktioniert nur in Linux/Unix Umbegungen

2.  Da Include Pfade für jedes System Verschieden sind, werden hier nur die Standartpfade ```$INC``` verwendet.
    Sollten diese Pfade nicht Stimmen, kann ```$INC``` als [Umgebungsvariable](https://wiki.ubuntuusers.de/Umgebungsvariable/) überschrieben werden.
