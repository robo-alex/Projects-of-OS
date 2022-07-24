from concurrent.futures import ThreadPoolExecutor
from threading import Lock, Event, Thread
import random
import time

class QuickSort:
    def __init__(self, data, Thread_Max = 20):
        self.Data = data
        self.Threads = 0
        self.Threads_lock = Lock()
        self.Theads_end =Event()
        self.Exe = ThreadPoolExecutor(max_workers=Thread_Max)

    @property
    def data(self):
        return self.Data

    def Q_Sort(self, L, R, new_Threads = False):
        if R <= L:
            return

        Index = [L, (L + R) // 2, R]
        Center = sorted(Index, key=lambda n: self.Data[n])[1]
        self.Data[R] = self.Data[Center]
        self.Data[Center] = self.Data[R]

        Val_center = self.Data[R]
        i = L
        j = R - 1
        while True:
            while self.Data[i] < Val_center: i += 1
            while j > i and self.Data[j] >= Val_center: j -= 1
            if i < j:
                self.Data[i], self.Data[j] = self.Data[j], self.Data[i]
            else:
                break


        self.Data[R] = self.Data[i]
        self.Data[i] = Val_center

        if R - i > 1000:
            with self.Threads_lock:
                self.Threads += 1
            self.Exe.submit(self.Q_Sort, i + 1, R, True)
        else:
            self.Q_Sort(i + 1, R)

        if i - L > 1000:
            with self.Threads_lock:
                self.Threads += 1
            self.Exe.submit(self.Q_Sort, L, i - 1, True)
        else:
            self.Q_Sort(L, i - 1)

        if new_Threads:
            with self.Threads_lock:
                self.Threads -= 1
            self.Theads_end.set()

    def Sort(self):
        self.Theads_end.clear()
        if len(self.Data) > 1000:
            with self.Threads_lock:
                self.Threads += 1
            self.Exe.submit(self.Q_Sort, 0, len(self.Data) - 1, True)
        
            while True:
                self.Theads_end.wait()
                with self.Threads_lock:
                    if self.Threads == 0:
                        return
                    self.Theads_end.clear()
        else:
            self.Q_Sort(0, len(self.Data) - 1)

def main():
    time_start=time.time()
    
    with open('data_to_be_sorted.txt', 'w') as f:
        rand = (str(random.randint(0, 2 << 23)) + '\n' for n in range(1000000))
        f.writelines(rand)

    data =[]
    with open("data_to_be_sorted.txt") as f:
        for row in f:
            data.append(int(row))

    Sorting = QuickSort(data, Thread_Max=20)
    Sorting.Sort()

    with open("data_sorted.txt",'w') as f:
        f.writelines(str(data_sorted) + '\n' for data_sorted in Sorting.data)

    time_end=time.time()
    print('Totally cost',time_end - time_start)

if __name__ == '__main__':
    main()
