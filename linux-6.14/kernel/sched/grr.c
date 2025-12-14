#include "sched.h"

#ifdef CONFIG_GRR_SCHED

/* 100ms time slice. Assuming CONFIG_HZ=1000, 100ms = 100 ticks. 
   If HZ=100, 100ms = 10 ticks. We use msecs_to_jiffies just in case. */
#define GRR_TIME_SLICE_MS 100

void init_grr_rq(struct grr_rq *grr_rq)
{
    INIT_LIST_HEAD(&grr_rq->queue);
    grr_rq->nr_running = 0;
}

static void enqueue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
    struct grr_rq *grr = &rq->grr;
    
    /* Add to the tail of the list */
    list_add_tail(&p->grr_list, &grr->queue);
    grr->nr_running++;
    add_nr_running(rq, 1);
}

static void dequeue_task_grr(struct rq *rq, struct task_struct *p, int flags)
{
    struct grr_rq *grr = &rq->grr;
    
    list_del(&p->grr_list);
    grr->nr_running--;
    sub_nr_running(rq, 1);
}

static void yield_task_grr(struct rq *rq)
{
    struct task_struct *p = rq->curr;
    struct grr_rq *grr = &rq->grr;
    
    /* Move current task to end of list */
    list_move_tail(&p->grr_list, &grr->queue);
}

static void check_preempt_curr_grr(struct rq *rq, struct task_struct *p, int flags)
{
    /* Simple RR doesn't usually preempt unless time slice is done */
}

static struct task_struct *
pick_next_task_grr(struct rq *rq) /* previous args :struct task_struct *prev, struct rq_flags *rf */
{
    struct grr_rq *grr = &rq->grr;
    struct task_struct *p;

    if (!grr->nr_running)
        return NULL;

    /* Pick the first task in the list */
    p = list_first_entry(&grr->queue, struct task_struct, grr_list);

    /* Standard kernel boiler plate for switching */
    /*if (prev)
        put_prev_task(rq, prev);*/
    
    p->se.exec_start = rq_clock_task(rq);
    return p;
}

static void put_prev_task_grr(struct rq *rq, struct task_struct *p)
{
    /* Update execution time stats if needed */
}

/* * This is crucial. When a task wakes up, where does it go?
 * Requirement: "idlest CPU core... among the cores remaining in the group"
 */
static int select_task_rq_grr(struct task_struct *p, int cpu, int flags)
{
    int i;
    int target_cpu = cpu;
    unsigned int min_load = UINT_MAX;
    int group = p->grr_group;

    /* Valid group safety check */
    if (group != GRR_DEFAULT && group != GRR_PERFORMANCE)
        group = GRR_DEFAULT;

    /* Iterate all online CPUs */
    for_each_online_cpu(i) {
        if (grr_cpu_group[i] == group) {
            unsigned int load = cpu_rq(i)->grr.nr_running;
            if (load < min_load) {
                min_load = load;
                target_cpu = i;
            }
        }
    }
    return target_cpu;
}

static void set_curr_task_grr(struct rq *rq)
{
    struct task_struct *p = rq->curr;
    p->se.exec_start = rq_clock_task(rq);
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
    /* If we just switched to GRR, and we are not running, ensure we run soon */
    if (p->on_rq && rq->curr != p)
        resched_curr(rq);
}

static void update_curr_grr(struct rq *rq) { }
static void prio_changed_grr(struct rq *rq, struct task_struct *p, int oldprio) { }

/* Define the sched_class structure */
DEFINE_SCHED_CLASS(grr) = {
    
    /* NO .next POINTER HERE! */

    .enqueue_task       = enqueue_task_grr,
    .dequeue_task       = dequeue_task_grr,
    .yield_task         = yield_task_grr,
    .check_preempt_curr = check_preempt_curr_grr,

    .pick_next_task     = pick_next_task_grr,
    .put_prev_task      = put_prev_task_grr,
    .set_next_task      = set_next_task_grr, /* You might need this helper or leave NULL if not strictly required */

    .select_task_rq     = select_task_rq_grr,
    .set_curr_task      = set_curr_task_grr,
    .task_tick          = task_tick_grr,

    .switched_to        = switched_to_grr,
    .prio_changed       = prio_changed_grr,
    .update_curr        = update_curr_grr,
};

#endif /* CONFIG_GRR_SCHED */
