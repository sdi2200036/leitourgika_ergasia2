/* kernel/sched/grr.c - Linux 6.14 Compatible */
#include "sched.h"

#ifdef CONFIG_GRR_SCHED

#define GRR_TIME_SLICE_MS 100

/* Prototype to silence warning */
void init_grr_rq(struct grr_rq *grr_rq);

void init_grr_rq(struct grr_rq *grr_rq)
{
    INIT_LIST_HEAD(&grr_rq->queue);
    grr_rq->nr_running = 0;
}

static void enqueue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
    struct grr_rq *grr = &rq->grr;
    list_add_tail(&p->grr_list, &grr->queue);
    grr->nr_running++;
    add_nr_running(rq, 1);
}

static bool dequeue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
    struct grr_rq *grr = &rq->grr;
    list_del(&p->grr_list);
    grr->nr_running--;
    sub_nr_running(rq, 1);
    return true;
}

static void yield_task_grr(struct rq *rq)
{
    struct task_struct *p = rq->curr;
    struct grr_rq *grr = &rq->grr;
    list_move_tail(&p->grr_list, &grr->queue);
}

static void wakeup_preempt_grr(struct rq *rq, struct task_struct *p, int flags)
{
    /* No preemption for GRR */
}

static struct task_struct *pick_next_task_grr(struct rq *rq, struct task_struct *prev)
{
    struct grr_rq *grr = &rq->grr;
    struct task_struct *p;

    if (!grr->nr_running) 
        return NULL;

    p = list_first_entry(&grr->queue, struct task_struct, grr_list);
    p->se.exec_start = rq_clock_task(rq);
    return p;
}

static void put_prev_task_grr(struct rq *rq, struct task_struct *prev, struct task_struct *next)
{
}

static void set_next_task_grr(struct rq *rq, struct task_struct *p, bool first)
{
    p->se.exec_start = rq_clock_task(rq);
}

int select_task_rq_grr(struct task_struct *p, int cpu, int flags)
{
    int i, target = cpu;
    unsigned int min_load = UINT_MAX;
    int group = p->grr_group ? p->grr_group : 1;

    for_each_online_cpu(i) {
        if (grr_cpu_group[i] == group) {
            unsigned int load = cpu_rq(i)->grr.nr_running;
            if (load < min_load) {
                min_load = load;
                target = i;
            }
        }
    }
    return target;
}

static void task_tick_grr(struct rq *rq, struct task_struct *p, int queued)
{
    /* 1. Decrement the slice */
    if (p->grr_time_slice > 0)
        p->grr_time_slice--;

    /* 2. If it is NOT zero, keep running */
    if (p->grr_time_slice > 0)
        return;

    /* 3. RECHARGE THE SLICE (Crucial Fix!) */
    p->grr_time_slice = 10; /* Or msecs_to_jiffies(100) */

    /* 4. Rotate the queue if other tasks are waiting */
    if (rq->grr.nr_running > 1) {
        list_move_tail(&p->grr_list, &rq->grr.queue);
        resched_curr(rq);
    }
}

static void switched_to_grr(struct rq *rq, struct task_struct *p)
{
    if (p->on_rq && rq->curr != p) resched_curr(rq);
}

/* Helper to move tasks off a CPU that changed groups */

static void prio_changed_grr(struct rq *rq, struct task_struct *p, int oldprio) { }
static void update_curr_grr(struct rq *rq) { }

/* --- SAFETY STUBS --- */
static void task_fork_grr(struct task_struct *p) { }
static void task_dead_grr(struct task_struct *p) { }
static void switched_from_grr(struct rq *rq, struct task_struct *p) { }
static void migrate_task_rq_grr(struct task_struct *p, int new_cpu) { }
static bool yield_to_task_grr(struct rq *rq, struct task_struct *p) { return true; }
static void set_cpus_allowed_grr(struct task_struct *p, struct affinity_context *ctx) { }

/* FIXED: Correct signature for Kernel 6.14 (Returns int, only 2 args) */
static unsigned int get_rr_interval_grr(struct rq *rq, struct task_struct *task) 
{ 
    return msecs_to_jiffies(GRR_TIME_SLICE_MS); 
}

/* -------------------- */

DEFINE_SCHED_CLASS(grr) = {
    .enqueue_task       = enqueue_task_grr,
    .dequeue_task       = dequeue_task_grr,
    .yield_task         = yield_task_grr,
    .wakeup_preempt     = wakeup_preempt_grr,
    
    .pick_next_task     = pick_next_task_grr,
    .put_prev_task      = put_prev_task_grr,
    .set_next_task      = set_next_task_grr,
    
    .select_task_rq     = select_task_rq_grr,
    .task_tick          = task_tick_grr,
    
    .switched_to        = switched_to_grr,
    .prio_changed       = prio_changed_grr,
    .update_curr        = update_curr_grr,

    /* Full hooks list with correct signatures */
    .task_fork          = task_fork_grr,
    .task_dead          = task_dead_grr,
    .switched_from      = switched_from_grr,
    .migrate_task_rq    = migrate_task_rq_grr,
    .yield_to_task      = yield_to_task_grr,
    .get_rr_interval    = get_rr_interval_grr,
    .set_cpus_allowed   = set_cpus_allowed_grr,
};
#endif