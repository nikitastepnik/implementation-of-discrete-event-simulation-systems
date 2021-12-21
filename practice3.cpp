#include <iostream>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <queue>
#include <iomanip>
#include <condition_variable>
#include <fstream>
#define R1 9
#define G1 7
#define B1 8
#include <mutex>
std::condition_variable cv1;
std::condition_variable cv2;
std::condition_variable cv3;
std::condition_variable cv4;

bool state1 = false;
bool state2 = false;
bool state3 = false;
bool state4 = false;
bool first_cashier_use () {return state1;}
bool second_cashier_use() {return state2;}
bool first_queue_is_exist() {return state3;}
bool second_queue_is_exist() {return state4;}
double globaltime = 0;


double uniform (double min, double max)
{
    return (double)(rand())/RAND_MAX*(max-min)+min;
}


class Cashier
{
public:
    Cashier ();
    Cashier (int, int, int);
    void waiting (std::ofstream&);
    void job(std::ofstream&);
    std::queue <int> q;
    int id;

private:
    int min, max, q_num;
    std::mutex mtx, mtx2;
    double worktime;
    double arrivetime;

};


void Cashier::job(std::ofstream& f)
{

    while (globaltime < 3600) {
        std::unique_lock<std::mutex> ulm1(mtx);
        if (q_num == 1)
            cv1.wait(ulm1, first_cashier_use);
        else
            cv2.wait(ulm1, second_cashier_use);
        arrivetime = globaltime;
        id = q.front();
        worktime = uniform(min, max);
        f<< "В момент времени " << std::fixed << std::setprecision(3) << arrivetime
                  << " транзакт с идентификатором "
                  << id << " занял устройство " << q_num << std::endl;
        std::this_thread::sleep_for(std::chrono::duration<double, std::nano>(worktime));
        f << "В момент времени " << std::fixed << std::setprecision(3) << arrivetime + worktime
                  << " транзакт с идентификатором "
                  << id << " освободил устройство " << q_num <<" и покинул модель" << std::endl;
        q.pop();
        if (q.size() == 0 and q_num == 1)
            state1 = false;
        else if (q.size() == 0 and q_num == 2)
            state2 = false;
    }


}

void Cashier::waiting(std::ofstream &f)
{
    while (globaltime < 3600) {
        std::unique_lock<std::mutex> ulm1(mtx2);
        if (q_num == 1)
            cv3.wait(ulm1, first_queue_is_exist);
        else
            cv4.wait(ulm1, second_queue_is_exist);
        arrivetime = globaltime;
        f << "В момент времени " << std::fixed << std::setprecision(3) << arrivetime
                  << " транзакт с идентификатором "
                  << q.front() << " занял очередь " << q_num << std::endl;

        if (q_num == 1)
        {state3 = false;
            state1 = true;
            cv1.notify_one();}
        else if (q_num == 2)
        {state4 = false;
            state2 = true;
            cv2.notify_one();}

    }
}

Cashier::Cashier (int n, int x,int num)
{
    min = n;
    max = x;
    q_num = num;
}

int main (int argc, char* argv[])
{
    int id = 1;
    system("chcp 65001");
    srand(time(NULL));

    Cashier q1 (R1, R1 + G1 + B1, 1);
    Cashier q2 (G1, R1 + G1 + B1, 2);

    std::ofstream fout;

    fout.open("log.txt", std::ios::trunc);



    std::thread th1 ([&]()
                     {
                         q1.job(fout);
                     });

    std::thread th2 ([&]()
                     {
                         q1.waiting(fout);
                     });

    std::thread th3 ([&]()
                     {
                         q2.job(fout);
                     });

    std::thread th4 ([&]()
                     {
                         q2.waiting(fout);
                     });


    while (globaltime < 3600)
    {
        double time = uniform(0, 24);
        globaltime += time;

        fout << "В момент времени " << std::fixed << std::setprecision(3) << globaltime << " транзакт с идентификатором "
                  << id << " вошел в модель" << std::endl;


        if (q1.q.size() <= q2.q.size())
        {
            q1.q.push(id);
            state3 = true;
            cv3.notify_all();

        }
        else
        {
            q2.q.push(id);
            state4 = true;
            cv4.notify_all();
        }

        id += 1;
        std::this_thread::sleep_for(std::chrono::duration<double, std::nano>(time));

    }

    th1.join();
    th2.join();
    th3.join();
    th4.join();

    fout.close();
    return 0;
}