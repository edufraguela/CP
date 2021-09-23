• Hacer que la cola pueda usarse por varios threads a la vez.
• La interfaz no puede cambiar.
• Los elementos de sincronización necesarios estarán ocultos en el interfaz.
• Si un thread intenta quitar un elemento y no hay elementos disponibles esperará hasta
que los haya.
• Minimizar el número de veces que se despierta a los threads.
• Si hay más de SIZE elementos en la cola, un thread que intente añadir un elemento
tiene que esperar a que haya menos elementos.


#define SIZE 100
struct node {
    struct node ∗next;
    struct data ∗data;
};
struct queue {
    struct node ∗first ;
        struct node ∗last ;
};
int queue_add(struct queue ∗queue, struct data ∗data)
{
    struct node ∗new;

        new = malloc(sizeof(∗new));
        if (new ==NULL)
            return −1;
        new − > data = data;
        new − > next = NULL;
        
        if (last != NULL)
            last− > next = new;
        else
            first = new;
        last = new;
}
struct data ∗queue remove(struct queue ∗queue)
{
    struct data ∗result ;
        struct node ∗old;
    if ( first ==NULL)
        return NULL;
    old = first ;
    result = old− > data;
    if (last == first )
        last = NULL;
    first = old− > next;
    free(old);
    return result ;
}
int pthread mutex lock(pthread mutex t ∗mutex);
int pthread mutex trylock(pthread mutex t ∗mutex);
int pthread mutex unlock(pthread mutex t ∗mutex);
int pthread cond wait(pthread cond t ∗cond, pthread mutex t ∗mutex);
int pthread cond broadcast(pthread cond t ∗cond);
int pthread cond signal(pthread cond t ∗cond);





