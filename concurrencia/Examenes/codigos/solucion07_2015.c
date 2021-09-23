#define SIZE 100
struct node {
struct node ∗next;
struct data ∗data;
};
struct queue {
struct node ∗first ;
struct node ∗last ;
pthread mutex t ∗lock ;
pthread cond t ∗empty;
pthread cond t ∗full ;
int elements;
int waiting empty;
int waiting full ;
};
int queue add(struct queue ∗queue,
{
struct node ∗new;
struct data ∗data)
new = malloc(sizeof(∗new));
if (new ==NULL)
return −1;
new − > data = data;
new − > next = NULL;
pthread mutex lock(queue − >lock);
while(queue − > elements==SIZE) {
queue − >waiting full++;
pthread cond wait(queue − >full , queue − >lock);
}
if (queue − >last != NULL)
queue − >last− > next = new;
else
queue − >first = new;
queue − >last = new;
if (queue − > waiting empty) {
queue − > waiting empty−−;
pthread cond signal(queue − > empty);
}
queue − > elements++;
pthread mutex unlock(queue − >lock);
}
struct data ∗queue remove(struct queue ∗queue)
{
struct data ∗result ;
struct node ∗old;
pthread mutex lock(queue − >lock);
while(queue − >first ==NULL) {
queue − > waiting empty++;
pthread cond wait(queue − > empty, queue − >lock);
}
old = queue − >first ;
result = old− > data;
if (queue − >last == first )
queue − >last = NULL;
queue − >first = old− > next;
queue − > elements−−;
if (queue − >waiting full) {
queue − >waiting full−−;
pthread cond signal(queue − >full );
}
pthread mutex unlock(queue − >lock);
free(old);
return result ;
}