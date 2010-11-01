// -*- c-basic-offset: 4 -*-
#ifndef CLICK_NULLTASK_HH
#define CLICK_NULLTASK_HH
#include <click/element.hh>
#include <click/task.hh>
CLICK_DECLS

/*
=c

NullTask([<keyword> LIMIT, STOP])

=s test

associated with a do-nothing Task

=d

NullTask simply schedule a task which, when scheduled, does nothing.
This can be useful for benchmarking.

=over 8

=item LIMIT

Unsigned.  NullTask will schedule itself at most LIMIT times.  0 means
forever.  Default is 0.

=item STOP

Boolean.  If true, NullTask will stop the driver when LIMIT is
reached.  Default is false.

=back

=h count r

Returns the number of times the element has been scheduled.

=a

ScheduleInfo
*/

class NullTask : public Element { public:

    NullTask();
    ~NullTask();

    const char *class_name() const		{ return "NullTask"; }
    const char *port_count() const		{ return PORTS_0_0; }
    const char *processing() const		{ return AGNOSTIC; }

    int configure(Vector<String> &, ErrorHandler *);
    int initialize(ErrorHandler *);
    void add_handlers();

    bool run_task(Task *);

  private:

    uint32_t _count;
    uint32_t _limit;
    Task _task;
    bool _stop;

};

CLICK_ENDDECLS
#endif
