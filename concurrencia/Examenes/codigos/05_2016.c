Se quiere diseñar un sistema para controlar la entrada de grupos a un museo con capacidad
limitada (capacity). Los grupos tienen que gestionar la entrada y salida de forma conjunta,
es decir:
• Todos los miembros tienen que entrar al mismo tiempo.
• En caso de que no haya espacio, todos los miembros del grupo esperarán hasta que lo
haya, aunque hubiera plazas para que una parte del grupo entrase.
Cada miembro del grupo ejecuta la función visitor, y recibe como parámetro una estructura
group que comparte con todos los demás miembros del mismo grupo.
Añada el código necesario para gestionar la entrada y salida de grupos a la función visitor.
Intente minimizar el número de veces que se despierta a los procesos en espera.

struct group {
    const int members; // total number of members in the group
    pthread mutex t ∗counter m;
    int counter; // members currently inside the museum
};

int capacity;
pthread mutex t lock ;
pthread cond t no space;

void ∗visitor(void ∗arg) {
    struct group ∗grp = arg;

    // Gestion de acceso
    visit ();
    // Gestion de salida
}

Solucion

struct group {
    const int members; // Total Number of members in the group
    pthread mutex t ∗counter m;
    int counter; // Members active
    pthread cond t group entry;
};

int capacity; // Number of available places
pthread mutex t lock ;
pthread cond t no space;

void ∗visitor(void ∗arg) {
    struct group ∗grp = arg;
    pthread mutex lock(grp − > counterm);
    if (counter==0) {
        pthread mutex lock(&lock);
        while(free places< grp − > members)
            pthread cond wait(&no space, &lock);
        free places −= grp − > members;
        pthread mutex unlock(&lock);
    }
    grp − > counter++;
    if (grp − > counter==grp − > members)
        pthread cond broadcast(grp − > group entry);
    else
        pthread cond wait(grp − > group entry, grp − > counterm);
    pthread mutex unlock(grp − > counterm);

    visit ();

    pthread mutex lock(grp − > counterm);
    grp − > counter−−;
    if (counter==0)
        pthread mutex lock(&lock);
        free places += grp − > members;
        pthread cond broadcast(&no space);
        pthread mutex unlock(&lock);
    }
    pthread mutex unlock(grp − > counterm);
}
