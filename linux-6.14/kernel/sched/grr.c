/* kernel/sched/grr.c */
#include "sched.h"

#ifdef CONFIG_GRR_SCHED

#define GRR_TIME_SLICE_MS 100

/* Prototype to fix "missing prototype" warning */
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

/* FIXED: Return type must be bool */
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

/* FIXED: Replaces check_preempt_curr */
static void wakeup_preempt_grr(struct rq *rq, struct task_struct *p, int flags)
{
    /* No preemption in simple RR, new tasks wait at the back */
}

/* FIXED: Takes 'prev' argument */
static struct task_struct *pick_next_task_grr(struct rq *rq, struct task_struct *prev)
{
    struct grr_rq *grr = &rq->grr;
    struct task_struct *p;

    if (!grr->nr_running) 
        return NULL;

    /* If prev was GRR and is still runnable, put it back? 
       Standard logic handles this in put_prev_task or upper layers.
       We simply pick the head of the queue. */
       
    p = list_first_entry(&grr->queue, struct task_struct, grr_list);
    p->se.exec_start = rq_clock_task(rq);
    return p;
}

/* FIXED: Takes 'next' argument */
static void put_prev_task_grr(struct rq *rq, struct task_struct *prev, struct task_struct *next)
{
    /* Update stats if needed, otherwise empty */
}

/* FIXED: Replaces set_curr_task. Argument 'first' indicates if it's the first time. */
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
    if (--p->grr_time_slice) return;

    p->grr_time_slice = msecs_to_jiffies(GRR_TIME_SLICE_MS);
    if (rq->grr.nr_running > 1) {
        list_move_tail(&p->grr_list, &rq->grr.queue);
        resched_curr(rq);
    }
}

static void switched_to_grr(struct rq *rq, struct task_struct *p)
{
    if (p->on_rq && rq->curr != p) resched_curr(rq);
}

static void update_curr_grr(struct rq *rq) { }

static void prio_changed_grr(struct rq *rq, struct task_struct *p, int oldprio) { }

/* MODERN DEFINITION */
DEFINE_SCHED_CLASS(grr) = {
    .enqueue_task       = enqueue_task_grr,
    .dequeue_task       = dequeue_task_grr,
    .yield_task         = yield_task_grr,
    .wakeup_preempt     = wakeup_preempt_grr, /* Renamed from check_preempt_curr */
    
    .pick_next_task     = pick_next_task_grr,
    .put_prev_task      = put_prev_task_grr,
    .set_next_task      = set_next_task_grr,  /* Renamed from set_curr_task */

    .select_task_rq     = select_task_rq_grr,
    .task_tick          = task_tick_grr,
    
    .switched_to        = switched_to_grr,
    .prio_changed       = prio_changed_grr,
    .update_curr        = update_curr_grr,
};
#endif
