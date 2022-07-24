#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <queue>
#include <ctime>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

using namespace std;

struct Customer
{
    int ID;
    int Teller_ID;
    int t_arr;
    int period;
    int begin_time;
    pthread_t Thread_num;
    Customer(int ID, int t_arr, int period) : ID(ID), t_arr(t_arr), period(period), begin_time(0), Thread_num(0) {}
};

struct Teller
{
    int ID;
    pthread_t Thread_num;
    Teller() : ID(0), Thread_num(0) {}
};

int Customer_Num;
int Teller_Num = 20;
sem_t Customer_sem;
sem_t Teller_sem;
vector<Customer> Customer_list;
vector<Teller> Teller_list(Teller_Num);
queue<int> Customer_Q;

time_t Begin_Moment;
pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER;

void in_file()
{
    ifstream infile("test.txt", ios::in);
    int ID;
    int t_arr;
    int period;
    while (true)
    {
        infile >> ID >> t_arr >> period;
        if (infile.fail())
            break;
        Customer_list.push_back(Customer(ID, t_arr, period));
        ++Customer_Num;
    }
}

void *Thread_C(void *C)
{
    Customer &customer = *(Customer *)C;
    sleep(customer.t_arr);
    pthread_mutex_lock(&LOCK);
    cout << "Time " << time(NULL) - Begin_Moment << ": Customer " << customer.ID << " arrive at bank" << endl;
    Customer_Q.push(customer.ID);
    sem_post(&Customer_sem);
    pthread_mutex_unlock(&LOCK);

    sem_wait(&Teller_sem);
    pthread_mutex_lock(&LOCK);
    cout << "Time " << time(NULL) - Begin_Moment << ": No." << customer.Teller_ID << " Teller starts serve Customer " << customer.ID << endl;
    pthread_mutex_unlock(&LOCK);
    sleep(customer.period);
    pthread_exit(NULL);
    return NULL;
}

void *Thread_T(void *T)
{
    Teller &teller = *(Teller *)T;
    while (true)
    {
        sem_wait(&Customer_sem);
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        pthread_mutex_lock(&LOCK);
        Customer &customer = Customer_list[Customer_Q.front() - 1];
        Customer_Q.pop();
        customer.Teller_ID = teller.ID;
        customer.begin_time = time(NULL) - Begin_Moment;
        pthread_mutex_unlock(&LOCK);

        sem_post(&Teller_sem);
        sleep(customer.period);
        pthread_mutex_lock(&LOCK);
        cout << "Time " << time(NULL) - Begin_Moment << ": Customer " << customer.ID << "'s service finished" << endl;
        pthread_mutex_unlock(&LOCK);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    }
}

int main()
{
    pthread_mutex_init(&LOCK, NULL);
    sem_init(&Customer_sem, 0, 0);
    sem_init(&Teller_sem, 0, 0);
    in_file();
    Begin_Moment = time(NULL);
    for (int i = 0; i < Customer_list.size(); ++i)
    {
        Customer &customer = Customer_list[i];
        int Exc = pthread_create(&(customer.Thread_num), NULL, Thread_C, (void *)(&customer));
        if (Exc)
        {
            cerr << "Create counter thread " << i + 1 << " failed. " << strerror(Exc) << endl;
            exit(-1);
        }
    }
    for (int i = 0; i < Teller_list.size(); ++i)
    {
        Teller &teller = Teller_list[i];
        teller.ID = i + 1;
        int Exc = pthread_create(&(teller.Thread_num), NULL, Thread_T, (void *)(&teller));
        if (Exc)
        {
            cerr << "Create teller thread " << i + 1 << " failed. " << strerror(Exc) << endl;
            exit(-1);
        }
    }
    for (auto &customer : Customer_list)
        pthread_join(customer.Thread_num, NULL);

    for (auto &teller : Teller_list)
        pthread_cancel(teller.Thread_num);

    for (auto &teller : Teller_list)
        pthread_join(teller.Thread_num, NULL);

    return 0;
}