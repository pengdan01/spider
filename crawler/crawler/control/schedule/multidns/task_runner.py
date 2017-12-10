from threading import Thread, Event

__all__ = ["map_task"]

__version__ = '1.0'

class task(object):
    def __init__(self, func, finish=None):
        self.__func = func
        self.__finish = finish
        self.__thread = Thread(target=self.run)
        self.__thread.daemon = True

        self.__start_event = Event()
        self.__start_event.clear()
        
        self.__terminate_event = Event()
        self.__terminate_event.clear()

        self.__terminated = False
        self.__started = False
        self.__thread.start()
        
    @property
    def started(self):
        return self.__started

    @property
    def data(self):
        return self.__data

    @data.setter
    def data(self, d):
        self.__data = d
    
    def run(self):
        while not self.__terminated:
            self.__start_event.wait(10)
            # clear the start event before next waiting
            self.__start_event.clear()
            
            if self.__started:
                # let threads joining this task wait
                self.__terminate_event.clear()
                
                try:
                    self.__func(self.__data)
                except:
                    pass

                self.__started = False
                if self.__finish:
                    self.__finish(self)
                    
                # after running, let threads joining this task resume
                self.__terminate_event.set()

    def start(self):
        self.__started = True
        self.__start_event.set()

    def terminate(self):
        self.__terminated = True

    def terminate_and_join(self):
        self.terminate()
        if self.started:
            self.__terminate_event.wait()

def get_free_task(task_list):
    for t in task_list:
        if not t.started:
            return t
    return None

def __set_event_callback(e):
    def __callback(t):
        e.set()
    return __callback

def map_task(source, func, thread_limit = 10):
    '''Run func in up to thread_limit threads, with data from
    source arg passed into it.
    
    The arg source must be iterable. map_task() will call next()
    each time a free thread is available.

    The function will block until all of the tasks are completed.
    '''
    e = Event()
    task_list = []
    for i in xrange(0, thread_limit):
        task_list.append(task(func, __set_event_callback(e)))
    iterer = source.__iter__()
    while 1:
        t = get_free_task(task_list)
        if t:
            try:
                data = iterer.next()
                t.data = data
                t.start()
            except StopIteration:
                # iteration is stopped
                for a_task in task_list:
                    a_task.terminate_and_join()
                return
        else:
            e.clear()
            e.wait()